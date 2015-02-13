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

module C  = Cil
module E  = Errormsg
module IH = Inthash

let spf = Printf.sprintf

let mkSimpleField ci fn ft fl =
  { C.fcomp = ci ; C.fname = fn ; C.ftype = ft ; C.fbitfield = None ; C.fattr = [];
    C.floc = fl }

let change_do_children x = C.ChangeDoChildrenPost( x, fun e -> e )

let label_counter = ref 1
let fresh_return_label loc =
  let n = !label_counter in
  let () = label_counter := n + 1 in
  C.Label( spf "__charcoal_return_%d" n, loc, false )

type crcl_fun_kind =
  | CRCL_FUN_YIELDING
  | CRCL_FUN_UNYIELDING
  | CRCL_FUN_PROLOGUE
  | CRCL_FUN_OTHER

let crcl_yielding_prefix     = "__charcoal_fn_yielding_"
let crcl_unyielding_prefix   = "__charcoal_fn_unyielding_"
let crcl_prologue_prefix     = "__charcoal_fn_prologue_"
let crcl_epilogue_prefix     = "__charcoal_fn_epilogue_"
let crcl_after_return_prefix = "__charcoal_fn_after_return_"
let crcl_locals_prefix       = "__charcoal_fn_locals_"
let (crcl_yielding, crcl_unyielding, crcl_prologue,
     crcl_epilogue, crcl_after_return, crcl_locals) =
  let ps = [ crcl_yielding_prefix; crcl_unyielding_prefix; crcl_prologue_prefix;
             crcl_epilogue_prefix; crcl_after_return_prefix; crcl_locals_prefix; ]
  in
  let ps_lens = List.map ( fun p -> ( p, String.length p ) ) ps in
  let comp( p, l ) =
    fun s ->
    let m = ( p = Str.string_before s l ) in
    ( m, if m then Str.string_after s l else "" )
  in
  ( match List.map comp ps_lens with
      [y;u;p;e;a;l] -> (y,u,p,e,a,l)
    | _ -> E.s( E.error "Unpossible!!!" ) )

let crcl_fun_kind name =
  if fst( crcl_yielding name ) then
    CRCL_FUN_YIELDING
  else if fst( crcl_unyielding name ) then
    CRCL_FUN_UNYIELDING
  else if fst( crcl_prologue name ) then
    CRCL_FUN_PROLOGUE
  else
    CRCL_FUN_OTHER


type frame_info =
    {
      caller_fld : C.fieldinfo;
      callee_fld : C.fieldinfo;
      locals_fld : C.fieldinfo;
      return_fld : C.fieldinfo;
      lval       : C.lval;
      exp        : C.exp;
      typ        : C.typ;
      locals_typ : C.typ;
      yielding   : C.varinfo;
    }

class bar( newname: string ) = object( self )
                                       (*inherit C.copyFunctionVisitor*)
end

class coroutinifyUnyieldingVisitor = object(self)
  inherit C.nopCilVisitor
end

let function_vars = IH.create 42

let fooo (v : C.varinfo) =
  try IH.find function_vars v.C.vid
  with Not_found ->
    (* XXX fix types: *)
    let mvi = C.makeVarinfo true in
    let i = mvi (spf "%s%s" crcl_prologue_prefix   v.C.vname) C.voidType in
    let y = mvi (spf "%s%s" crcl_yielding_prefix   v.C.vname) C.voidType in
    let u = mvi (spf "%s%s" crcl_unyielding_prefix v.C.vname) C.voidType in
    let () = IH.replace function_vars v.C.vid (i, y, u) in
    (i, y, u)


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this type:
 *     union {
 *         struct /* no params or locals -> char[0] */
 *         {
 *             p1;
 *             p2;
 *             p3;
 *             l1;
 *             l2;
 *             l3;
 *         } locals;
 *         rt return_value; /* void -> char[0] */
 *     }
 *)
(* XXX GCompTag??? *)
let make_locals_struct fdec =
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

  let fname = fdec.C.svar in
  let fdef_loc = fname.C.vdecl in
  let unit_type = C.TArray( C.charType, Some( C.zero ), [(*attrs*)] ) in
  let locals_type =
    match fdec.C.sformals @ fdec.C.slocals with
      [] -> unit_type
    | all ->
       let field v =
         let () = sanity v in
         (* TODO: maybe drop if unused *)
         ( v.C.vname, v.C.vtype, None, [(*attrs*)], v.C.vdecl )
       in
       let fields = List.map field all in
       let struct_name = spf "%s%s_%s" crcl_locals_prefix fname.C.vname "struct" in
       let ci = C.mkCompInfo true struct_name ( fun _ -> fields ) [(*attrs*)] in
       let field_select t v =
         let field =
           { C.fcomp = ci; C.fname = v.C.vname; C.ftype = v.C.vtype;
             C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = v.C.vdecl }
         in
         IH.replace t v.C.vid ( fun e -> ( C.Mem e, C.Field( field, C.NoOffset ) ) )
       in
       let locals = IH.create( List.length all ) in
       let () = List.iter field_select in
       C.TComp( ci, [(*attrs*)] )
  in
  let return_type =
    match fname.C.vtype with
      C.TFun( C.TVoid _, _, _, _ ) -> unit_type
    | C.TFun( rt, _, _, _ ) -> rt
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let locals_field = ( "L", locals_type, None, [(*attrs*)], fdef_loc ) in
  let ret_field    = ( "R", return_type, None, [(*attrs*)], fdef_loc ) in
  let fields = [ locals_field; ret_field ] in
  let union_name = spf "%s%s_%s" crcl_locals_prefix fname.C.vname "union" in
  let union = C.mkCompInfo false union_name ( fun _ -> fields ) [(*attrs*)] in
  let locals_field =
    { C.fcomp = union; C.fname = "L"; C.ftype = locals_type;
      C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = fdef_loc }
  in
  let ret_field =
    { C.fcomp = union; C.fname = "R"; C.ftype = ret_type;
      C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = fdef_loc }
  in
  fdec


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this prologue:
 *     frame_p __crcl_fn_prologue_f( frame_p caller, void *ret_ptr, p1, p2, p3 )
 *     {
 *        /* NOTE: Using a call here to keep code size down.  The system is 
 *         * welcome to inline it, if that's a good idea. */
 *        frame_p frame = __crcl_generic_prologue(
 *           sizeof( f struct ), ret_ptr, caller, __crcl_fn_yielding_f );
 *        ( f struct ) *locals = &frame->locals;
 *        locals->p1 = p1;
 *        locals->p2 = p2;
 *        locals->p3 = p3;
 *        /* XXX not totally sure if these inits exist: */
 *        locals->l1 = init1;
 *        locals->l2 = init2;
 *        locals->l3 = init3;
 *        return frame;
 *     }
 *)
let make_prologue original frame generic params =
  let orig_var = original.C.svar in
  let fdef_loc = orig_var.C.vdecl in
  (* XXX function type? *)
  let prologue =
    C.emptyFunction( spf "%s%s" crcl_prologue_prefix orig_var.C.vname )
  in
  let () = C.setFormals prologue original.C.sformals in
  let generic_params =
    let ret_ptr = C.makeFormalVar prologue ~where:"^" "ret_ptr" C.voidPtrType in
    let caller  = C.makeFormalVar prologue ~where:"^" "caller"  frame.typ in
    let ps = List.map ( fun v -> C.Lval( C.var v ) )
                      [ ret_ptr; caller; frame.yielding ]
    in
    ( C.SizeOf frame.locals_typ )::ps
  in
  let this = C.makeLocalVar prologue "frame" frame.typ in
  let call = C.Call( Some( C.var this ), generic, generic_params, fdef_loc ) in
  (* If the original function has zero parameters, skip this *)
  let param_assignments = match original.C.sformals with
      [] -> []
    | fs ->
       let locals =
         let locals_val = ( C.Mem( C.Lval( C.var this ) ),
                            C.Field( frame.locals_fld, C.NoOffset ) )
         in
         let locals_init = C.SingleInit( C.AddrOf locals_val ) in
         let locals_type = C.TPtr( frame.locals_typ, [(*attrs*)] ) in
         C.makeLocalVar prologue "locals" ~init:locals_init locals_type
       in
       let assign_param v =
         let local_var =
           match IH.tryfind params v.C.vid with
             Some f -> f locals
           | _ -> E.s( E.error "Unpossible" )
         in
         C.Set( local_var, C.Lval( C.var v ), fdef_loc )
       in
       List.map assign_param fs
  in
  let instrs = call::param_assignments in
  let body = C.mkBlock[ C.mkStmt( C.Instr instrs ) ] in
  let () = prologue.C.sbody <- body in
  prologue


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this epilogue:
 *     frame_p __crcl_fn_epilogue( frame_p frame, rt v )
 *     {
 *         *( ( return type * ) &( frame->locals ) ) = v;
 *         XXX OR ( ( f struct * ) &( frame->locals ) )->_.ret_val = v;
 *         return __crcl_fn_generic_epilogue( frame );
 *     }
 *)
(* XXX  Mind the generated function's return type *)
let make_epilogue original frame generic =
  let orig_var = original.C.svar in
  let fdef_loc = orig_var.C.vdecl in
  let epilogue =
    C.emptyFunction( spf "%s%s" crcl_epilogue_prefix orig_var.C.vname )
  in
  let return_type =
    match orig_var.C.vtype with
      C.TFun( r, _, _, _ ) -> r
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let this = C.Lval( C.var( C.makeFormalVar epilogue "frame" frame.typ ) ) in
  let caller = C.var( C.makeTempVar epilogue frame.typ ) in
  let instrs =
    let call_instr = C.Call( Some caller, generic, [ frame.exp ], fdef_loc ) in
    match return_type with
      C.TVoid _ -> [ call_instr ]
    | _ ->
       let rval = C.Lval( C.var( C.makeFormalVar epilogue "v" return_type ) ) in
       let locals = ( C.Mem frame.exp, C.Field( frame.locals_fld, C.NoOffset ) ) in
       let locals_cast = C.mkCast ( C.mkAddrOf locals ) return_type in
       [ C.Set( ( C.Mem locals_cast, C.NoOffset ), rval, fdef_loc ); call_instr ]
  in
  let return_stmt = C.mkStmt( C.Return( Some( C.Lval caller ), fdef_loc ) ) in
  let body = C.mkBlock[ C.mkStmt( C.Instr instrs ); return_stmt ] in
  let () = epilogue.C.sbody <- body in
  epilogue


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this after-return:
 *     void __crcl_fn_after_return_f( frame_p frame, rt *lhs )
 *     {
 *         /* sequence is important because generic frees callee */
 *         if( lhs )
 *             *lhs = *((rt * )(&(frame->callee->locals)));
 *         __crcl_fn_generic_after_return( frame );
 *     }
 *)
let make_after_return original frame generic_after =
  let orig_var = original.C.svar in
  let fdef_loc = orig_var.C.vdecl in
  let after_ret =
    C.emptyFunction( spf "%s%s" crcl_after_return_prefix orig_var.C.vname )
  in
  let return_type =
    match orig_var.C.vtype with
      C.TFun( r, _, _, _ ) -> r
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let this =
    let t = C.makeFormalVar after_ret "frame" frame.typ in
    C.Lval( C.var t )
  in
  let lhsp =
    let l = C.makeFormalVar after_ret "lhs"
                            ( C.TPtr( return_type, [(*attrs*)] ) ) in
    C.Lval( C.var l )
  in
  let callee = C.Lval( C.Mem this, C.Field( frame.callee_fld, C.NoOffset ) ) in
  let locals = ( C.Mem callee, C.Field( frame.locals_fld, C.NoOffset ) ) in
  let locals_cast = C.mkCast ( C.mkAddrOf locals ) return_type in
  let lhs = ( C.Mem lhsp, C.NoOffset ) in
  let assign = C.Set( lhs, C.Lval( C.Mem locals_cast, C.NoOffset ), fdef_loc ) in
  let assign_block = C.mkBlock( [ C.mkStmt( C.Instr( [ assign ] ) ) ] ) in
  let empty_block = C.mkBlock( [ C.mkEmptyStmt() ] ) in
  let null_check = C.mkStmt( C.If( lhsp, assign_block, empty_block, fdef_loc ) ) in
  let call_to_generic = C.Call( None, generic_after, [ this ], fdef_loc ) in
  let body = C.mkBlock[ null_check; C.mkStmt( C.Instr[ call_to_generic ] ) ] in
  let () = after_ret.C.sbody <- body in
  after_ret


(* Translate direct calls from:
 *     lhs = f( p1, p2, p3 );
 * to:
 *     return __crcl_fn_prologue_f( frame, &__crcl_return_N, p1, p2, p3 );
 *   __crcl_return_N:
 *     __crcl_fn_after_return_f( frame, &lhs );
 *
 * XXX unimp  Translate indirect calls from:
 *     lhs = exp( p1, p2, p3 );
 * to:
 *     return exp.prologue( frame, &__crcl_return_N, p1, p2, p3 );
 *   __crcl_return_N:
 *     exp.after_return( frame, &lhs );
 *)
let coroutinify_call fdec frame instr =
  match instr with
    C.Call( lhs_opt, fn_exp, params, loc ) ->
    let (prologue, after_return, _) = match fn_exp with
        C.Lval( C.Var v, C.NoOffset ) -> fooo v
      | _ -> E.s( E.unimp "Oh noes; indirect calls" )
    in
    let after_call =
      let params =
        match ( C.typeOf fn_exp, lhs_opt ) with
          ( C.TFun( C.TVoid _, _, _, _ ), _ ) -> [ frame.exp ]
        | ( C.TFun _, None )                  -> [ frame.exp; C.zero ]
        | ( C.TFun _, Some lhs )              -> [ frame.exp; C.AddrOf lhs ]
        | _ -> E.s( E.error "Function with non-function type?!?" )
      in
      let after_instr = C.Call( None, C.Lval( C.var after_return ), params, loc ) in
      let r = C.mkStmt( C.Instr[ after_instr ] ) in
      let () = r.C.labels <- [ fresh_return_label loc ] in
      r
    in
    let callee = C.var( C.makeTempVar fdec frame.typ ) in
    let ps = frame.exp::( C.AddrOfLabel( ref after_call ) )::params in
    let prologue_call =
      C.mkStmt( C.Instr[ C.Call( Some callee, C.Lval( C.var prologue ), ps, loc ) ] )
    in
    let prologue_return = C.mkStmt( C.Return( Some( C.Lval callee ), loc ) )  in
    [prologue_call; prologue_return; after_call]

  (* Anything other than a call *)
  | _ -> [ C.mkStmt( C.Instr[ instr ] ) ]


(* Translate returns from:
 *     return exp;
 * to:
 *     return __crcl_fn_epilogue_f( frame, exp );
 *)
let coroutinify_return fdec rval_opt loc frame epilogue =
  let caller = C.var( C.makeTempVar fdec frame.typ ) in
  let params = match rval_opt with
      None ->   [ frame.exp ]
    | Some e -> [ frame.exp; e ]
  in
  let call = C.mkStmt( C.Instr[ C.Call( Some caller, epilogue, params, loc ) ] ) in
  let return_stmt = C.mkStmt( C.Return( Some( C.Lval caller ), loc ) ) in
  change_do_children( C.mkStmt( C.Block( C.mkBlock[ call; return_stmt ] ) ) )


class coroutinifyYieldingVisitor fdec locals frame epilogue = object(self)
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
      ( match IH.tryfind locals v.C.vid with
          Some l -> change_do_children l
        | None -> C.DoChildren )
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
      if List.for_all is_not_call instrs then
        C.DoChildren
      else
        let coroutinify_call_here = coroutinify_call fdec frame in
        let stmts = List.concat( List.map coroutinify_call_here instrs ) in
        change_do_children( C.mkStmt( C.Block( C.mkBlock stmts ) ) )

    | (C.Return( rval_opt, loc )) ->
       coroutinify_return fdec rval_opt loc frame epilogue

    | _ -> C.DoChildren
end

let make_yielding_version fdec =
  let new_name = spf "%s%s" crcl_yielding_prefix fdec.C.svar.C.vname in
  let yielding_version = C.copyFunction fdec new_name in
  let fields =
    let f v = (v.C.vname, v.C.vtype, None (* bitfield *), [(*attrs*)], v.C.vdecl) in
    (List.map f fdec.C.sformals) @ (List.map f fdec.C.slocals)
  in
  let struct_name = spf "%s%s" crcl_locals_prefix fdec.C.svar.C.vname in
  let locals_struct = C.mkCompInfo true struct_name (fun _ -> fields) [(* attrs *)] in
  let locals_type = C.TPtr( C.TComp( locals_struct, [(*attrs*)] ), [(*attrs*)] ) in
  let locals_init =
    C.SingleInit( C.SizeOf locals_type (* XXX & ( *frame . locals ) *) ) in
  let locals_var =
    C.makeLocalVar yielding_version "locals" ~init:locals_init locals_type
  in
  let locals_tbl = IH.create (List.length fields) in
  let () =
    let make_field v =
      (* XXX don't lose inits *)
      let f =
        { C.fcomp     = locals_struct;
          C.ftype     = v.C.vtype;
          C.fname     = v.C.vname;
          C.fbitfield = None;
          C.fattr     = [];
          C.floc      = v.C.vdecl }
      in
      let l = C.AddrOf( C.Mem( C.Lval( C.var locals_var ) ),
                        C.Field( f, C.NoOffset ) )
      in
      IH.replace locals_tbl v.C.vid l
    in
    List.iter make_field ( fdec.C.sformals @ fdec.C.slocals )
  in
  yielding_version

class findFunctionsVisitor = object(self)
  inherit C.nopCilVisitor
  method vglob g =
    (* XXX Somewhere. make sure it's big enough for the return value too: *)
    (* XXX somewhere in here for prologue
      ( C.Mem( C.Lval( C.var locals ) ), C.Field( f, C.NoOffset ) ) *)
    match g with
    | C.GFun( fdec, loc ) ->
       let kind = crcl_fun_kind fdec.C.svar.C.vname in
       (match kind with
          CRCL_FUN_OTHER ->
          let xxx = make_yielding_version fdec in
          change_do_children [g]
        | _ -> C.SkipChildren
       )

    (* We need to find various struct definitions *)
    | C.GCompTag( compinfo, location ) ->
       (* XXX NO NO NO. We make the locals *)
       let (is_locals, locals_name) = crcl_locals compinfo.C.cname in
       C.SkipChildren
       (*  if crcl_yielding_prefix = Str.string_before name crcl_yielding_prefix_len then*)

    | _ -> C.SkipChildren
end

let do_coroutinify( f : C.file ) =
  let () = C.visitCilFile (new findFunctionsVisitor) f in
  ()

let feature : C.featureDescr =
  { C.fd_name = "coroutinify";
    C.fd_enabled = Cilutil.doCoroutinify;
    C.fd_description = "heap-allocate call frames";
    C.fd_extraopt = [];
    C.fd_doit = do_coroutinify;
    C.fd_post_check = true ;
  }

