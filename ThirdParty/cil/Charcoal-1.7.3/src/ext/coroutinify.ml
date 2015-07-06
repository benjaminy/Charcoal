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

let getTFunInfo t msg =
  match t with
    C.TFun( r, ps, v, atts ) -> ( r, ps, v, atts )
  | _ -> E.s( E.error "Expecting function type; %s" msg )

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

let remove_charcoal_linkage_from_type expect_crcl t =
  match t with
    C.TFun( rt, ps, va, attrs ) ->
    let foo attr = attr = C.Attr( "linkage_charcoal", [] ) in
    ( match L.partition foo attrs with
        ( [_], others ) -> Some( C.TFun( rt, ps, va, others ) )
      | _ -> if expect_crcl then
               E.s( E.error "Linkage angry?!?" )
             else
               None )
  | _ -> None

let remove_charcoal_linkage fdec =
  let v = fdec.C.svar in
  match remove_charcoal_linkage_from_type true v.C.vtype with
    Some t' -> v.C.vtype <- t'
  | _ -> E.s( E.error "Remove linkage from non-function?!?" )

let add_charcoal_linkage_var v =
  let ( rt, ps, va, attrs ) = getTFunInfo v.C.vtype "Add linkage?!?" in
  v.C.vtype <- C.TFun( rt, ps, va, C.Attr( "linkage_charcoal", [] )::attrs )

let add_charcoal_linkage fdec = add_charcoal_linkage_var fdec.C.svar

let type_is_charcoal_fn ty =
  match ty with
    C.TFun( _, _, _, attrs )
  | C.TPtr( C.TFun( _, _, _, attrs ), _) -> C.linkage_charcoal attrs
  | _ -> false

let exp_is_charcoal_fn exp = type_is_charcoal_fn( C.typeOf exp )

type frame_info =
  {
    (* generic: *)
    specific_cast   : C.typ -> C.lval -> C.exp;
    typ             : C.typ;
    typ_ptr         : C.typ;
    epilogueB_typ   : C.typ;
    caller_sel      : C.exp -> C.lval;
    callee_sel      : C.exp -> C.lval;
    return_addr_sel : C.exp -> C.lval;
    oldest_sel      : C.exp -> C.lval;
    gen_prologue    : C.varinfo;
    gen_epilogueA   : C.varinfo;
    gen_epilogueB   : C.varinfo;
    activate        : C.varinfo;
    act_intermed    : C.varinfo;
    act_epilogue    : C.varinfo;
    yield           : C.varinfo;
    yield_impl      : C.exp;
    (* function-specific *)
    sizeof_specific : C.exp;
    locals_sel      : C.lval -> C.lval;
    return_sel      : C.lval -> C.lval;
    locals_type     : C.typ;
    epilogueA       : C.varinfo;
    epilogueB       : C.varinfo;
    yielding        : C.varinfo;
    locals          : C.exp -> ( ( C.offset -> C.lval ) IH.t );
    ret_type        : C.typ;
    param_types     : ( string * C.typ * C.attributes ) list;
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
  let return_type, real_return_type, param_types =
    match fname.C.vtype with
      C.TFun( C.TVoid _ as r, ps, _, _ ) -> unit_type, r, ps
    | C.TFun( r, ps, _, _ ) -> r, r, ps
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
    let ( _, ps, va, attrs ) = getTFunInfo prologue.C.svar.C.vtype "Pro" in
    prologue.C.svar.C.vtype <- C.TFun( frame.typ_ptr, ps, va, attrs )
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
    let ( r, ps, va, attrs ) = getTFunInfo epilogueA.C.svar.C.vtype "EpiA" in
    let () = epilogueA.C.svar.C.vtype <-
               C.TFun( frame.typ_ptr, ps, va, attrs )
    in
    r
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
 * Generate this epilogueB:
 *     void __epilogueB_f( frame_p frame, rt *lhs )
 *     {
 *         /* Order is important because generic frees the callee */
 *         if( lhs )
 *             *lhs = return_cast( frame->callee );
 *         __generic_epilogueB( frame );
 *     }
 *)
let make_epilogueB epilogueB frame =
  let fdef_loc = epilogueB.C.svar.C.vdecl in
  let return_type =
    let ( r, _, _, _ ) = getTFunInfo epilogueB.C.svar.C.vtype "EpiB" in
    r
  in
  let () =
    let () = C.setFormals epilogueB [] in
    C.setFunctionType epilogueB
        ( C.TFun( C.voidType, Some [], false, [] ) )
  in
  let () = epilogueB.C.slocals <- [] in
  let this = C.var( C.makeFormalVar epilogueB "frame" frame.typ_ptr ) in
  let ga = C.Lval( C.var frame.gen_epilogueB ) in
  let call_to_generic = C.Call( None, ga, [ C.Lval this ], fdef_loc ) in
  let body = match return_type with
      C.TVoid _ -> C.mkBlock[ C.mkStmt( C.Instr[ call_to_generic ] ) ]
    | _ ->
       let lhsp =
         let l = C.makeFormalVar epilogueB "lhs"
                                 ( C.TPtr( return_type, [(*attrs*)] ) ) in
         C.Lval( C.var l )
       in
       let rhs = C.Lval( frame.return_sel( frame.callee_sel( C.Lval this ) ) ) in
       let assign = C.Set( ( C.Mem lhsp, C.NoOffset ), rhs, fdef_loc ) in
       let assign_block = C.mkBlock( [ C.mkStmt( C.Instr( [ assign ] ) ) ] ) in
       let empty_block = C.mkBlock( [ C.mkEmptyStmt() ] ) in
       let null_check = C.mkStmt( C.If( lhsp, assign_block, empty_block, fdef_loc ) ) in
       C.mkBlock[ null_check; C.mkStmt( C.Instr[ call_to_generic ] ) ]
  in
  let () = epilogueB.C.sbody <- body in
  epilogueB

(*
 * Translate:
 *     __activate_intermediate( act, fn, p1, p2, p3 );
 * to:
 *     act_frame = __prologue_fn( frame, 0, p1, p2, p3 );
 *     return __activate( frame, &__return_N, act, act_frame, __epilogueB_fn XXX );
 *   __return_N:
 *)
let coroutinify_activate params fdec loc frame =
  let act, entry_fn, real_params =
    match params with
      a::e::r -> a, e, r
    | _ -> E.s( E.bug "Call to activate_intermediate with too few params" )
  in
  let prologue, epilogueB =
    match entry_fn with
      C.AddrOf( C.Var v, C.NoOffset ) ->
      ( match lookup_fn_translation v with
          Some ( _, p, eB ) -> p, (C.mkCast eB frame.epilogueB_typ)
        | None -> E.s( E.bug "Missing translation for activate entry %a"
                             C.d_lval( C.var v ) ) )
    | _ -> E.s( E.bug "Activate entry exp is not a variable! %a" C.d_exp entry_fn )
  in
  let after_act =
    let r = C.mkStmt( C.Block( C.mkBlock [] ) ) in
    let () = r.C.labels <- [ fresh_return_label loc ] in
    r
  in
  let act_frame = C.var( C.makeTempVar fdec ~name:"act_frame" frame.typ_ptr ) in
  let prologue_call =
    let ps = frame.exp::C.zero::real_params in
    C.Call( Some act_frame, prologue, ps, loc )
  in
  let next_frame = C.var( C.makeTempVar fdec ~name:"next_frame" frame.typ_ptr ) in
  let activate_call =
    let ps = [ frame.exp; C.AddrOfLabel( ref after_act ); act;
               C.Lval act_frame; epilogueB ] in
    C.Call( Some next_frame, C.Lval( C.var frame.activate ), ps, loc )
  in
  let return_next_frame = C.mkStmt( C.Return( Some( C.Lval next_frame ), loc ) )  in
  let p, a =
    C.mkStmt( C.Instr( [ prologue_call ] ) ),
    C.mkStmt( C.Instr( [ activate_call ] ) )
  in
  [ p; a; return_next_frame; after_act ]


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
 *
 *)
(*
 * TODO: experiment with translating the after-return to
 *     lhs = (ret type)frame->callee->_.R;
 *     free( frame->callee );
 *)
type dir_indir =
    CActivate
  | CYield
  | CDirect of C.exp * C.exp * C.exp
  | CIndirect of C.exp

let coroutinify_normal_call lhs_opt params call_stuff fdec loc frame =
  let epilogueB_call =
    (* XXX fix to be direct or indirect *)
    let params =
      let f = match call_stuff with
          CDirect( u, _, _ ) -> u
        | CIndirect e -> E.s( E.unimp "EERR 89235 %a" C.d_exp e )
        | _ -> E.s( E.unimp "EERR 34254" )
      in
      match ( C.typeOf f, lhs_opt ) with
        ( C.TFun( C.TVoid _, _, _, _ ), _ ) -> [ frame.exp ]
      | ( C.TFun( t, _, _, _ ), None )      -> [ frame.exp; C.zero ]
      | ( C.TFun _, Some lhs )              -> [ frame.exp; C.AddrOf lhs ]
      | _ -> E.s( E.error "Function with non-function type?!?" )
    in
    let call =
      let f = match call_stuff with
          CDirect( _, _, eB ) -> eB
        | CIndirect p -> p
        | CActivate | CYield -> E.s( E.bug "Noo act yield" )
      in
      C.Call( None, f, params, loc )
    in
    let r = C.mkStmt( C.Instr[ call ] ) in
    let () = r.C.labels <- [ fresh_return_label loc ] in
    r
  in
  let callee = C.var( C.makeTempVar fdec ~name:"callee" frame.typ_ptr ) in
  (* XXX fix to be direct or indirect *)
  let ps = frame.exp::( C.AddrOfLabel( ref epilogueB_call ) )::params in
  let prologue_call =
    let f = match call_stuff with
        CDirect( _, p, _ ) -> p
      | CIndirect p -> p
      | CActivate | CYield -> E.s( E.bug "Noooo Act yiel" )
    in
    let call = C.Call( Some callee, f, ps, loc ) in
    C.mkStmt( C.Instr[ call ] )
  in
  let return_callee = C.mkStmt( C.Return( Some( C.Lval callee ), loc ) )  in
  [ prologue_call; return_callee; epilogueB_call ]

let coroutinify_yield lhs_opt params fdec loc frame =
  let post_yield_stmt =
    let inst =
      match lhs_opt with
        None -> []
      | Some lhs -> [ C.Set( lhs, (* XXX yielded?*) C.zero, loc ) ]
    in
    let post = C.mkStmt( C.Instr inst ) in
    let () = post.C.labels <- [ fresh_return_label loc ] in
    post
  in
  let next_frame = C.var( C.makeTempVar fdec ~name:"next_frame" frame.typ_ptr ) in
  let params = [ frame.exp; C.AddrOfLabel( ref post_yield_stmt ) ] in
  let yield_call = C.Call( Some next_frame, frame.yield_impl, params, loc ) in
  let return_next = C.mkStmt( C.Return( Some( C.Lval next_frame ), loc ) )  in
  [ C.mkStmt( C.Instr[ yield_call ] ); return_next; post_yield_stmt ]

let coroutinify_call visitor fdec frame instr =
  let () = trc( Pretty.dprintf "coroutinify_call %a %a\n"
                               C.d_lval ( C.var fdec.C.svar )
                               C.d_instr instr ) in
  match instr with
    C.Call( lhs_opt_pre_vis, fn_exp_pre_vis, params_pre_vis, loc )
       when exp_is_charcoal_fn fn_exp_pre_vis ->
    let () = trc (Pretty.dprintf "entering function %a\n%a\n"
                                 C.d_exp fn_exp_pre_vis
                                 C.d_type( C.typeOf fn_exp_pre_vis ) )
    in
    let lhs_opt = opt_map (C.visitCilLval visitor) lhs_opt_pre_vis in
    let fn_exp = C.visitCilExpr visitor fn_exp_pre_vis in
    let params = L.map (C.visitCilExpr visitor) params_pre_vis in
    let call_stuff = match fn_exp with
        C.Lval( C.Var v, C.NoOffset ) ->
        if v.C.vid = frame.act_intermed.C.vid then
          CActivate
        else if v.C.vid = frame.yield.C.vid then
          CYield
        else
          ( match lookup_fn_translation v with
              Some ( u, p, eB ) -> CDirect( u, p, eB )
            | None -> CIndirect fn_exp )
      | _ -> CIndirect fn_exp
    in
    ( match call_stuff, lhs_opt with
        CActivate, Some _ ->
        E.s( E.bug "ERR 129637" )
      | CActivate, None -> coroutinify_activate params fdec loc frame
      | CYield, _ -> coroutinify_yield lhs_opt params fdec loc frame
      | _ -> coroutinify_normal_call lhs_opt params call_stuff fdec loc frame
    )

  (* Something other than a call to a Charcoal function *)
  | _ -> [ C.mkStmt( C.Instr( C.visitCilInstr visitor instr ) ) ]


(* Translate returns from:
 *     return exp;
 * to:
 *     return __epilogueA_f( frame, exp );
 *
 * Prereq: exp should be coroutinified already
 *)
(*
 * Experimenting with not calling epilogue functions
 * Translate returns from:
 *     return exp;
 * to:
 *     return_cast( frame ) = exp;
 *     return frame->caller;
 *
 * Prereq: exp should be coroutinified already
 *)
(*
 * If is activity entry:
 *     return exp;
 * to:
 *     __epilogueA_f( frame, exp );
 *     return activity_epilogue( frame );
 *)
let coroutinify_return fdec rval_opt loc frame is_activity_entry =
  if false then
    let set_return_val = match rval_opt with
        None -> []
      | Some e ->
         let set_rv = C.Set( frame.return_sel frame.lval, e, loc ) in
         [ C.mkStmt( C.Instr( [ set_rv ] ) ) ]
    in
    let caller = C.Lval( frame.caller_sel( C.Lval frame.lval ) ) in
    let return_stmt = [ C.mkStmt( C.Return( Some( caller ), loc ) ) ] in
    C.ChangeTo( C.mkStmt( C.Block( C.mkBlock( set_return_val @ return_stmt ) ) ) )
  else
    let next =
      C.var( C.makeTempVar fdec ~name:"next_frame" frame.typ_ptr )
    in
    let params = match rval_opt with
        None ->   [ frame.exp ]
      | Some e -> [ frame.exp; e ]
    in
    let callEpA =
      C.Call( Some next, C.Lval( C.var frame.epilogueA ), params, loc ) in
    let callActEp =
      C.Call( Some next, C.Lval( C.var frame.act_epilogue ), [ frame.exp ], loc ) in
    let calls =
      if is_activity_entry then [ callEpA; callActEp ] else [ callEpA ]
    in
    let return_stmt = C.mkStmt( C.Return( Some( C.Lval next ), loc ) ) in
    let stmts = [ C.mkStmt( C.Instr( calls ) ); return_stmt ] in
    C.ChangeTo( C.mkStmt( C.Block( C.mkBlock stmts ) ) )

(* Translate uses of local variables from:
 *     x
 * to:
 *     locals->x
 *)
let coroutinify_local_var locals lhost offset =
  match lhost with
    C.Var v ->
    ( match IH.tryfind locals v.C.vid with
        Some l -> change_do_children ( l offset )
      | None -> C.DoChildren )
  (* We can ignore the Mem case because we'll get it later in the visit *)
  | _ -> C.DoChildren

(*
 * Translate:
 *     __activate_intermediate( act, fn, p1, p2, p3 );
 * to:
 *     act_frame = __prologue_fn( 0, 0, p1, p2, p3 );
 *     __activate( 0, 0, act, act_frame, __epilgoueB_fn );
 *)
let unyielding_activate params fdec loc frame =
  let act, entry_fn, real_params =
    match params with
      a::e::r -> a, e, r
    | _ -> E.s( E.bug "Call to activate_intermediate with too few params" )
  in
  let prologue, epilogueB =
    match entry_fn with
      C.AddrOf( C.Var v, C.NoOffset ) ->
      ( match lookup_fn_translation v with
          Some ( _, p, eB ) -> p, (C.mkCast eB frame.epilogueB_typ)
        | None -> E.s( E.bug "Missing translation for activate entry %a"
                             C.d_lval( C.var v ) ) )
    | _ -> E.s( E.bug "Activate entry exp is not a variable! %a" C.d_exp entry_fn )
  in
  let act_frame = C.var( C.makeTempVar fdec ~name:"act_frame" frame.typ_ptr ) in
  let prologue_call =
    let ps = C.zero::C.zero::real_params in
    C.Call( Some act_frame, prologue, ps, loc )
  in
  let activate_call =
    let ps = [ C.zero; C.zero; act; C.Lval act_frame; epilogueB ] in
    C.Call( None, C.Lval( C.var frame.activate ), ps, loc )
  in
  [ prologue_call; activate_call ]

let unyielding_indirect_call lval f ps loc =
  let lval_param = match lval with
      None   -> C.zero
    | Some l -> C.AddrOf l
  in
  change_do_children[ C.Call( None, f, C.zero::C.zero::lval_param::ps, loc ) ]

let unyielding_call i fdec frame =
  match i with
    C.Call( lval_opt, ( C.Lval( C.Var fname, C.NoOffset ) as f), params, loc )
       when exp_is_charcoal_fn f ->
    if fname.C.vid = frame.act_intermed.C.vid then
      change_do_children( unyielding_activate params fdec loc frame )
    else if fname.C.vid = frame.yield.C.vid then
      change_do_children(
          match lval_opt with
            None -> []
          | Some lval -> [ C.Set( lval, C.zero, loc ) ] )
    else
      (match lookup_fn_translation fname with
         Some( u, _ , _ ) ->
         change_do_children[ C.Call( lval_opt, u, params, loc ) ]
       | None -> unyielding_indirect_call lval_opt f params loc )

  (* Indirect call: *)
  | C.Call( lval_opt, fn, params, loc ) when exp_is_charcoal_fn fn ->
     unyielding_indirect_call lval_opt fn params loc

  | _ -> C.DoChildren

(* In the body of f, change calls to unyielding versions *)
class coroutinifyUnyieldingVisitor fdec frame = object(self)
  inherit C.nopCilVisitor

  method vexpr e = match e with
      C.UnOp( C.NoYield, exp, _ (* Could type matter? *) ) ->
      change_do_children exp
    | _ -> C.DoChildren

  method vinst i = unyielding_call i fdec frame

  method vstmt s =
    match s.C.skind with
      C.NoYieldStmt( b, _ ) -> change_do_children( C.mkStmt( C.Block b ) )
    | _ -> C.DoChildren
end

class coroutinifyYieldingVisitor fdec locals frame is_activity_entry = object(self)
  inherit C.nopCilVisitor

  val mutable no_yield_depth = []

  method yielding_mode () = no_yield_depth = []

  (* It's tempting to use vvrbl, because we're only interested in uses
   * of locals.  However, we need to replace them with a more complex
   * lval, so we need the method that lets us return lvals. *)
  method vlval( lhost, offset ) =
    coroutinify_local_var locals lhost offset

  method vexpr e = match e with
      C.UnOp( C.NoYield, exp, _ (* Could type matter? *) ) ->
      let () = no_yield_depth <- ()::no_yield_depth in
      let dumb_post x =
        let () = match no_yield_depth with
            _ :: d -> no_yield_depth <- d
          | _ -> E.s( E.bug "2907456370" )
        in
        x
      in
      C.ChangeDoChildrenPost( exp, dumb_post )
    | _ -> C.DoChildren

  method vinst i =
    if self#yielding_mode() then
      C.DoChildren
    else
      unyielding_call i fdec frame

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
      else if self#yielding_mode() then
        let coroutinify_call_here =
          coroutinify_call ( self :> C.cilVisitor ) fdec frame
        in
        let stmts = L.concat( L.map coroutinify_call_here instrs ) in
        C.ChangeTo( C.mkStmt( C.Block( C.mkBlock stmts ) ) )
      else
        C.DoChildren

    | ( C.Return( rval_opt, loc ) ) ->
       let x = opt_map ( C.visitCilExpr ( self :> C.cilVisitor ) ) rval_opt in
       coroutinify_return fdec x loc frame is_activity_entry

    | C.NoYieldStmt( body, loc ) ->
       let () = no_yield_depth <- ()::no_yield_depth in
       let dumb_post x =
         let () = match no_yield_depth with
             _ :: d -> no_yield_depth <- d
           | _ -> E.s( E.bug "2907456370" )
         in
         x
       in
       C.ChangeDoChildrenPost( C.mkStmt( C.Block( body ) ), dumb_post )

    | _ -> C.DoChildren
end

(* Translate:
 *     rt f( p1, p2, p3 ) { ... }
 * to:
 *     frame_p __yielding_f( frame_p frame )
 *     {
 *         __locals_f *locals = &locals_cast( frame );
 *         if( frame->return_addr )
 *             goto frame->return_addr;
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
let make_yielding yielding frame_info_spec is_activity_entry =
  let () = Pf.printf "make_yielding %s\n" yielding.C.svar.C.vname in
  (* let () = *)
  (*   trc( Pretty.dprintf "%a\n" C.d_block yielding.C.sbody ) *)
  (* in *)
  let fdef_loc = yielding.C.svar.C.vdecl in
  let () =
    let ( _, ps, va, attrs ) = getTFunInfo yielding.C.svar.C.vtype "Yield" in
    yielding.C.svar.C.vtype <-
      C.TFun( frame_info_spec.typ_ptr, ps, va, attrs )
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
    let ret_addr_field = C.Lval( frame_info.return_addr_sel( C.Lval( C.var this ) ) ) in
    let empty_block = C.mkBlock( [ C.mkEmptyStmt() ] ) in
    let goto = C.mkStmt( C.ComputedGoto( ret_addr_field, fdef_loc ) ) in
    C.mkStmt( C.If( ret_addr_field, C.mkBlock( [ goto ] ), empty_block, fdef_loc ) )
  in
  let v = new coroutinifyYieldingVisitor
              yielding locals_tbl frame_info is_activity_entry
  in
  let y = C.visitCilFunction ( v :> C.cilVisitor ) yielding in
  let () = y.C.sbody.C.bstmts <- locals_init :: goto_stmt :: y.C.sbody.C.bstmts in
  y

let make_unyielding unyielding frame_info =
  let v = new coroutinifyUnyieldingVisitor unyielding frame_info in
  C.visitCilFunction ( v :> C.cilVisitor ) unyielding

(* XXX Need to replace &fn with &__indirect_fn ... Maybe not ... *)
(* XXX Need to replace fun ptr types w/ our craziness *)

(* For this function definition
 *     rt f( p1, p2, p3 ) { ... }
 * Generate the function for indirect calling:
 * NOTE: This function can be called in three different contexts:
 *   - unyielding mode (indicated by caller == NULL)
 *   - yielding mode prologue (indicated by ret != NULL)
 *   - yielding mode epilogueB (neither of the above cases)
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
    let ( rt, ps, va, attrs ) = getTFunInfo original.C.svar.C.vtype "indir" in
    let () =
      original.C.svar.C.vtype <-
        C.TFun( frame_info.typ_ptr, ps, va, attrs )
    in
    ( match rt with
        C.TVoid _ -> None
      | r -> Some r
    )
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
let examine_activity_t_struct ci frame_info =

  let old =
    let oldr = ref None in
    let fields = [ ( "oldest_frame", oldr ); ] in
    let extract field =
      let find_field ( name, field_ref ) =
        if field.C.fname = name then
          field_ref := Some field
      in
      L.iter find_field fields
    in
    let () = L.iter extract ci.C.cfields in
    match !oldr with
      Some old -> old
    | _ -> E.s( E.error "frame struct missing fields?!?" )
  in
  { frame_info with
    oldest_sel = ( fun e -> ( C.Mem e, C.Field( old, C.NoOffset ) ) );
  }

(* When we find the definition of the generic frame type, examine it and
 * record some stuff. *)
let examine_frame_t_struct ci dummy_var =
  let dummy_lval  = ( C.Var dummy_var, C.NoOffset ) in
  let dummy_exp   = C.zero in
  let dummy_type  = C.voidType in

  let _, s, r, ce, cr =
    let ar, sr, ra, cer, crr =
      ref None, ref None, ref None, ref None, ref None in
    let fields = [ ( "activity", ar ); ( "specific", sr );
                   ( "return_addr", ra ); ( "callee", cer );
                   ( "caller", crr ); ] in
    let extract field =
      let find_field ( name, field_ref ) =
        if field.C.fname = name then
          field_ref := Some field
      in
      L.iter find_field fields
    in
    let () = L.iter extract ci.C.cfields in
    match ( !ar, !sr, !ra, !cer, !crr ) with
      Some a, Some s, Some r, Some ce, Some cr -> a, s, r, ce, cr
    | _ -> E.s( E.error "frame struct missing fields?!?" )
  in
  let specific_cast specific_type frame =
    let t = C.TPtr( specific_type, [(*attrs*)] ) in
    let e = C.AddrOf( C.Mem( C.Lval frame ), C.Field( s, C.NoOffset ) ) in
    C.mkCast e t
  in
  let frame_typ = C.TComp( ci, [(*attrs*)] ) in
  let frame_ptr_typ = C.TPtr( frame_typ, [(*attrs*)] ) in
    (* WARNING: This type has to be the same as crcl(epilogueB_t)
       defined in the runtime library. *)
  let epilogueB_typ =
    C.TPtr( C.TFun( C.voidType,
                    Some[ ( "frame", frame_ptr_typ, [] );
                          ( "data", C.voidPtrType, [] ) ],
                    false, [] ), [] )
  in
  {
    (* generic: *)
    specific_cast   = specific_cast;
    typ             = frame_typ;
    typ_ptr         = frame_ptr_typ;
    epilogueB_typ   = epilogueB_typ;
    caller_sel      = ( fun e -> ( C.Mem e, C.Field( cr,  C.NoOffset ) ) );
    callee_sel      = ( fun e -> ( C.Mem e, C.Field( ce,  C.NoOffset ) ) );
    return_addr_sel = ( fun e -> ( C.Mem e, C.Field( r,   C.NoOffset ) ) );
    oldest_sel      = ( fun e -> ( C.Mem e, C.Field( cr , C.NoOffset ) ) );
    gen_prologue    = dummy_var;
    gen_epilogueA   = dummy_var;
    gen_epilogueB   = dummy_var;
    activate        = dummy_var;
    act_intermed    = dummy_var;
    act_epilogue    = dummy_var;
    yield           = dummy_var;
    yield_impl      = dummy_exp;
    (* function-specific *)
    sizeof_specific = dummy_exp;
    locals_sel      = ( fun e -> dummy_lval );
    return_sel      = ( fun e -> dummy_lval );
    locals_type     = dummy_type;
    yielding        = dummy_var;
    epilogueA       = dummy_var;
    epilogueB       = dummy_var;
    locals          = ( fun _ -> IH.create 0 );
    ret_type        = dummy_type;
    param_types     = [];
    (* context-specific *)
    lval            = dummy_lval;
    exp             = C.zero;
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
  val mutable activity_struct = None

  (* If we have encountered the def of crcl(frame_t), extract its fields. *)
  method fill_in_frame v =
    match ( frame_opt, frame_struct, activity_struct ) with
      ( None, Some fci, Some aci ) ->
      let frame_info = examine_frame_t_struct fci v in
      let frame_info = examine_activity_t_struct aci frame_info in
      let () = frame_opt <- Some( frame_info ) in
      let () = frame_struct <- None in
      let () = activity_struct <- None in
      ()
    | ( None, _, _ ) -> ()
    | ( Some frame_info, None, None ) ->
       let name = v.C.vname in
       let () =
         if name = crcl "fn_generic_prologue" then
           frame_opt <- Some{ frame_info with gen_prologue = v }
         else if name = crcl "fn_generic_epilogueA" then
           frame_opt <- Some{ frame_info with gen_epilogueA = v }
         else if name = crcl "fn_generic_epilogueB" then
           frame_opt <- Some{ frame_info with gen_epilogueB = v }
         else if name = crcl "activate_intermediate" then
           frame_opt <- Some{ frame_info with act_intermed = v }
         else if name = crcl "activity_epilogue" then
           frame_opt <- Some{ frame_info with act_epilogue = v }
         else if name = crcl "yield" then
           let () = if not( type_is_charcoal_fn v.C.vtype ) then
                      add_charcoal_linkage_var v
           in
           frame_opt <- Some{ frame_info with yield = v }
         else if name = crcl "yield_impl" then
           frame_opt <- Some{ frame_info with yield_impl = C.Lval( C.var v ) }
         else if name = crcl "activate" then
           frame_opt <- Some{ frame_info with activate = v }
         else ()
       in
       ()
    | ( Some _, _, _ ) -> E.s( E.error "Too much frame" )

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
       let generic_frame_info =
         match frame_opt with
           Some f -> f
         | None ->
            E.s( E.error "GFun before frame_t def  :(  %s !" orig_name )
       in
       if type_is_charcoal_fn original.C.svar.C.vtype then
         let is_activity_entry =
           let s = "__charcoal_act_" in
           try s = "__charcoal_application_main" ||
               s = String.sub original.C.svar.C.vname 0 ( String.length s )
           with Invalid_argument _ -> false
         in
         (* NOTE: These are modified below. *)
         let( yielding, unyielding, prologue, epilogueA, epilogueB ) =
           make_function_skeletons original orig_name
         in
         let ( tags, specific_frame_info ) =
           make_specific yielding orig_var generic_frame_info
         in
         let frame_info =
           { specific_frame_info with epilogueA = epilogueA.C.svar;
                                      epilogueB = epilogueB.C.svar;
                                      yielding = yielding.C.svar}
         in
         (* "p" before "y" *)
         let () =
           let e f = C.Lval( C.var( f.C.svar ) ) in
           add_fn_translation orig_var ( e unyielding ) ( e prologue ) ( e epilogueB )
         in
         let p  = make_prologue     prologue frame_info yielding.C.sformals in
         let y  = make_yielding     yielding frame_info is_activity_entry in
         let u  = make_unyielding unyielding frame_info in
         let eA = make_epilogueA   epilogueA frame_info in
         let eB = make_epilogueB   epilogueB frame_info in
         let i  = make_indirect     original frame_info in
         let decls = L.map ( fun f -> C.GVarDecl( f.C.svar, loc ) )
                           [ y; u; eA; p; eB; i ]
         in
         let funs = L.map ( fun f -> C.GFun( f, loc ) )
                          [ y; u; eA; p; eB; i ]
         in
         C.ChangeTo ( tags @ decls @ funs )

       else
         (* XXX is this necessary? what about fill_in_frame? *)
(*           let () =
             if crcl_runtime_fn_name = "fn_generic_prologue" then
               frame_opt <-
                 Some{ frame with gen_prologue = orig_var };
             if crcl_runtime_fn_name = "fn_generic_epilogueA" then
               frame_opt <-
                 Some{ frame with gen_epilogueA = orig_var };
             if crcl_runtime_fn_name = "fn_generic_epilogueB" then
               frame_opt <-
                 Some{ frame with gen_epilogueB = orig_var };
           in*)
           C.SkipChildren

    | C.GVarDecl( var, _ ) ->
       let () = self#fill_in_frame var in
       C.DoChildren

    (* We need to find various struct definitions *)
    | C.GCompTag( ci, loc ) ->
       (* let () = Pf.printf "ffv - gct - %s\n" ci.C.cname in *)
       if ci.C.cname = "__charcoal_frame_t" then
         let () = frame_struct <- Some ci in
         C.DoChildren
       else if ci.C.cname = "activity_t" then
         let () = activity_struct <- Some ci in
         C.DoChildren
       else
         C.DoChildren

    | C.GVar( v, _, _ ) ->
       let () = self#fill_in_frame v in
       C.DoChildren

    | _ -> C.SkipChildren
end

class scrub_linkage_visitor = object(self)
  inherit C.nopCilVisitor

  method vtype t =
    match remove_charcoal_linkage_from_type false t with
      None -> C.DoChildren
    | Some t' -> change_do_children t'
end

let do_coroutinify( f : C.file ) =
  let () = C.visitCilFile ( new globals_visitor :> C.cilVisitor ) f in
  let () = C.visitCilFile ( new scrub_linkage_visitor ) f in
  ()

let feature : C.featureDescr =
  { C.fd_name = "coroutinify";
    C.fd_enabled = Cilutil.doCoroutinify;
    C.fd_description = "heap-allocate call frames";
    C.fd_extraopt = [];
    C.fd_doit = do_coroutinify;
    C.fd_post_check = true ;
  }

