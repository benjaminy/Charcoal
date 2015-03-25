(*
 * Copyright etc.
 *)

(*
 * Coroutinify: a program transformation that moves procedure call
 * frames onto the heap.  In order to do this we:
 * 1) Find all local variables and make a struct for each procedure that
 *    stores its frame (procedure calling stuff, plus local vars)
 * 2) Change calls to funky things that initialize a frame for the
 *    callee, then return that.
 * 3) Put labels everywhere and use computed gotos.
 *)

module L  = List
module C  = Cil
module E  = Errormsg
module IH = Inthash
module Pf = Printf
module T  = Trace

let spf = Printf.sprintf

let opt_map f x = match x with None -> None | Some y -> Some( f y )

let trc = T.trace "coroutinify"

let change_do_children x = C.ChangeDoChildrenPost( x, fun e -> e )

let label_counter = ref 1
let fresh_return_label loc =
  let n = !label_counter in
  let () = label_counter := n + 1 in
  C.Label( spf "__charcoal_return_%d" n, loc, false )

let charcoal_pfx   = "__charcoal_"
let unyielding_pfx = "__charcoal_fn_unyielding_"
let locals_pfx     = "__charcoal_fn_locals_"
let specific_pfx   = "__charcoal_fn_specific_"
let prologue_pfx   = "__charcoal_fn_prologue_"
let yielding_pfx   = "__charcoal_fn_yielding_"
let epilogueA_pfx  = "__charcoal_fn_epilogueA_"
let epilogueB_pfx  = "__charcoal_fn_epilogueB_"
let charcoal_pfx_len = String.length charcoal_pfx

let charcoal_pfx_regex = Str.regexp charcoal_pfx

let crcl s = "__charcoal_" ^ s

let crcl_fun_kind name =
  if name = "__charcoal_application_main" then
    ( false, name )
  else
    let c = Str.string_match charcoal_pfx_regex name 0 in
    ( c, if c then Str.string_after name charcoal_pfx_len else name )

let remove_charcoal_linkage fdec =
  let v = fdec.C.svar in
  match v.C.vtype with
    C.TFun( rt, ps, va, attrs ) ->
    let foo attr = attr = C.Attr( "linkage_charcoal", [] ) in
    ( match L.partition foo attrs with
        ( [_], others ) -> v.C.vtype <- C.TFun( rt, ps, va, others )
      | _ -> E.s( E.error "Linkage angry?!?" ) )
  | _ -> E.s( E.error "Remove linkage from non-function?!?" )

let add_charcoal_linkage fdec =
  let v = fdec.C.svar in
  match v.C.vtype with
    C.TFun( rt, ps, va, attrs ) ->
    v.C.vtype <- C.TFun( rt, ps, va, C.Attr( "linkage_charcoal", [] )::attrs )
  | _ -> E.s( E.error "Add linkage to non-function?!?" )

let is_charcoal_fn exp =
  match C.typeOf exp with
    C.TFun( _, _, _, attrs )
  | C.TPtr( C.TFun( _, _, _, attrs ), _) -> C.linkage_charcoal attrs
  | _ -> false

type frame_info =
  {
    (* generic: *)
    specific_cast   : C.typ -> C.lval -> C.exp;
    typ             : C.typ;
    typ_ptr         : C.typ;
    callee_sel      : C.exp -> C.lval;
    goto_sel        : C.exp -> C.lval;
    gen_prologue    : C.varinfo;
    gen_epilogueA   : C.varinfo;
    gen_after_ret   : C.varinfo;
    (* function-specific *)
    sizeof_specific : C.exp;
    locals_sel      : C.lval -> C.lval;
    return_sel      : C.lval -> C.lval;
    locals_type     : C.typ;
    epilogueA       : C.varinfo;
    yielding        : C.varinfo;
    locals          : C.exp -> ( ( C.offset -> C.lval ) IH.t );
    (* context-specific *)
    lval            : C.lval;
    exp             : C.exp;
  }

let function_vars : ( C.exp * C.exp * C.exp ) IH.t = IH.create 42
let lookup_fn_translation v = IH.tryfind function_vars v.C.vid
let add_fn_translation v u p a = IH.replace function_vars v.C.vid ( u, p, a )

(*
  try IH.find function_vars v.C.vid
  with Not_found ->
    (* XXX fix types: *)
    let mvi = C.makeVarinfo true in
    let i = mvi (spf "%s%s" prologue_pfx v.C.vname) C.voidType in
    let y = mvi (spf "%s%s" yielding_pfx   v.C.vname) C.voidType in
    let u = mvi (spf "%s%s" unyielding_pfx v.C.vname) C.voidType in
 *)

(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this type:
 *     union {
 *         struct /* no params or locals -> char[0] */
 *         {
 *             struct /* XXX unimp separate structs */
 *             {
 *                 p1;
 *                 p2;
 *                 p3;
 *             } params;
 *             struct
 *             {
 *                 l1;
 *                 l2;
 *                 l3;
 *             } locals;
 *         } L;
 *         rt R; /* void -> char[0] */
 *     }
 *)
let make_specific fdec fname frame_info =
  (* Crash if v doesn't have the properties expected of locals. *)
  let sanity v =
    let () = match v.C.vstorage with
        C.NoStorage | C.Register -> ()
        | C.Static | C.Extern -> E.s( E.error "static or extern local?!?" )
    in
    let () = if v.C.vglob then E.s( E.error "global local?!?" ) in
    let () = if v.C.vinline then E.s( E.error "inline local?!?" ) in
    let () = match v.C.vinit.C.init with
        None -> ()
      | Some _ ->  E.s( E.error "init of local?!?" )
    in
    ()
  in

  let fdef_loc  = fname.C.vdecl in
  let unit_type = C.TArray( C.charType, Some( C.zero ), [(*attrs*)] ) in
  let ( locals_type, locals, tags ) =
    match L.filter ( fun v -> (* XXX always false??? v.C.vreferenced *) true )
                      ( fdec.C.sformals @ fdec.C.slocals )
    with
      [] -> ( unit_type, ( fun _ -> IH.create 0 ), [] )
    | all ->
       let field v =
         let () = sanity v in
         ( v.C.vname, v.C.vtype, None, [(*attrs*)], v.C.vdecl )
       in
       let fields = L.map field all in
       let struct_name = spf "%s%s" locals_pfx fname.C.vname in
       let ci = C.mkCompInfo true struct_name ( fun _ -> fields ) [(*attrs*)] in
       let locals_ptr = ref C.zero in
       let field_select t v =
         let field =
           { C.fcomp = ci; C.fname = v.C.vname; C.ftype = v.C.vtype;
             C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = v.C.vdecl }
         in
         let () = Pf.printf "Adding %d %s\n" v.C.vid v.C.vname in
         IH.replace t v.C.vid ( fun o -> ( C.Mem !locals_ptr, C.Field( field, o ) ) )
       in
       let locals = IH.create( L.length all ) in
       let () = L.iter ( field_select locals ) all in
       (* Given exp, an expression for a locals pointer, map variable x to exp->x *)
       let locals_gen l =
         let () = locals_ptr := l in
         locals
       in
       ( C.TComp( ci, [(*attrs*)] ), locals_gen, [ C.GCompTag( ci, fdef_loc ) ] )
  in
  let return_type =
    match fname.C.vtype with
      C.TFun( C.TVoid _, _, _, _ ) -> unit_type
    | C.TFun( r, _, _, _ ) -> r
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let specific =
    let f n t = ( n, t, None, [(*attrs*)], fdef_loc ) in
    let fields = [ f "L" locals_type; f "R" return_type ] in
    let specific_name = spf "%s%s" specific_pfx fname.C.vname in
    C.mkCompInfo false specific_name ( fun _ -> fields ) [(*attrs*)]
  in
  let specific_type = C.TComp( specific, [(*attrs*)] ) in
  let select name ftype =
    let field =
      { C.fcomp = specific; C.fname = name; C.ftype = ftype;
        C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = fdef_loc }
    in
    fun frame ->
      ( C.Mem( frame_info.specific_cast specific_type frame ),
        C.Field( field, C.NoOffset ) )
  in
  let specific_tag = C.GCompTag( specific, fdef_loc ) in

  let updated_frame_info =
    { frame_info with
      sizeof_specific = C.SizeOf( specific_type );
      locals_sel      = select "L" locals_type;
      return_sel      = select "R" return_type;
      locals_type     = C.TPtr( locals_type, [(*attrs*)] );
      yielding        = frame_info.yielding;
      locals          = locals;
    }
  in
  ( tags @ [ specific_tag ], updated_frame_info )


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this prologue:
 *     frame_p __prologue_f( frame_p caller, void *ret_ptr, p1, p2, p3 )
 *     {
 *        /* NOTE: Using a call here to keep code size down.  The system is
 *         * welcome to inline it, if that's a good idea. */
 *        frame_p frame = __generic_prologue(
 *           sizeof( __specific_f ), ret_ptr, caller, __yielding_f );
 *        ( __locals_f ) *locals = select_locals_field( frame );
 *        locals->p1 = p1;
 *        locals->p2 = p2;
 *        locals->p3 = p3;
 *        /* TODO: Verify that Cil doesn't use inits on local variables (i.e. an
 *         * init in source will turn into expressions) */
 *        return frame;
 *     }
 *)
let make_prologue prologue frame original_formals =
  let fname = prologue.C.svar in
  let fdef_loc = fname.C.vdecl in
  let () =
    match prologue.C.svar.C.vtype with
      C.TFun( _, params, vararg, attrs ) ->
      prologue.C.svar.C.vtype <-
        C.TFun( frame.typ_ptr, params, vararg, attrs )
    | _ -> E.s( E.error "Prologue must be function" )
  in
  let this = C.makeLocalVar prologue "frame" frame.typ_ptr in
  let call =
    let ret_ptr = C.makeFormalVar prologue ~where:"^" "ret_ptr" C.voidPtrType in
    let caller  = C.makeFormalVar prologue ~where:"^" "caller"  frame.typ_ptr in
    let ps = L.map ( fun v -> C.Lval( C.var v ) )
                      [ ret_ptr; caller; frame.yielding ]
    in
    let gp = C.Lval( C.var frame.gen_prologue ) in
    C.Call( Some( C.var this ), gp, frame.sizeof_specific::ps, fdef_loc )
  in
  (* If the yielding function has zero parameters, skip this *)
  let param_assignments = match original_formals with
      [] -> []
    | fs ->
       let ( locals, locals_init ) =
         let l = C.makeLocalVar prologue "locals" frame.locals_type in
         let locals_val = C.AddrOf( frame.locals_sel( C.var this ) ) in
         let locals_init = C.Set( C.var l, locals_val, fdef_loc ) in
         ( frame.locals( C.Lval( C.var l ) ), locals_init )
       in
       let assign_param v =
         let local_var =
           match IH.tryfind locals v.C.vid with
             Some f -> f C.NoOffset
           | _ -> E.s( E.error "Missing local \"%s\"" v.C.vname )
         in
         C.Set( local_var, C.Lval( C.var v ), fdef_loc )
       in
       locals_init::( L.map assign_param fs )
  in
  let body =
    let instrs = call::param_assignments in
    let r = C.mkStmt( C.Return( Some( C.Lval( C.var this ) ), fdef_loc ) ) in
    C.mkBlock[ C.mkStmt( C.Instr instrs ); r ]
  in
  let () = prologue.C.sbody <- body in
  prologue


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this epilogueA:
 *     frame_p __epilogueA( frame_p frame, rt v )
 *     {
 *         return_cast( frame ) = v;
 *         return __generic_epilogueA( frame );
 *     }
 *)
let make_epilogueA epilogueA frame =
  let fdef_loc = epilogueA.C.svar.C.vdecl in
  let return_type =
    match epilogueA.C.svar.C.vtype with
      C.TFun( r, params, vararg, attrs ) ->
      let () = epilogueA.C.svar.C.vtype <-
                 C.TFun( frame.typ_ptr, params, vararg, attrs )
      in
      r
    | _ -> E.s( E.error "EpilogueA must be function" )
  in
  let () = C.setFormals epilogueA [] in
  let () = epilogueA.C.slocals <- [] in
  let this = C.var( C.makeFormalVar epilogueA "frame" frame.typ_ptr ) in
  let caller = C.var( C.makeTempVar epilogueA ~name:"caller" frame.typ_ptr ) in
  let instrs =
    let ge = C.Lval( C.var frame.gen_epilogueA ) in
    let call_instr = C.Call( Some caller, ge, [ C.Lval this ], fdef_loc ) in
    match return_type with
      C.TVoid _ -> [ call_instr ]
    | _ ->
       let rval = C.Lval( C.var( C.makeFormalVar epilogueA "v" return_type ) ) in
       [ C.Set( frame.return_sel this, rval, fdef_loc ); call_instr ]
  in
  let return_stmt = C.mkStmt( C.Return( Some( C.Lval caller ), fdef_loc ) ) in
  let body = C.mkBlock[ C.mkStmt( C.Instr instrs ); return_stmt ] in
  let () = epilogueA.C.sbody <- body in
  epilogueA


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this after-return:
 *     void __epilogueB_f( frame_p frame, rt *lhs )
 *     {
 *         /* Order is important because generic frees the callee */
 *         if( lhs )
 *             *lhs = return_cast( frame->callee );
 *         __generic_epilogueB( frame );
 *     }
 *)
let make_epilogueB after_ret frame =
  let fdef_loc = after_ret.C.svar.C.vdecl in
  let return_type =
    match after_ret.C.svar.C.vtype with
      C.TFun( r, _, _, _ ) -> r
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let () = C.setFormals after_ret [] in
  let () = after_ret.C.slocals <- [] in
  let this = C.var( C.makeFormalVar after_ret "frame" frame.typ_ptr ) in
  let lhsp =
    let l = C.makeFormalVar after_ret "lhs"
                            ( C.TPtr( return_type, [(*attrs*)] ) ) in
    C.Lval( C.var l )
  in
  let rhs = C.Lval( frame.return_sel( frame.callee_sel( C.Lval this ) ) ) in
  let assign = C.Set( ( C.Mem lhsp, C.NoOffset ), rhs, fdef_loc ) in
  let assign_block = C.mkBlock( [ C.mkStmt( C.Instr( [ assign ] ) ) ] ) in
  let empty_block = C.mkBlock( [ C.mkEmptyStmt() ] ) in
  let null_check = C.mkStmt( C.If( lhsp, assign_block, empty_block, fdef_loc ) ) in
  let ga = C.Lval( C.var frame.gen_after_ret ) in
  let call_to_generic = C.Call( None, ga, [ C.Lval this ], fdef_loc ) in
  let body = C.mkBlock[ null_check; C.mkStmt( C.Instr[ call_to_generic ] ) ] in
  let () = after_ret.C.sbody <- body in
  after_ret


(* Translate direct calls from:
 *     lhs = f( p1, p2, p3 );
 * to:
 *     return __prologue_f( frame, &__return_N, p1, p2, p3 );
 *   __return_N:
 *     __epilogueB_f( frame, &lhs );
 *
 * Translate indirect calls from:
 *     lhs = exp( p1, p2, p3 );
 * to:
 *     return exp( frame, &__return_N, null, p1, p2, p3 );
 *   __return_N:
 *     exp( frame, null, &lhs );
 *)

type dir_indir =
    CDirect of C.exp * C.exp
  | CIndirect of C.exp

let coroutinify_call visitor fdec frame instr =
  let () = Pf.printf "coroutinify_call %s\n%!" fdec.C.svar.C.vname in
  match instr with
    C.Call( lhs_opt_previs, fn_exp_previs, params_previs, loc )
       when is_charcoal_fn fn_exp_previs ->
    let () = trc (Pretty.dprintf "entering function %a\n%a\n"
                                 C.d_exp fn_exp_previs
                                 C.d_type( C.typeOf fn_exp_previs ) )
    in
    let lhs_opt = opt_map (C.visitCilLval visitor) lhs_opt_previs in
    let fn_exp = C.visitCilExpr visitor fn_exp_previs in
    let params = L.map (C.visitCilExpr visitor) params_previs in
    let call_stuff = match fn_exp with
        C.Lval( C.Var v, C.NoOffset ) ->
        ( match lookup_fn_translation v with
            Some ( _, c, a ) -> CDirect( c, a )
          | None -> CIndirect fn_exp )
      | _ -> CIndirect fn_exp
    in
    let after_call =
      (* XXX fix to be direct or indirect *)
      let params =
        match ( C.typeOf fn_exp, lhs_opt ) with
          ( C.TFun( C.TVoid _, _, _, _ ), _ ) -> [ frame.exp ]
        | ( C.TFun _, None )                  -> [ frame.exp; C.zero ]
        | ( C.TFun _, Some lhs )              -> [ frame.exp; C.AddrOf lhs ]
        | _ -> E.s( E.error "Function with non-function type?!?" )
      in
      let call =
        let f = match call_stuff with
            CDirect( _, a ) -> a
          | CIndirect p -> p
        in
        C.Call( None, f, params, loc )
      in
      let r = C.mkStmt( C.Instr[ call ] ) in
      let () = r.C.labels <- [ fresh_return_label loc ] in
      r
    in
    let callee = C.var( C.makeTempVar fdec ~name:"callee" frame.typ_ptr ) in
    (* XXX fix to be direct or indirect *)
    let ps = frame.exp::( C.AddrOfLabel( ref after_call ) )::params in
    let prologue_call =
      let f = match call_stuff with
          CDirect( c, _ ) -> c
        | CIndirect p -> p
      in
      let call = C.Call( Some callee, f, ps, loc ) in
      C.mkStmt( C.Instr[ call ] )
    in
    let return_callee = C.mkStmt( C.Return( Some( C.Lval callee ), loc ) )  in
    [ prologue_call; return_callee; after_call ]

  (* Something other than a call to a Charcoal function *)
  | _ -> [ C.mkStmt( C.Instr( C.visitCilInstr visitor instr ) ) ]


(* Translate returns from:
 *     return exp;
 * to:
 *     return __epilogueA_f( frame, exp );
 *
 * Prereq: exp should be coroutinified already
 *)
let coroutinify_return fdec rval_opt loc frame =
  let caller = C.var( C.makeTempVar fdec ~name:"caller" frame.typ_ptr ) in
  let params = match rval_opt with
      None ->   [ frame.exp ]
    | Some e -> [ frame.exp; e ]
  in
  let ep = C.Lval( C.var frame.epilogueA ) in
  let call = C.mkStmt( C.Instr[ C.Call( Some caller, ep, params, loc ) ] ) in
  let return_stmt = C.mkStmt( C.Return( Some( C.Lval caller ), loc ) ) in
  C.ChangeTo( C.mkStmt( C.Block( C.mkBlock[ call; return_stmt ] ) ) )


class coroutinifyYieldingVisitor fdec locals frame = object(self)
  inherit C.nopCilVisitor

  (* Translate uses of local variables from:
   *     x
   * to:
   *     locals->x
   *)
  (* It's tempting to use vvrbl, because we're only interested in uses
   * of locals.  However, we need to replace them with a more complex
   * lval, so we need the method that lets us return lvals. *)
  method vlval( lhost, offset ) =
    match lhost with
      C.Var v ->
      let () = trc
          (Pretty.dprintf "Replace local var? %s %d\n" v.C.vname v.C.vid )
      in
      ( match IH.tryfind locals v.C.vid with
          Some l -> let () = Pf.printf "  ...Yup\n" in change_do_children ( l offset )
        | None -> let () = Pf.printf "  ...Nope\n" in C.DoChildren )
    (* We can ignore the Mem case because we'll get it later in the visit *)
    | _ -> C.DoChildren

  (* It's tempting to use vinstr for calls.  However, we need to replace
   * them with a sequence that has returns and labels, so we need the
   * method that lets us return statements. *)
  method vstmt s =
    let is_not_call i =
      match i with
        C.Call _ -> false
      | _ -> true
    in

    match s.C.skind with
      (C.Instr instrs) ->
      if L.for_all is_not_call instrs then
        C.DoChildren
      else
        let coroutinify_call_here =
          coroutinify_call ( self :> C.cilVisitor ) fdec frame
        in
        let stmts = L.concat( L.map coroutinify_call_here instrs ) in
        C.ChangeTo( C.mkStmt( C.Block( C.mkBlock stmts ) ) )

    | ( C.Return( rval_opt, loc ) ) ->
       let x = opt_map ( C.visitCilExpr ( self :> C.cilVisitor ) ) rval_opt in
       coroutinify_return fdec x loc frame

    | _ -> C.DoChildren
end

(* Translate:
 *     rt f( p1, p2, p3 ) { ... }
 * to:
 *     frame_p __yielding_f( frame_p frame )
 *     {
 *         __locals_f *locals = &locals_cast( frame );
 *         if( frame->goto_address )
 *             goto frame->goto_address;
 *         coroutinify( ... )
 *     }
 *)

(* XXX There's probably a bug because we introduce new locals after making
 * the locals struct *)
(* XXX Or maybe there's not a bug.  Maybe none of the locals introduced by
 * coroutinification are live across calls. *)
(* XXX Also we should only allocate struct space for locals that are live
 * across returns.  Others can just be regular locals. *)
(* XXX Also some day we can do register allocation style optimizations to
 * share slots in the locals struct *)
let make_yielding yielding frame_info_spec =
  let () = Pf.printf "make_yielding %s\n" yielding.C.svar.C.vname in
  let fdef_loc = yielding.C.svar.C.vdecl in
  let () =
    match yielding.C.svar.C.vtype with
      C.TFun( _, params, vararg, attrs ) ->
      yielding.C.svar.C.vtype <-
        C.TFun( frame_info_spec.typ_ptr, params, vararg, attrs )
    | _ -> E.s( E.error "Yielding must be function" )
  in
  let () = C.setFormals yielding [] in
  let () = yielding.C.slocals <- [] in
  let this = C.makeFormalVar yielding "frame" frame_info_spec.typ_ptr in
  let frame_info = {
      frame_info_spec with lval = C.var this;
                           exp = C.Lval( C.var this ); }
  in
  let ( locals_tbl, locals_init ) =
    let locals = C.var( C.makeTempVar yielding ~name:"locals" frame_info.locals_type ) in
    let locals_val = C.AddrOf( frame_info.locals_sel( C.var this ) ) in
    let locals_init_instr = C.Set( locals, locals_val, fdef_loc ) in
    let locals_init = C.mkStmt( C.Instr[ locals_init_instr ] ) in
    ( frame_info.locals( C.Lval( locals ) ), locals_init )
  in
  let goto_stmt =
    let goto_field = C.Lval( frame_info.goto_sel( C.Lval( C.var this ) ) ) in
    let empty_block = C.mkBlock( [ C.mkEmptyStmt() ] ) in
    let goto = C.mkStmt( C.ComputedGoto( goto_field, fdef_loc ) ) in
    C.mkStmt( C.If( goto_field, C.mkBlock( [ goto ] ), empty_block, fdef_loc ) )
  in
  let v = new coroutinifyYieldingVisitor yielding locals_tbl frame_info in
  let y = C.visitCilFunction v yielding in
  let () = y.C.sbody.C.bstmts <- locals_init :: goto_stmt :: y.C.sbody.C.bstmts in
  y

(* In the body of f, change calls to unyielding versions *)
class coroutinifyUnyieldingVisitor = object(self)
  inherit C.nopCilVisitor

  method indirect_call lval f ps loc =
    let lval_param = match lval with
        None   -> C.zero
      | Some l -> C.AddrOf l
    in
    C.ChangeTo[ C.Call( None, f, C.zero::C.zero::lval_param::ps, loc ) ]

  method vinst i =
    match i with
      C.Call( lval_opt, ( C.Lval( C.Var fname, C.NoOffset ) as f), params, loc )
         when is_charcoal_fn f ->
      (match lookup_fn_translation fname with
         Some( u, _ , _ ) ->
         C.ChangeTo[ C.Call( lval_opt, u, params, loc ) ]
       | None -> self#indirect_call lval_opt f params loc )

      (* Indirect call: *)
    | C.Call( lval_opt, fn, params, loc )
         when is_charcoal_fn fn ->
       self#indirect_call lval_opt fn params loc

    | _ -> C.SkipChildren
end

let make_unyielding unyielding =
  let v = new coroutinifyUnyieldingVisitor in
  C.visitCilFunction ( v :> C.cilVisitor ) unyielding

(* XXX Need to replace &fn with &__indirect_fn ... Maybe not ... *)
(* XXX Need to replace fun ptr types w/ our craziness *)

(* For this function definition
 *     rt f( p1, p2, p3 ) { ... }
 * Generate the function for indirect calling:
 * NOTE: This function can be called in three different contexts:
 *   - unyielding mode (indicated by caller == NULL)
 *   - yielding mode prologue (indicated by ret != NULL)
 *   - yielding mode after return (neither of the above cases)
 *
 *     frame_p __indirect_f( frame_p caller, void *ret, rt *lhs, struct *ps )
 *     {
 *         /* UNIMP assertions */
 *         assert( lhs ==> !( caller && ret ) );
 *         assert( ret ==> caller );
 *         assert( ( ps && caller ) ==> !lhs );
 *         rt temp;
 *         if( caller ) /* Caller in yielding mode */
 *         {
 *     #if yielding version exists
 *             if( ret )
 *                 return __prologue_f( caller, ret, ps->p1, ps->p2, ps->p3 );
 *             else
 *                 __epilogueB_f( caller, lhs );
 *                 return 0;
 *     #else
 *             if( ret )
 *             {
 *                 frame_p frame = __generic_prologue(
 *                     sizeof( __specific_f ), ret, caller, 0 );
 *                 return_cast( frame ) = __unyielding_f( ps->p1, ps->p2, ps->p3 );
 *             }
 *             else
 *             {
 *                 temp = return_cast( caller->callee );
 *                 __generic_epilogueB( caller );
 *             }
 *     #endif
 *         }
 *         else /* Caller in unyielding mode */
 *         {
 *     #if unyielding version exists
 *             temp = __unyielding_f( ps->p1, ps->p2, ps->p3 );
 *     #else
 *             ERROR Cannot call yielding-only functions in unyielding mode!!!
 *             exit();
 *     #endif
 *         }
 *         if( lhs )
 *             *lhs = temp;
 *         return caller;
 *     }
 *)
(* NOTE: We're taking "ownership" of the original Charcoal function here, because
 * it makes less work for finding address-of operations later. *)
let make_indirect original frame_info =
  let return_type_opt =
    match original.C.svar.C.vtype with
      C.TFun( rt, params, vararg, attrs ) ->
      let () =
        original.C.svar.C.vtype <-
          C.TFun( frame_info.typ_ptr, params, vararg, attrs )
      in
      ( match rt with
          C.TVoid _ -> None
        | r -> Some r
      )
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let original_formals = original.C.sformals in
  let () = C.setFormals original [] in
  let () = original.C.slocals <- [] in
  let caller  = C.makeFormalVar original "caller"  frame_info.typ_ptr in
  let ret_ptr = C.makeFormalVar original "ret_ptr" C.voidPtrType in
  let lhs_opt =
    let l rt = C.makeFormalVar original "lhs" ( C.TPtr( rt, [(*attrs*)] ) ) in
    opt_map l return_type_opt
  in

(* let locals = C.var( C.makeTempVar yielding ~name:"locals" frame_info.locals_type ) in *)

  let return_stmt = C.mkStmt( C.Return( Some( C.Lval( C.var caller ) ),
                                        original.C.svar.C.vdecl ) ) in
  let () = original.C.sbody.C.bstmts <- [ return_stmt ] in
  original (* XXX *)


(* When we find the definition of the generic frame type, examine it and
 * record some stuff. *)
let examine_frame_t_struct ci dummy_var =
  let dummy_lval  = ( C.Var dummy_var, C.NoOffset ) in
  let dummy_exp   = C.zero in
  let dummy_type  = C.voidType in

  let _, s, g, ce =
    let ar, sr, gr, cer = ref None, ref None, ref None, ref None in
    let fields = [ ( "activity", ar ); ( "specific", sr );
                   ( "goto_address", gr ); ( "callee", cer ); ] in
    let extract field =
      let find_field ( name, field_ref ) =
        if field.C.fname = name then
          field_ref := Some field
      in
      L.iter find_field fields
    in
    let () = L.iter extract ci.C.cfields in
    match ( !ar, !sr, !gr, !cer ) with
      Some a, Some s, Some g, Some ce -> a, s, g, ce
    | _ -> E.s( E.error "frame struct missing fields?!?" )
  in
  let specific_cast specific_type frame =
    let t = C.TPtr( specific_type, [(*attrs*)] ) in
    let e = C.AddrOf( C.Mem( C.Lval frame ), C.Field( s, C.NoOffset ) ) in
    C.mkCast e t
  in

  {
    (* generic: *)
    specific_cast   = specific_cast;
    typ             = C.TComp( ci, [(*attrs*)] );
    typ_ptr         = C.TPtr( C.TComp( ci, [(*attrs*)] ), [(*attrs*)] );
    callee_sel      = ( fun e -> ( C.Mem e, C.Field( ce, C.NoOffset ) ) );
    goto_sel        = ( fun e -> ( C.Mem e, C.Field( g, C.NoOffset ) ) );
    gen_prologue    = dummy_var;
    gen_epilogueA   = dummy_var;
    gen_after_ret   = dummy_var;
    (* function-specific *)
    sizeof_specific = dummy_exp;
    locals_sel      = ( fun e -> dummy_lval );
    return_sel      = ( fun e -> dummy_lval );
    locals_type     = dummy_type;
    yielding        = dummy_var;
    epilogueA       = dummy_var;
    locals          = ( fun _ -> IH.create 0 );
    (* context-specific *)
    lval            = dummy_lval;
    exp             = dummy_exp;
  }

let make_function_skeletons f n =
  let () = remove_charcoal_linkage f in
  let y = C.copyFunction f ( spf "%s%s"   yielding_pfx n ) in
  let u = C.copyFunction f ( spf "%s%s" unyielding_pfx n ) in
  let c = C.copyFunction f ( spf "%s%s"   prologue_pfx n ) in
  let e = C.copyFunction f ( spf "%s%s"  epilogueA_pfx n ) in
  let a = C.copyFunction f ( spf "%s%s"  epilogueB_pfx n ) in
  let () = add_charcoal_linkage f in
  ( y, u, c, e, a )

class globals_visitor = object(self)
  inherit C.nopCilVisitor

  val mutable frame_opt = None
  val mutable frame_struct = None

  (* If we have encountered the def of crcl(frame_t), extract its fields. *)
  method fill_in_frame v =
    match ( frame_opt, frame_struct ) with
      ( None, None ) -> ()
    | ( None, Some ci ) ->
       let () = frame_opt <- Some( examine_frame_t_struct ci v ) in
       frame_struct <- None
    | ( Some frame_info, None ) ->
       let name = v.C.vname in
       let () =
         if name = crcl "fn_generic_prologue" then
           frame_opt <- Some{ frame_info with gen_prologue = v };
         if name = crcl "fn_generic_epilogueA" then
           frame_opt <- Some{ frame_info with gen_epilogueA = v };
         if name = crcl "fn_generic_epilogueB" then
           frame_opt <- Some{ frame_info with gen_after_ret = v };
       in
       ()
    | ( Some _, Some _ ) -> E.s( E.error "Too much frame" )

  method vglob g =
    let () = let p () = match g with
      | C.GType( t, l )    -> Pf.printf "T %s\n%!" t.C.tname
      | C.GCompTag( c, l ) -> Pf.printf "CT %s\n%!" c.C.cname
      | C.GCompTagDecl( c, l ) -> Pf.printf "CTD %s\n%!" c.C.cname
      | C.GEnumTag( e, l ) -> Pf.printf "ET\n%!"
      | C.GEnumTagDecl( e, l ) -> Pf.printf "ETD\n%!"
      | C.GVarDecl( v, l ) -> ()(*Pf.printf "VD %s\n%!" v.C.vname*)
      | C.GVar( v, i, l ) -> Pf.printf "V %s\n%!" v.C.vname
      | C.GFun( f, l ) -> Pf.printf "F %s\n%!" f.C.svar.C.vname
      | C.GAsm( s, l ) -> Pf.printf "A\n%!"
      | C.GPragma( a, l ) -> Pf.printf "P\n%!"
      | C.GText( s ) -> Pf.printf "T\n%!"
    in (* p *) () in
    match g with
    | C.GFun( original, loc ) ->
       let orig_var  = original.C.svar in
       let orig_name = orig_var.C.vname in
       (* let () = Pf.printf "ffv - vg - %s\n" orig_name in *)
       let () = self#fill_in_frame orig_var in
       let kind = crcl_fun_kind orig_name in
       (match (kind, frame_opt) with
          ( _, None ) ->
          E.s( E.error "GFun before frame_t def  :(  %s!" orig_name )
        | ( ( false, _ ), Some generic_frame_info ) ->
          (* NOTE: These are modified below. *)
          let( yielding, unyielding, prologue, epilogueA, epilogueB ) =
            make_function_skeletons original orig_name
          in
          let ( tags, specific_frame_info ) =
            make_specific yielding orig_var generic_frame_info
          in
          let frame_info =
            { specific_frame_info with epilogueA = epilogueA.C.svar;
                                       yielding = yielding.C.svar}
          in
          (* "p" before "y" *)
          let p = make_prologue     prologue frame_info yielding.C.sformals in
          let y = make_yielding     yielding frame_info in
          let u = make_unyielding unyielding in
          let e = make_epilogueA   epilogueA frame_info in
          let a = make_epilogueB   epilogueB frame_info in
          let i = make_indirect     original frame_info in
          let () =
            let e f = C.Lval( C.var( f.C.svar ) ) in
            add_fn_translation orig_var ( e u ) ( e p ) ( e a )
          in
          let decls = L.map ( fun f -> C.GVarDecl( f.C.svar, loc ) )
                            [ y; u; e; p; a; i ]
          in
          let funs = L.map ( fun f -> C.GFun( f, loc ) )
                           [ y; u; e; p; a; i ]
          in
          C.ChangeTo ( tags @ decls @ funs )

        | ( ( true, crcl_runtime_fn_name ), Some frame ) ->
           let () =
             if crcl_runtime_fn_name = "fn_generic_prologue" then
               frame_opt <-
                 Some{ frame with gen_prologue = orig_var };
             if crcl_runtime_fn_name = "fn_generic_epilogueA" then
               frame_opt <-
                 Some{ frame with gen_epilogueA = orig_var };
             if crcl_runtime_fn_name = "fn_generic_epilogueB" then
               frame_opt <-
                 Some{ frame with gen_after_ret = orig_var };
           in
           C.SkipChildren
       )

    | C.GVarDecl( var, _ ) ->
       let () = self#fill_in_frame var in
       C.DoChildren

    (* We need to find various struct definitions *)
    | C.GCompTag( ci, loc ) ->
       (* let () = Pf.printf "ffv - gct - %s\n" ci.C.cname in *)
       if ci.C.cname = "__charcoal_frame_t" then
         let () = frame_struct <- Some ci in
         C.DoChildren
       else
         C.DoChildren

    | C.GVar( v, _, _ ) ->
       let () = self#fill_in_frame v in
       C.DoChildren

    | _ -> C.SkipChildren
end

let do_coroutinify( f : C.file ) =
  let () = C.visitCilFile (new globals_visitor :> C.cilVisitor) f in
  ()

let feature : C.featureDescr =
  { C.fd_name = "coroutinify";
    C.fd_enabled = Cilutil.doCoroutinify;
    C.fd_description = "heap-allocate call frames";
    C.fd_extraopt = [];
    C.fd_doit = do_coroutinify;
    C.fd_post_check = true ;
  }

