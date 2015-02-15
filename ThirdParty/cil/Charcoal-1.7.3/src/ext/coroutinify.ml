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

let unyielding_pfx   = "__charcoal_fn_unyielding_"
let locals_pfx       = "__charcoal_fn_locals_"
let specific_pfx     = "__charcoal_fn_specific_"
let prologue_pfx     = "__charcoal_fn_prologue_"
let yielding_pfx     = "__charcoal_fn_yielding_"
let epilogue_pfx     = "__charcoal_fn_epilogue_"
let after_return_pfx = "__charcoal_fn_after_return_"
let (crcl_unyielding, crcl_locals, crcl_specific, crcl_prologue,
     crcl_yielding, crcl_epilogue, crcl_after_return) =
  let ps = [ unyielding_pfx; locals_pfx; specific_pfx;
             prologue_pfx; yielding_pfx; epilogue_pfx; after_return_pfx; ]
  in
  let ps_lens = List.map ( fun p -> ( p, String.length p ) ) ps in
  let comp( p, l ) =
    fun s ->
    let m = ( p = Str.string_before s l ) in
    ( m, if m then Str.string_after s l else "" )
  in
  ( match List.map comp ps_lens with
      [u;l;s;p;y;a;e] -> (u,l,s,p,y,a,e)
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
      specific_cast : C.typ -> C.varinfo -> C.exp;
      caller_fld    : C.fieldinfo;
      callee_sel    : C.exp -> C.lval;
      locals_fld    : C.fieldinfo;
      return_fld    : C.fieldinfo;
      lval          : C.lval;
      exp           : C.exp;
      typ           : C.typ;
      locals_typ    : C.typ;
      yielding      : C.varinfo;
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
    let i = mvi (spf "%s%s" prologue_pfx   v.C.vname) C.voidType in
    let y = mvi (spf "%s%s" yielding_pfx   v.C.vname) C.voidType in
    let u = mvi (spf "%s%s" unyielding_pfx v.C.vname) C.voidType in
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
 *         } L;
 *         rt R; /* void -> char[0] */
 *     }
 *)
(* XXX GCompTag??? *)
let make_specific fdec specific_ptr =
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
  let ( locals_type, locals, tags ) =
    match fdec.C.sformals @ fdec.C.slocals with
      [] -> ( unit_type, IH.create 0, [] )
    | all ->
       let field v =
         let () = sanity v in
         ( v.C.vname, v.C.vtype, None, [(*attrs*)], v.C.vdecl )
       in
       let used_vars = List.filter ( fun v -> v.C.vreferenced ) all in
       let fields = List.map field used_vars in
       let struct_name = spf "%s%s" locals_pfx fname.C.vname in
       let ci = C.mkCompInfo true struct_name ( fun _ -> fields ) [(*attrs*)] in
       let field_select t v =
         let field =
           { C.fcomp = ci; C.fname = v.C.vname; C.ftype = v.C.vtype;
             C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = v.C.vdecl }
         in
         IH.replace t v.C.vid ( fun e o -> ( C.Mem e, C.Field( field, o ) ) )
       in
       let locals = IH.create( List.length used_vars ) in
       let () = List.iter ( field_select locals ) used_vars in
       ( C.TComp( ci, [(*attrs*)] ), locals, [ C.GCompTag( ci, fdef_loc ) ] )
  in
  let return_type =
    match fname.C.vtype with
      C.TFun( C.TVoid _, _, _, _ ) -> unit_type
    | C.TFun( rt, _, _, _ ) -> rt
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let specific =
    let f n t = ( n, t, None, [(*attrs*)], fdef_loc ) in
    let fields = [ f "L" locals_type; f "R" return_type ] in
    let specific_name = spf "%s%s" specific_pfx fname.C.vname in
    C.mkCompInfo false specific_name ( fun _ -> fields ) [(*attrs*)]
  in
  let select name ftype =
    let specific_foo = specific_ptr( C.TComp( specific, [(*attrs*)] ) ) in
    let field =
      { C.fcomp = specific; C.fname = name; C.ftype = ftype;
        C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = fdef_loc }
    in
    fun frame ->
      ( C.Mem( specific_foo frame ), C.Field( field, C.NoOffset ) )
  in
  let specific_tag = C.GCompTag( specific, fdef_loc ) in
  ( locals_type,
    tags @ [ specific_tag ],
    select "L" locals_type,
    select "R" return_type,
    locals )


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this prologue:
 *     frame_p __prologue_f( frame_p caller, void *ret_ptr, p1, p2, p3 )
 *     {
 *        /* NOTE: Using a call here to keep code size down.  The system is 
 *         * welcome to inline it, if that's a good idea. */
 *        frame_p frame = __generic_prologue(
 *           sizeof( __specific_f ), ret_ptr, caller, __yielding_f );
 *        ( __specific_f ) *locals = &locals_cast( frame );
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
    C.emptyFunction( spf "%s%s" prologue_pfx orig_var.C.vname )
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
       let ( locals, locals_init ) =
         let locals_val = ( C.Mem( C.Lval( C.var this ) ),
                            C.Field( frame.locals_fld, C.NoOffset ) )
         in
         let locals_type = C.TPtr( frame.locals_typ, [(*attrs*)] ) in
         let locals = C.makeLocalVar prologue "locals" locals_type in
         let locals_init = C.Set( C.var locals, C.AddrOf locals_val, fdef_loc ) in
         ( locals, locals_init )
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
 *     frame_p __epilogue( frame_p frame, rt v )
 *     {
 *         return_cast( frame ) = v;
 *         return __generic_epilogue( frame );
 *     }
 *)
(* XXX  Mind the generated function's return type *)
let make_epilogue original frame generic =
  let orig_var = original.C.svar in
  let fdef_loc = orig_var.C.vdecl in
  let epilogue =
    C.emptyFunction( spf "%s%s" epilogue_pfx orig_var.C.vname )
  in
  let return_type =
    match orig_var.C.vtype with
      C.TFun( r, _, _, _ ) -> r
    | _ -> E.s( E.error "Function with non-function type?!?" )
  in
  let this = C.Lval( C.var( C.makeFormalVar epilogue "frame" frame.typ ) ) in
  let caller = C.var( C.makeTempVar epilogue frame.typ ) in
  let instrs =
    let call_instr = C.Call( Some caller, generic, [ this ], fdef_loc ) in
    match return_type with
      C.TVoid _ -> [ call_instr ]
    | _ ->
       let rval = C.Lval( C.var( C.makeFormalVar epilogue "v" return_type ) ) in
       let locals = ( C.Mem this, C.Field( frame.locals_fld, C.NoOffset ) ) in
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
 *     void __after_return_f( frame_p frame, rt *lhs )
 *     {
 *         /* sequence is important because generic frees the callee */
 *         if( lhs )
 *             *lhs = return_cast( frame );
 *         __generic_after_return( frame );
 *     }
 *)
let make_after_return original frame generic_after =
  let orig_var = original.C.svar in
  let fdef_loc = orig_var.C.vdecl in
  let after_ret =
    C.emptyFunction( spf "%s%s" after_return_pfx orig_var.C.vname )
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
  let callee = C.Lval( frame.callee_sel this ) in
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
 *     return __prologue_f( frame, &__return_N, p1, p2, p3 );
 *   __return_N:
 *     __after_return_f( frame, &lhs );
 *
 * XXX unimp  Translate indirect calls from:
 *     lhs = exp( p1, p2, p3 );
 * to:
 *     return exp.prologue( frame, &__return_N, p1, p2, p3 );
 *   __return_N:
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
 *     return __epilogue_f( frame, exp );
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
let make_yielding fdec =
  let new_name = spf "%s%s" yielding_pfx fdec.C.svar.C.vname in
  let yielding_version = C.copyFunction fdec new_name in
  let fields =
    let f v = (v.C.vname, v.C.vtype, None (* bitfield *), [(*attrs*)], v.C.vdecl) in
    (List.map f fdec.C.sformals) @ (List.map f fdec.C.slocals)
  in
  let struct_name = spf "%s%s" locals_pfx fdec.C.svar.C.vname in
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

let extract_frame_fields ci dummy_var =
  let a = ref None in
  let s = ref None in
  let cr = ref None in
  let ce = ref None in
  let extract f =
    if f.C.fname == "activity" then
      a := Some f
    else if f.C.fname == "specific" then
      s := Some f
    else if f.C.fname == "caller" then
      cr := Some f
    else if f.C.fname == "callee" then
      ce := Some f
    else
      ()
  in
  let () = List.iter extract ci.C.cfields in
  match ( !a, !s, !cr, !ce ) with
    Some a, Some s, Some cr, Some ce ->
    let dummy_field = a in
    let dummy_lval  = ( C.Var dummy_var, C.NoOffset ) in
    let dummy_exp   = C.zero in
    let dummy_type  = C.voidType in
    let specific_cast specific_type frame =
      let t = C.TPtr( specific_type, [(*attrs*)] ) in
      let e = C.AddrOf( C.Mem( C.Lval( C.var frame ) ), C.Field( s, C.NoOffset ) )
      in
      C.mkCast e t
    in
    {
      specific_cast = specific_cast;
      caller_fld    = cr;
      callee_sel    = ( fun e -> dummy_lval );
      locals_fld    = dummy_field;
      return_fld    = dummy_field;
      lval          = dummy_lval;
      exp           = dummy_exp;
      typ           = C.TComp( ci, [(*attrs*)] );
      locals_typ    = dummy_type;
      yielding      = dummy_var;
    }
  | _ -> E.s( E.error "frame struct missing fields?!?" )

class findFunctionsVisitor = object(self)
  inherit C.nopCilVisitor

  val mutable frame_opt = None
  val mutable dummy_var = None

  method vglob g =
    (* XXX somewhere in here for prologue
      ( C.Mem( C.Lval( C.var locals ) ), C.Field( f, C.NoOffset ) ) *)
    match g with
    | C.GFun( fdec, loc ) ->
       let kind = crcl_fun_kind fdec.C.svar.C.vname in
       (match (kind, frame_opt) with
          ( CRCL_FUN_OTHER, None ) -> E.s( E.error "No specific field?!?" )
        | ( CRCL_FUN_OTHER, Some frameinfo ) ->
          let ( locals_type, tags, locals_cast, ret_cast, locals_table ) =
            make_specific fdec frameinfo.specific_cast
          in
          let p = make_prologue fdec frameinfo in (* generic params =*)
          let a = make_after_return fdec frameinfo in (* generic_after*)
          let e = make_epilogue fdec frameinfo in (*  generic =*)
          let xxx = make_yielding fdec in
          change_do_children [g]
        | _ -> C.SkipChildren
       )

    (* We need to find various struct definitions *)
    | C.GCompTag( ci, loc ) ->
       if ci.C.cname == "__charcoal_frame_t" then
         match dummy_var with
           None -> E.s( E.error "Must be var?!?" )
         | Some v ->
            let () = frame_opt <- Some( extract_frame_fields ci v ) in
            C.DoChildren
       else
         C.DoChildren

    | GVarDecl( v, _ ) ->
       let () = dummy_var <- Some v in
       C.DoChildren

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

