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
  | CRCL_FUN_INIT
  | CRCL_FUN_OTHER

let crcl_yielding_prefix   = "__charcoal_fn_yielding_"
let crcl_unyielding_prefix = "__charcoal_fn_unyielding_"
let crcl_init_prefix       = "__charcoal_fn_init_"
let crcl_locals_prefix     = "__charcoal_fn_locals_"
let (crcl_yielding, crcl_unyielding, crcl_init, crcl_locals) =
  let ps = [ crcl_yielding_prefix; crcl_unyielding_prefix;
             crcl_init_prefix; crcl_locals_prefix; ]
  in
  let ps_lens = List.map ( fun p -> ( p, String.length p ) ) ps in
  let comp( p, l ) =
    fun s ->
    let m = ( p = Str.string_before s l ) in
    ( m, if m then Str.string_after s l else "" )
  in
  ( match List.map comp ps_lens with
      [y;u;i;l] -> (y,u,i,l)
    | _ -> E.s( E.error "Unpossible!!!" ) )

let crcl_fun_kind name =
  if fst( crcl_yielding name ) then
    CRCL_FUN_YIELDING
  else if fst( crcl_unyielding name ) then
    CRCL_FUN_UNYIELDING
  else if fst( crcl_init name ) then
    CRCL_FUN_INIT
  else
    CRCL_FUN_OTHER

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
    let i = C.makeVarinfo true (spf "%s%s" crcl_init_prefix       v.C.vname) C.voidType in
    let y = C.makeVarinfo true (spf "%s%s" crcl_yielding_prefix   v.C.vname) C.voidType in
    let u = C.makeVarinfo true (spf "%s%s" crcl_unyielding_prefix v.C.vname) C.voidType in
    let () = IH.replace function_vars v.C.vid (i, y, u) in
    (i, y, u)

class coroutinifyYieldingVisitor fdec locals frame_info free_fn = object(self)
  inherit C.nopCilVisitor

  (* It's tempting to use vvrbl, because we're only interested in uses
   * of locals.  However, we need to replace them with a more complex
   * lval, so we need the method that lets us return lvals. *)
  method vlval( lhost, offset ) =
    match lhost with
      C.Var v ->
      ( match IH.tryfind locals v.C.vid with
          Some e -> change_do_children( C.Mem e, offset )
        | None -> C.DoChildren )
    (* We can ignore the Mem case because we'll get it later in the visit *)
    | _ -> C.DoChildren

  (* It's tempting to use vinstr, because we're only interested in
   * calls.  However, we need to replace them with a sequence that has
   * returns and labels, so we need the method that lets us return
   * statements. *)
  method vstmt s =
    let ( frame_type, frame_var, frame_return_field, frame_locals_field,
          frame_callee_field, frame_caller_field ) =
      frame_info
    in
    let frame_lval = C.var frame_var in
    let frame_exp = C.Lval( frame_lval ) in
    let is_not_call i =
      match i with
        C.Call _ -> false
      | _ -> true
    in
    (* Translate calls from:
     *     lhs = f( p1, p2, p3 );
     * to:
     *     frame->return_pointer = &__charcoal_return_N;
     *     /* allocates callee frame: */
     *     return __charcoal_fn_init_f( frame, p1, p2, p3 );
     *   __charcoal_return_N:
     *     lhs = *( ( return type * ) &( frame->callee->locals ) );
     *     free( frame->callee );
     *     /* TODO: maybe add for debugging: */
     *     frame->callee = 0xJUNK;
     *)
    let coroutinify_call instr =
      match instr with
        C.Call( lhs_opt, fn_exp, params_exps, loc ) ->
        let (i, y, u) = match fn_exp with
            C.Lval( C.Var v, C.NoOffset ) -> fooo v
          | _ -> E.s( E.unimp "Oh noes; indirect calls" )
        in
        let callee_lval = ( C.Mem frame_exp, C.Field( frame_callee_field, C.NoOffset ) ) in
        let return =
          let after =
            let free_callee : C.instr =
              C.Call( None, C.Lval( C.var free_fn ), [ C.Lval callee_lval ], loc ) in
            match lhs_opt with
              Some lhs ->
              let locals_lval =
                ( C.Mem( C.Lval callee_lval ), C.Field( frame_locals_field, C.NoOffset ) )
              in
              let return_type =
                match C.typeOf fn_exp with
                  C.TFun( r, _, _, _ ) -> r
                | _ -> E.s( E.error "Function with non-function type?!?" )
              in
              let lhost = C.Mem( C.mkCast ( C.mkAddrOf locals_lval ) return_type ) in
              [ C.Set( lhs, C.Lval( lhost, C.NoOffset ), loc ); free_callee ]
            | _ -> [ free_callee ]
          in
          let r = C.mkStmt( C.Instr after ) in
          let () = r.C.labels <- [ fresh_return_label loc ] in
          r
        in
        let save_goto =
          let s = C.Set( ( C.Mem frame_exp, C.Field( frame_return_field, C.NoOffset ) ),
                         C.AddrOfLabel( ref return ), loc ) in
          C.mkStmt( C.Instr[ s ] )
        in
        let temp_callee = C.var( C.makeTempVar fdec frame_type ) in
        let init_call =
          let ic = C.Call( Some temp_callee, C.Lval( C.var i ), frame_exp::params_exps, loc ) in
          C.mkStmt( C.Instr[ ic ] )
        in
        let init_return = C.mkStmt( C.Return( Some( C.Lval temp_callee ), loc ) )  in
        [save_goto; init_call; init_return; return]
      | _ -> [C.mkStmt( C.Instr[ instr ] )]
    in

    (* Translate returns from:
     *     return exp;
     * to:
     *     *( ( return type * ) &( frame->locals ) ) = exp;
     *     return frame->caller;
     *)
    let coroutinify_return rval_opt loc =
      let caller_lval = ( C.Mem frame_exp, C.Field( frame_caller_field, C.NoOffset ) ) in
      let return_stmt = C.mkStmt( C.Return( Some( C.Lval caller_lval ), loc ) ) in
      (match rval_opt with
       | None -> change_do_children return_stmt
       | Some rval ->
          let locals_lval =
            ( C.Mem( C.Lval frame_lval ), C.Field( frame_locals_field, C.NoOffset ) )
          in
          let return_type =
            match fdec.C.svar.C.vtype with
              C.TFun( r, _, _, _ ) -> r
            | _ -> E.s( E.error "Function with non-function type?!?" )
          in
          let lhs = ( C.Mem( C.mkCast ( C.mkAddrOf locals_lval ) return_type ), C.NoOffset ) in
          let set_return = C.mkStmt( C.Instr[ C.Set( lhs, rval, loc ) ] ) in
          change_do_children( C.mkStmt( C.Block( C.mkBlock[ set_return; return_stmt ] ) ) ) )
    in

    match s.C.skind with
      (C.Instr instrs) ->
      if List.for_all is_not_call instrs then
        C.DoChildren
      else
        let stmts = List.concat (List.map coroutinify_call instrs) in
        change_do_children( C.mkStmt( C.Block( C.mkBlock stmts ) ) )
    | (C.Return( rval_opt, loc )) -> coroutinify_return rval_opt loc
    | _ -> C.DoChildren
end

(* Translate from:
 *     rt f( p1, p2, p3 ) { ... }
 * to:
 *     frame_p __charcoal_fn_init_f( frame_p caller, p1, p2, p3 )
 *     {
 *        size_t locals_size = sizeof( f struct );
 *        /* using a call here to reduce explosion down: */
 *        frame_p frame = __crcl_generic_init( caller, locals_size, __crcl_fn_yielding_f );
 *        ( f struct ) *locals = &frame->locals;
 *        locals->p1 = p1;
 *        locals->p2 = p2;
 *        locals->p3 = p3;
 *        locals->l1 = init1;
 *        locals->l2 = init2;
 *        locals->l3 = init3;
 *        return frame;
 *     }
 *)
let make_init_version () =
  ()

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

