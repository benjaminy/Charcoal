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

let mkSimpleField ci fn ft fl =
  { C.fcomp = ci ; C.fname = fn ; C.ftype = ft ; C.fbitfield = None ; C.fattr = [];
    C.floc = fl }

let change_do_children_no_post x = C.ChangeDoChildrenPost( x, fun e -> e )

let label_counter = ref 1
let fresh_return_label loc =
  let n = !label_counter in
  let () = label_counter := n + 1 in
  C.Label( Printf.sprintf "__charcoal_return_%d" n, loc, false )

type crcl_fun_kind =
  | CRCL_FUN_YIELDING
  | CRCL_FUN_UNYIELDING
  | CRCL_FUN_INIT
  | CRCL_FUN_OTHER

let crcl_yielding_prefix   = "__charcoal_fn_yielding_"
let crcl_unyielding_prefix = "__charcoal_fn_unyielding_"
let crcl_init_prefix       = "__charcoal_fn_init_"
let crcl_yielding_prefix_len   = String.length crcl_yielding_prefix
let crcl_unyielding_prefix_len = String.length crcl_unyielding_prefix
let crcl_init_prefix_len       = String.length crcl_init_prefix

let crcl_fun_kind name =
  if crcl_yielding_prefix = Str.string_before name crcl_yielding_prefix_len then
    CRCL_FUN_YIELDING
  else if crcl_unyielding_prefix = Str.string_before name crcl_unyielding_prefix_len then
    CRCL_FUN_UNYIELDING
  else if crcl_init_prefix = Str.string_before name crcl_init_prefix_len then
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
       let i = C.makeVarinfo true (crcl_init_prefix       ^ v.C.vname) C.voidType in
       let y = C.makeVarinfo true (crcl_yielding_prefix   ^ v.C.vname) C.voidType in
       let u = C.makeVarinfo true (crcl_unyielding_prefix ^ v.C.vname) C.voidType in
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
          Some e -> change_do_children_no_post( C.Mem e, offset )
        | None -> C.DoChildren )
    (* We can ignore the Mem case because we'll get it later in the visit *)
    | _ -> C.DoChildren

  (* It's tempting to use vinstr, because we're only interested in
   * calls.  However, we need to replace them with a sequence that has
   * returns and labels, so we need the method that lets us return
   * statements. *)
  method vstmt s =
    let ( frame_type, frame_var, frame_return_field, frame_locals_field, frame_callee_field ) =
      frame_info
    in
    let is_not_call i =
      match i with
        C.Call _ -> false
      | _ -> true
    in
    (* Translate calls from:
     *   lhs = f( p1, p2, p3 );
     * to:
     *   frame->return_pointer = &__charcoal_return_N;
     *   return __charcoal_fn_init_f( frame, p1, p2, p3 );
     *   __charcoal_return_N:
     *   lhs = *( ( return type * ) &( frame->callee->locals ) );
     *   free( frame->callee );
     *   frame->callee = 0xJUNK; /* TODO: maybe add for debugging */
     *)
    let coroutinify_call instr =
      match instr with
        C.Call( lhs_opt, fn_exp, params_exps, loc ) ->
        let (i, y, u) = match fn_exp with
            C.Lval( C.Var v, C.NoOffset ) -> fooo v
          | _ -> E.s( E.unimp "Oh noes; indirect calls" )
        in
        let frame_exp = C.Lval( C.var frame_var ) in
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
              let lh = C.Mem( C.mkCast ( C.mkAddrOf locals_lval ) return_type ) in
              [ C.Set( lhs, C.Lval( lh, C.NoOffset ), loc ); free_callee ]
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
      | _ -> [C.mkStmt( Instr[ instr ] )]
    in
    match s.skind with
      (Instr instrs) ->
      if List.for_all is_not_call instrs then
        C.DoChildren
      else
        let stmts = List.concat (List.map coroutinify_call instrs) in
        C.DoChildren
    | _ -> C.DoChildren
end

class findFunctionsVisitor = object(self)
  inherit C.nopCilVisitor
  method vglob g =
    match g with
    | GFun( fdec, loc ) ->
       let kind = crcl_fun_kind fdec.C.svar.C.vname in
       (match kind with
          CRCL_FUN_OTHER ->
          let yielding_version = C.copyFunction fdec in
          change_do_children_no_post [g]
        | _ -> C.SkipChildren
       )
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

