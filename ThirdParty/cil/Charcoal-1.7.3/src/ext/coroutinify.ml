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

(* Module and simple alias stuff *)
module L  = List
module C  = Cil
module E  = Errormsg
module H  = Hashtbl
module IH = Inthash
module Pf = Printf
module T  = Trace
module P  = Pretty
open Util
let spf = Printf.sprintf
let trc = T.trace "coroutinify"
let change_do_children x = C.ChangeDoChildrenPost( x, fun e -> e )
let var2exp v = C.Lval( C.var v )

module OrderedBuiltin = struct
  type t = string * int
  let compare ( n1, _ ) ( n2, _ ) = compare n1 n2
end
module BS = Set.Make( OrderedBuiltin )

let label_counter = ref 1
let fresh_return_label loc =
  let n = !label_counter in
  let () = label_counter := n + 1 in
  C.Label( spf "__charcoal_return_%d" n, loc, false )

let getTFunInfo t msg =
  match t with
    C.TFun( r, ps, v, atts ) -> ( r, ps, v, atts )
  | _ -> E.s( E.error "Expecting function type; %s" msg )

let getTFunInfoP t msg =
  match t with
    C.TFun( r, ps, v, atts ) -> ( r, ps, v, atts )
  | C.TPtr( C.TFun( r, ps, v, atts ), _ ) -> ( r, ps, v, atts )
  | _ -> E.s( E.error "Expecting function type; %s" msg )

let crcl s = "__charcoal_" ^ s

let charcoal_pfx  = crcl ""
let no_yield_pfx  = crcl "fn_no_yield_"
let specific_pfx  = crcl "fn_specific_"
let prologue_pfx  = crcl "fn_prologue_"
let yielding_pfx  = crcl "fn_yielding_"
let indirect_pfx  = crcl "fn_indirect_"
let charcoal_pfx_len = String.length charcoal_pfx
let charcoal_pfx_regex = Str.regexp charcoal_pfx

let internal_uid = ref 11
let internal_uid_gen () =
  let x = !internal_uid in
  let () = internal_uid := x + 1 in
  x
let gen_prologue_uid  = internal_uid_gen ()
let gen_epilogue_uid  = internal_uid_gen ()
let act_intermed_uid  = internal_uid_gen ()
let mode_test_uid     = internal_uid_gen ()
let self_activity_uid = internal_uid_gen ()
let act_epilogue_uid  = internal_uid_gen ()
let activity_wait_uid = internal_uid_gen ()
let act_wait_done_uid = internal_uid_gen ()
let yield_uid         = internal_uid_gen ()
let yield_impl_uid    = internal_uid_gen ()
let activate_uid      = internal_uid_gen ()
let alloca_uid        = internal_uid_gen ()
let alloca_impl_uid   = internal_uid_gen ()
let setjmp_uid        = internal_uid_gen ()
let setjmp_c_uid      = internal_uid_gen ()
let setjmp_yield_uid  = internal_uid_gen ()
let longjmp_uid       = internal_uid_gen ()
let longjmp_c_uid     = internal_uid_gen ()
let longjmp_yield_uid = internal_uid_gen ()
let longjmp_no_uid    = internal_uid_gen ()

let builtin_uids : ( string, int ) H.t = H.create 20
let () = L.iter ( fun ( x, y ) -> H.add builtin_uids x y )
[
    ( crcl "fn_generic_prologue" ,     gen_prologue_uid );
    ( crcl "fn_generic_epilogue" ,     gen_epilogue_uid );
    ( crcl "activate_intermediate",    act_intermed_uid );
    ( crcl "yielding_mode",            mode_test_uid );
    ( "self_activity",                 self_activity_uid );
    ( crcl "activity_epilogue",        act_epilogue_uid );
    ( crcl "activity_wait",            activity_wait_uid );
    ( crcl "activity_waiting_or_done", act_wait_done_uid );
    ( crcl "yield",                    yield_uid );
    ( crcl "yield_impl",               yield_impl_uid );
    ( crcl "activate",                 activate_uid );
    ( "alloca",                        alloca_uid );
    ( crcl "alloca",                   alloca_impl_uid );
    ( "setjmp",                        setjmp_uid );
    ( crcl "setjmp_c",                 setjmp_c_uid );
    ( crcl "setjmp_yielding",          setjmp_yield_uid );
    ( "longjmp",                       longjmp_uid );
    ( crcl "longjmp_c",                longjmp_c_uid );
    ( crcl "longjmp_yielding",         longjmp_yield_uid );
    ( crcl "longjmp_no_yield",         longjmp_no_uid );
]

let builtins = IH.create 20
let find_builtin = IH.find builtins

let gen_prologue () = find_builtin gen_prologue_uid
let gen_epilogue () = find_builtin gen_epilogue_uid
let act_intermed () = find_builtin act_intermed_uid
let mode_test    () = find_builtin mode_test_uid
let self_activity() = find_builtin self_activity_uid
let act_epilogue () = find_builtin act_epilogue_uid
let activity_wait() = find_builtin activity_wait_uid
let act_wait_done() = find_builtin act_wait_done_uid
let yield        () = find_builtin yield_uid
let yield_impl   () = find_builtin yield_impl_uid
let activate     () = find_builtin activate_uid
let alloca       () = find_builtin alloca_uid
let alloca_impl  () = find_builtin alloca_impl_uid
let setjmp       () = find_builtin setjmp_uid
let setjmp_yield () = find_builtin setjmp_yield_uid
let setjmp_c     () = find_builtin setjmp_c_uid
let longjmp      () = find_builtin longjmp_uid
let longjmp_c    () = find_builtin longjmp_c_uid
let longjmp_yield() = find_builtin longjmp_yield_uid
let longjmp_no   () = find_builtin longjmp_no_uid

let gen_prologue_e  = var2exp -| gen_prologue
let gen_epilogue_e  = var2exp -| gen_epilogue
let act_intermed_e  = var2exp -| act_intermed
let mode_test_e     = var2exp -| mode_test
let self_activity_e = var2exp -| self_activity
let act_epilogue_e  = var2exp -| act_epilogue
let activity_wait_e = var2exp -| activity_wait
let act_wait_done_e = var2exp -| act_wait_done
let yield_e         = var2exp -| yield
let yield_impl_e    = var2exp -| yield_impl
let activate_e      = var2exp -| activate
let alloca_e        = var2exp -| alloca
let alloca_impl_e   = var2exp -| alloca_impl
let setjmp_e        = var2exp -| setjmp
let setjmp_c_e      = var2exp -| setjmp_c
let setjmp_yield_e  = var2exp -| setjmp_yield
let longjmp_e       = var2exp -| longjmp
let longjmp_c_e     = var2exp -| longjmp_c
let longjmp_yield_e = var2exp -| longjmp_yield
let longjmp_no_e    = var2exp -| longjmp_no

type frame_info =
  {
    (* generic type/struct info: *)
    specific_cast   : C.typ -> C.lval -> C.exp;
    typ             : C.typ;
    typ_ptr         : C.typ;
    activity_sel    : C.exp -> C.lval;
    caller_sel      : C.exp -> C.lval;
    callee_sel      : C.exp -> C.lval;
    return_addr_sel : C.exp -> C.lval;
    (* function-specific: *)
    specific_type   : C.typ;
    specific_sel    : C.lval -> C.exp;
    lhs_sel         : C.exp -> C.lval;
    specifics       : C.exp -> ( ( C.offset -> C.lval ) IH.t );
    yielding        : C.varinfo;
    ret_type        : C.typ;
    formals         : ( string * C.typ * C.attributes * C.varinfo ) list;
    vararg          : bool;
    attrs           : C.attributes;
    (* context-specific: *)
    lval            : C.lval;
    exp             : C.exp;
  }

let crcl_fun_decls : ( C.varinfo * C.varinfo * C.varinfo ) IH.t = IH.create 42
let crcl_fun_defs : frame_info IH.t = IH.create 42

let lookup_fun_decl_var v = IH.tryfind crcl_fun_decls v.C.vid
let lookup_fun_decl v = opt_map ( map3 var2exp ) ( lookup_fun_decl_var v )
let add_fun_decl v u p i = IH.replace crcl_fun_decls v.C.vid ( u, p, i )

let lookup_fun_def v = IH.tryfind crcl_fun_defs v.C.vid
let add_fun_def v = IH.replace crcl_fun_defs v.C.vid

let remove_charcoal_linkage_from_attrs expect_crcl attrs =
  let is_crcl attr = attr = C.Attr( "linkage_charcoal", [] ) in
  ( match L.partition is_crcl attrs with
      ( _::_, others ) -> Some others
    | _ -> if expect_crcl then
             E.s( E.error "Linkage angry?!?" )
           else
             None )

let remove_charcoal_linkage_from_type expect_crcl t =
  match t with
    C.TFun( rt, ps, va, attrs ) ->
    opt_map ( fun others -> C.TFun( rt, ps, va, others ) )
            ( remove_charcoal_linkage_from_attrs expect_crcl attrs )
  | _ -> None

let remove_charcoal_linkage_var v =
  match remove_charcoal_linkage_from_type true v.C.vtype with
    Some t' -> v.C.vtype <- t'
  | _ -> E.s( E.error "Remove linkage from non-function?!?" )

let remove_charcoal_linkage fdec = remove_charcoal_linkage_var fdec.C.svar

let add_charcoal_linkage_var v =
  let ( rt, ps, va, attrs ) = getTFunInfo v.C.vtype "Add linkage?!?" in
  v.C.vtype <- C.TFun( rt, ps, va, C.Attr( "linkage_charcoal", [] )::attrs )

let add_charcoal_linkage fdec = add_charcoal_linkage_var fdec.C.svar

let type_is_charcoal_fn ty =
  match ty with
    C.TFun( _, _, _, attrs )
  | C.TPtr( C.TFun( _, _, _, attrs ), _) -> C.linkage_charcoal attrs
  | _ -> false

let exp_is_charcoal_fn exp =
  type_is_charcoal_fn( C.typeOf exp )
  || match exp with
       C.Lval( C.Var v, C.NoOffset ) -> IH.mem crcl_fun_decls v.C.vid
     | _ -> false

let clear_formals_locals f return_type =
  let () = C.setFormals f [] in
  let () = f.C.slocals <- [] in
  C.setFunctionType f ( C.TFun( return_type, Some [], false, [] ) )

(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this type:
 *     struct
 *     {
 *     #if return type is not void
 *         rt *lhs;
 *     #endif
 *         p1;
 *         p2;
 *         p3;
 *         l1;
 *         l2;
 *         l3;
 *     };
 *)
let make_specific fdec fname frame_info =
  (* let () = trc( P.dprintf "SPECIFIC %s\n" fdec.C.svar.C.vname ) in *)
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

  let ( return_type, orig_ps, va, attrs ) = getTFunInfo fname.C.vtype "FOO" in
  let formals =
    try
      List.map ( fun( ( w, x, y ), z ) -> ( w, x, y, z ) )
               ( List.combine ( opt_default [] orig_ps ) fdec.C.sformals )
    with Invalid_argument _ ->
      E.s( E.bug "Too many or too few formals" )
  in
  let frame_with_fun_info =
    { frame_info with ret_type = return_type; formals = formals;
                      vararg = va; attrs = attrs; }
  in

  let all_vars =
    L.filter ( fun v -> (* XXX always false??? v.C.vreferenced *) true )
             ( fdec.C.sformals @ fdec.C.slocals )
  in
  match return_type, all_vars with
    (* special case when the specific struct is empty; It's illegal to
     * have a struct with no fields. *)
    C.TVoid _, [] -> ( [], frame_with_fun_info )

  | rt, all ->
     let field v = sanity v; ( v.C.vname, v.C.vtype, None, [], v.C.vdecl ) in
     let fields =
       let fields_no_lhs = L.map field all in
       match rt with
         C.TVoid _ -> fields_no_lhs
       | _ ->
          let lhs = ( crcl "lhs", C.TPtr( rt, [] ), None, [], fdef_loc ) in
          lhs::fields_no_lhs
     in
     let struct_name = spf "%s%s" specific_pfx fname.C.vname in
     let ci = C.mkCompInfo true struct_name ( fun _ -> fields ) [] in
     let specific_type = C.TComp( ci, [(*attrs*)] ) in
     let specific_sel = frame_info.specific_cast specific_type in
     let specific_tag = C.GCompTag( ci, fdef_loc ) in

     (* Given exp, a pointer to a specifics struct, map variable x to exp->x *)
     let lhs_sel =
       match rt with
         C.TVoid _ -> fun p -> ( C.Mem p, C.NoOffset )
       | _ ->
          let field =
            { C.fcomp = ci; C.fname = crcl "lhs"; C.ftype = C.TPtr( rt, [] );
              C.fbitfield = None; C.fattr = []; C.floc = fdef_loc }
          in
          fun p -> ( C.Mem p, C.Field( field, C.NoOffset ) )
     in
     let specifics_gen specifics_ptr =
       let specifics_tbl = IH.create( L.length all ) in
       let field_select v =
         let field =
           { C.fcomp = ci; C.fname = v.C.vname; C.ftype = v.C.vtype;
             C.fbitfield = None; C.fattr = []; C.floc = v.C.vdecl }
         in
         IH.replace specifics_tbl v.C.vid
                    ( fun o -> ( C.Mem specifics_ptr, C.Field( field, o ) ) )
       in
       let () = L.iter field_select all in
       specifics_tbl
     in
     let updated_frame_info =
       { frame_with_fun_info with
         specific_type   = specific_type;
         specific_sel    = specific_sel;
         lhs_sel         = lhs_sel;
         specifics       = specifics_gen;
       }
     in
     ( [ specific_tag ], updated_frame_info )


(* For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this prologue:
 *     frame_p __prologue_f( frame_p caller, void *ret_addr, rt *lhs, p1, p2, p3 )
 *     {
 *        frame_p frame = __generic_prologue(
 *           sizeof( __specific_f ),
 *           ret_addr,
 *           caller,
 *           __yielding_f );
 *        XXX: OOM not implemented
 *        if( !frame )
 *            return __oom( caller );
 *        ( __specifics_f ) *specifics = __specifics_select( frame );
 *        specifics->p1 = p1;
 *        specifics->p2 = p2;
 *        specifics->p3 = p3;
 *        specifics->__ret_val_ptr = lhs;
 *        /* TODO: Verify that Cil doesn't use inits on local variables (i.e. an
 *         * init in source will turn into expressions) */
 *        return frame;
 *     }
 * Notes:
 * - The call to "generic" (as opposed to generating the code in-line)
     helps keep overall code size down.  Do measurement later.
 * - If the return type is void, the return value business is absent.
 *)
(* NEW IDEA! For this function definition:
 *     rt f( p1, p2, p3 ) { ... }
 * Generate this prologue-init:
 *     void __prologue_f( char *g, va_list )
 *     {
 *        ( __specifics_f ) *specifics = __specifics_cast( g );
 *        specifics->p1 = __builtin_va_next( ... );
 *        specifics->p2 = __builtin_va_next( ... );
 *        specifics->p3 = __builtin_va_next( ... );
 *        specifics->__ret_val_ptr = ;
 *        /* TODO: Verify that Cil doesn't use inits on local variables (i.e. an
 *         * init in source will turn into expressions) */
 *     }
 * Notes:
 * - The call to "generic" (as opposed to generating the code in-line)
     helps keep overall code size down.  Do measurement later.
 * - If the return type is void, the return value business is absent.
 *)
let make_prologue prologue frame =
  let makeFormal = C.makeFormalVar prologue in
  let makeLocal  = C.makeLocalVar  prologue in
  let fdef_loc   = prologue.C.svar.C.vdecl in
  let this       = C.var( makeLocal "frame" frame.typ_ptr ) in
  let call_to_generic =
    let caller  =  makeFormal "caller"   frame.typ_ptr in
    let ret_addr = makeFormal "ret_addr" C.voidPtrType in
    let ps = L.map var2exp [ ret_addr; caller; frame.yielding ] in
    C.Call( Some this, gen_prologue_e(), ( C.SizeOf frame.specific_type )::ps, fdef_loc )
  in
  let assignments = match frame.formals, frame.ret_type with
      (* If no need for assignments, don't even get the specifics address *)
      [], C.TVoid _ -> []
    | fs, _ ->
       let specifics_var   = C.var(
           makeLocal "specifics" ( C.TPtr( frame.specific_type, [] ) ) ) in
       let specifics_val   = frame.specific_sel this in
       let specifics_init  = C.Set( specifics_var, specifics_val, fdef_loc ) in
       let specifics_table = frame.specifics ( C.Lval specifics_var ) in
       let set_ret_val_ptr = match frame.ret_type with
           C.TVoid _ -> []
         | t -> let lhs_param = makeFormal "lhs" ( C.TPtr( t, [] ) ) in
                let lhs_frm = frame.lhs_sel( frame.specific_sel this ) in
                [ C.Set( lhs_frm, var2exp lhs_param, fdef_loc ) ]
       in
       let assign_param ( _, _, _, v ) =
         let specific_var =
           match IH.tryfind specifics_table v.C.vid with
             Some f -> f C.NoOffset
           | _ -> E.s( E.error "Missing local \"%s\"" v.C.vname )
         in
         let formal = makeFormal v.C.vname v.C.vtype in
         C.Set( specific_var, var2exp formal, fdef_loc )
       in
       specifics_init::( L.map assign_param fs @ set_ret_val_ptr )
  in
  let body =
    let ms = C.mkStmt in
    let then_block = C.mkBlock[ ms( C.Instr assignments ) ] in
    C.mkBlock[ ms( C.Instr [ call_to_generic ] );
               ms( C.If( C.Lval this, then_block, C.mkBlock[], fdef_loc ) );
               ms( C.Return( Some( C.Lval this ), fdef_loc ) ) ]
  in
  let real_type = match prologue.C.svar.C.vtype with
      C.TFun( _, ps, va, attrs ) -> C.TFun( frame.typ_ptr, ps, va, attrs )
    | _ -> E.s( E.bug "ANGRY" )
  in
  let () = C.setFunctionType prologue real_type in
  let () = prologue.C.sbody <- body in
  prologue

(*
 * Translate:
 *     lhs = setjmp( env );
 * to:
 *     __setjmp_yield( &lhs, env, &__return_N, frame );
 *   __return_N:
 *)
let coroutinify_setjmp lhs_opt params loc frame =
  let lhs = match lhs_opt with
      Some lhs -> C.AddrOf lhs
    | None -> C.zero
  in
  let env = match params with
      [e] -> e
    | _ -> E.s( E.error "call to setjmp with bad params?!?" )
  in
  let after_setjmp =
    let r = C.mkStmt( C.Block( C.mkBlock [] ) ) in
    let () = r.C.labels <- [ fresh_return_label loc ] in
    r
  in
  let setjmp_call =
    let ps = [ lhs; env; C.AddrOfLabel( ref after_setjmp ); frame.exp ] in
    C.Call( None, setjmp_yield_e(), ps, loc )
  in
  [ C.mkStmt( C.Instr( [ setjmp_call ] ) ); after_setjmp ]

(*
 * Translate:
 *     longjmp( env, val );
 * to:
 *     return __longjmp_yield( env, val, frame );
 *)
let coroutinify_longjmp params fdec loc frame =
  let env, value = match params with
      [e;v] -> e, v
    | _ -> E.s( E.error "call to longjmp with bad params?!?" )
  in
  let next_frame = C.var( C.makeTempVar fdec ~name:"next_frame" frame.typ_ptr ) in
  let longjmp_call =
    let ps = [ env; value; frame.exp ] in
    C.Call( Some next_frame, longjmp_yield_e(), ps, loc )
  in
  let return_next_frame = C.mkStmt( C.Return( Some( C.Lval next_frame ), loc ) )  in
  [ C.mkStmt( C.Instr( [ longjmp_call ] ) ); return_next_frame ]

(*
 * Translate:
 *     lhs = alloca( sz );
 * to:
 *     return __alloca( &lhs, sz, &__return_N, frame );
 *   __return_N:
 *)
let coroutinify_alloca lhs_opt params fdec loc frame =
  let sz = match params with
      [s] -> s
    | _ -> E.s( E.error "call to alloca with bad params?!?" )
  in
  match lhs_opt with
    None -> []
  | Some lhs ->
     let after_alloca =
       let r = C.mkStmt( C.Block( C.mkBlock [] ) ) in
       let () = r.C.labels <- [ fresh_return_label loc ] in
       r
     in
     let next_frame = C.var( C.makeTempVar fdec ~name:"next_frame" frame.typ_ptr ) in
     let alloca_call =
       let ps = [ C.AddrOf lhs; sz; C.AddrOfLabel( ref after_alloca ); frame.exp ] in
       C.Call( Some next_frame, alloca_impl_e(), ps, loc )
     in
     let return_next_frame = C.mkStmt( C.Return( Some( C.Lval next_frame ), loc ) )  in
     [ C.mkStmt( C.Instr( [ alloca_call ] ) ); return_next_frame; after_alloca ]

(*
 * Translate:
 *     __activate_intermediate( act, lhs_ptr, fn, p1, p2, p3 );
 * to:
 *     act_frame = __prologue_fn( frame, 0, p1, p2, p3 );
 *     return __activate( frame, &__return_N, act, act_frame );
 *   __return_N:
 *)
let coroutinify_activate params fdec loc frame =
  let act, lhs_ptr, entry_fn, real_params =
    match params with
      a::l::e::r -> a, l, e, r
    | _ -> E.s( E.bug "Call to activate_intermediate with too few params" )
  in
  let prologue =
    match entry_fn with
      C.AddrOf( C.Var v, C.NoOffset ) ->
      ( match lookup_fun_decl v with
          Some ( _, p, _ ) -> p
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
               C.Lval act_frame ] in
    C.Call( Some next_frame, activate_e(), ps, loc )
  in
  let return_next_frame = C.mkStmt( C.Return( Some( C.Lval next_frame ), loc ) )  in
  let p, a =
    C.mkStmt( C.Instr( [ prologue_call ] ) ),
    C.mkStmt( C.Instr( [ activate_call ] ) )
  in
  [ p; a; return_next_frame; after_act ]

(* Translate:
 *     __wait()
 * to:
 *     return __waiting_or_done( frame, &&after );
 *     after:
 *)
let coroutinify_wait fdec frame loc =
  let next = C.var( C.makeTempVar fdec ~name:"next_frame" frame.typ_ptr ) in
  let dummy = C.mkStmt( C.Instr[] ) in
  let () = dummy.C.labels <- [ fresh_return_label loc ] in
  let call =
    let ps = [ frame.exp; C.AddrOfLabel( ref dummy ) ] in
    C.Call( Some next, act_wait_done_e(), ps, loc )
  in
  let return_stmt = C.mkStmt( C.Return( Some( C.Lval next ), loc ) ) in
  [ C.mkStmt( C.Instr[ call ] ); return_stmt; dummy ]

(* Translate direct calls from:
 *     lhs = f( p1, p2, p3 );
 * to:
 *     return __prologue_f( frame, &__return_N, &lhs, p1, p2, p3 );
 *   __return_N:
 *
 * Translate indirect calls from:
 *     lhs = exp( p1, p2, p3 );
 * to:
 *     return exp( frame, &__return_N, &lhs null, p1, p2, p3 );
 *   __return_N:
 *
 *)
type dir_indir =
    CBuiltIn of C.varinfo
  | CDirect of C.exp * C.exp
  | CIndirect of C.exp

let coroutinify_normal_call lhs_opt callee_rt orig_params call_stuff fdec loc frame =
  let callee = C.var( C.makeTempVar fdec ~name:"callee" frame.typ_ptr ) in
  let after_return = C.mkEmptyStmt () in
  let () = after_return.C.labels <- [ fresh_return_label loc ] in
  let params =
    match callee_rt with
      C.TVoid _ -> orig_params
    | t ->
       let l = match lhs_opt with
           Some l -> l
         | None -> E.s( E.bug "572469246" )
       in
       ( C.AddrOf l )::orig_params
  in
  (* XXX fix to be direct or indirect *)
  let ps = frame.exp::( C.AddrOfLabel( ref after_return ) )::params in
  let prologue_call =
    let f = match call_stuff with
        CDirect( _, p ) -> p
      | CIndirect p -> p
      | CBuiltIn _ -> E.s( E.bug "Noooo Act yiel" )
    in
    let call = C.Call( Some callee, f, ps, loc ) in
    C.mkStmt( C.Instr[ call ] )
  in
  let return_callee = C.mkStmt( C.Return( Some( C.Lval callee ), loc ) )  in
  [ prologue_call; return_callee; after_return ]

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
  let yield_call = C.Call( Some next_frame, yield_impl_e(), params, loc ) in
  let return_next = C.mkStmt( C.Return( Some( C.Lval next_frame ), loc ) )  in
  [ C.mkStmt( C.Instr[ yield_call ] ); return_next; post_yield_stmt ]

let coroutinify_call visitor fdec frame instr =
  (* let () = trc( P.dprintf "coroutinify_call %a %a\n" *)
  (*                              C.d_lval ( C.var fdec.C.svar ) *)
  (*                              C.d_instr instr ) in *)
  match instr with
    C.Call( lhs_opt_pre_vis, fn_exp_pre_vis, params_pre_vis, loc ) ->
    (* let () = *)
    (*   trc( P.dprintf "CORO CALL exp: %a   type:%a\n" *)
    (*                       C.d_exp fn_exp_pre_vis *)
    (*                       C.d_type( C.typeOf fn_exp_pre_vis ) ) *)
    (* in *)
    let callee_ret_type, callee_param_types, callee_varargs, callee_attrs =
      getTFunInfoP ( C.typeOf fn_exp_pre_vis ) "OH NOES"
    in
    let lhs_opt = opt_map (C.visitCilLval visitor) lhs_opt_pre_vis in
    let fn_exp = C.visitCilExpr visitor fn_exp_pre_vis in
    let params = L.map (C.visitCilExpr visitor) params_pre_vis in
    let call_stuff = match fn_exp with
        C.Lval( C.Var v, C.NoOffset ) ->
        ( try
          if v.C.vid = (act_intermed()).C.vid ||
             v.C.vid = (yield()).C.vid ||
             v.C.vid = (alloca()).C.vid ||
             v.C.vid = (setjmp()).C.vid ||
             v.C.vid = (longjmp()).C.vid ||
             v.C.vid = (self_activity()).C.vid ||
             v.C.vid = (activity_wait()).C.vid ||
             v.C.vid = (mode_test()).C.vid
          then CBuiltIn v
          else
            ( match lookup_fun_decl v with
                Some ( u, p, _ ) -> CDirect( u, p )
              | None -> CIndirect fn_exp )
        with Not_found -> E.s( E.bug "5235092750" ) )
      | _ -> CIndirect fn_exp
    in
    ( match call_stuff with
        CBuiltIn v ->
        if v.C.vid = (act_intermed()).C.vid then
          match lhs_opt with
            Some _ -> (* XXX lhs? *) coroutinify_activate params fdec loc frame
          | None -> coroutinify_activate params fdec loc frame
        else if v.C.vid = (alloca()).C.vid then
          coroutinify_alloca lhs_opt params fdec loc frame
        else if v.C.vid = (setjmp()).C.vid then
          coroutinify_setjmp lhs_opt params loc frame
        else if v.C.vid = (longjmp()).C.vid then
          coroutinify_longjmp params fdec loc frame
        else if v.C.vid = (yield()).C.vid then
          coroutinify_yield lhs_opt params fdec loc frame
        else if v.C.vid = (self_activity()).C.vid then
          match lhs_opt with
            None -> []
          | Some lhs ->
             let a = C.Lval( frame.activity_sel frame.exp ) in
             [ C.mkStmt( C.Instr[ C.Set( lhs, a, loc ) ] ) ]
        else if v.C.vid = (mode_test()).C.vid then
          match lhs_opt with
            None -> []
          | Some lhs -> [ C.mkStmt( C.Instr[ C.Set( lhs, C.one, loc ) ] ) ]
        else if v.C.vid = (activity_wait()).C.vid then
          coroutinify_wait fdec frame loc
        else
          E.s( E.bug "Built-in %s" v.C.vname )
      | _ when exp_is_charcoal_fn fn_exp_pre_vis ->
         coroutinify_normal_call lhs_opt callee_ret_type params call_stuff fdec loc frame
      | _ -> [ C.mkStmt( C.Instr( C.visitCilInstr visitor instr ) ) ]
    )

  (* Something other than a call to a Charcoal function *)
  | _ -> [ C.mkStmt( C.Instr( C.visitCilInstr visitor instr ) ) ]


(* Translate returns from:
 *     return exp;
 * to:
 *     *( frame->specific.lhs_ptr ) = exp;
 *     #if activity entry
 *         return __activity_epilogue( frame );
 *     #else
 *         return __generic_epilogue( frame );
 *
 * Prereq: exp should be coroutinified already
 *)
let coroutinify_return fdec rval_opt loc frame is_activity_entry =
  let set_return_val = match rval_opt with
      None -> []
    | Some e ->
       let lhs = C.Lval( frame.lhs_sel( frame.specific_sel frame.lval ) ) in
       [ C.Set( ( C.Mem lhs, C.NoOffset ), e, loc ) ]
  in
  let next =
    C.var( C.makeTempVar fdec ~name:"next_frame" frame.typ_ptr )
  in
  let call =
    let f = if is_activity_entry then act_epilogue_e() else gen_epilogue_e() in
    C.Call( Some next, f, [ frame.exp ], loc )
  in
  let instrs = set_return_val @ [ call ] in
  let return_stmt = C.mkStmt( C.Return( Some( C.Lval next ), loc ) ) in
  let stmts = [ C.mkStmt( C.Instr( instrs ) ); return_stmt ] in
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
 *     lhs = setjmp( env );
 * to:
 *     env->yielding_tag = 0;
 *     lhs = __setjmp_c( env->_.no_yield_env );
 *)
let no_yield_setjmp lhs params frame =
  let env = match params with
      [e] -> e
    | _ -> E.s( E.error "Bad params for setjmp?!?" )
  in
  let () = trc( P.dprintf "setjmp %a\n" C.d_type( C.typeOf env ) ) in
  []

(*
 * Translate:
 *     longjmp( env, val );
 * to:
 *     __longjmp_no( env, val );
 *)
let no_yield_longjmp params loc =
  [ C.Call( None, longjmp_no_e(), params, loc ) ]

(*
 * Translate:
 *     __activate_intermediate( act, XXX lhs_ptr, fn, p1, p2, p3 );
 * to:
 *     act_frame = __prologue_fn( 0, 0, p1, p2, p3 );
 *     __activate( 0, 0, act, act_frame );
 *)
let no_yield_activate params fdec loc frame =
  let act, lhs_opt, entry_fn, real_params =
    match params with
      a::l::e::r -> a, l, e, r
    | _ -> E.s( E.bug "Call to activate_intermediate with too few params" )
  in
  let prologue =
    match entry_fn with
      C.AddrOf( C.Var v, C.NoOffset ) ->
      ( match lookup_fun_decl v with
          Some ( _, p, _ ) -> p
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
    let ps = [ C.zero; C.zero; act; C.Lval act_frame ] in
    C.Call( None, activate_e(), ps, loc )
  in
  [ prologue_call; activate_call ]

let no_yield_indirect_call lval f ps loc =
  let lval_param = match lval with
      None   -> C.zero
    | Some l -> C.AddrOf l
  in
  change_do_children[ C.Call( None, f, C.zero::C.zero::lval_param::ps, loc ) ]

let no_yield_call i fdec frame =
  match i with
    C.Call( lval_opt, ( C.Lval( C.Var fname, C.NoOffset ) as f), params, loc ) ->
    if fname.C.vid = (act_intermed()).C.vid then
      change_do_children( no_yield_activate params fdec loc frame )
    else if fname.C.vid = (setjmp()).C.vid then
      change_do_children( no_yield_setjmp lval_opt params frame )
    else if fname.C.vid = (longjmp()).C.vid then
      change_do_children( no_yield_longjmp params loc )
    else if fname.C.vid = (yield()).C.vid || fname.C.vid = (mode_test()).C.vid then
      change_do_children(
          match lval_opt with
            None -> []
          | Some lval -> [ C.Set( lval, C.zero, loc ) ] )
    else if fname.C.vid = (self_activity()).C.vid then
      change_do_children(
          match lval_opt with
            None -> []
          | Some lval -> [ C.Set( lval, C.zero, loc ) ] ) (* XXX unimp *)
    else if fname.C.vid = (activity_wait()).C.vid then
      change_do_children(
          match lval_opt with
            None -> []
          | Some lval -> [ C.Set( lval, C.zero, loc ) ] ) (* XXX unimp *)
    else if exp_is_charcoal_fn f then
      (match lookup_fun_decl fname with
         Some( n, _, _ ) ->
         change_do_children[ C.Call( lval_opt, n, params, loc ) ]
       | None -> no_yield_indirect_call lval_opt f params loc )
    else
      C.DoChildren

  (* Indirect call: *)
  | C.Call( lval_opt, fn, params, loc ) when exp_is_charcoal_fn fn ->
     no_yield_indirect_call lval_opt fn params loc

  | _ -> C.DoChildren

exception FoundIt
class hasAddrOfLabelVisitor = object(self)
  inherit C.nopCilVisitor
  method vexpr e =
    match e with
      C.AddrOfLabel _ -> raise FoundIt
    | _ -> C.DoChildren
end

(* In the body of f, change calls to no_yield versions *)
class coroutinifyNoYieldVisitor fdec frame = object(self)
  inherit C.nopCilVisitor

  method vexpr e = match e with
      C.UnOp( C.NoYield, exp, _ (* Could type matter? *) ) ->
      change_do_children exp
    | C.AddrOf( C.Var v, C.NoOffset ) ->
       ( match lookup_fun_decl_var v with
           Some( _, _, i ) -> change_do_children( C.AddrOf( C.Var i, C.NoOffset ) )
         | None -> C.DoChildren )

    | _ -> C.DoChildren

  method vinst i = no_yield_call i fdec frame

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
    | C.AddrOf( C.Var v, C.NoOffset ) ->
       ( match lookup_fun_decl_var v with
           Some( _, _, i ) -> change_do_children( C.AddrOf( C.Var i, C.NoOffset ) )
         | None -> C.DoChildren )

    | _ -> C.DoChildren

  method vinst i =
    if self#yielding_mode() then
      C.DoChildren
    else
      no_yield_call i fdec frame

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
 *         __specifics_f *specifics = &specifics_cast( frame );
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
let make_yielding yielding frame_no_this is_activity_entry =
  (* let () = trc( P.dprintf "MAKE YIELDING %s\n" yielding.C.svar.C.vname ) in *)
  (* let () = *)
  (*   trc( P.dprintf "%a\n" C.d_block yielding.C.sbody ) *)
  (* in *)
  let fdef_loc = yielding.C.svar.C.vdecl in
  let () = clear_formals_locals yielding frame_no_this.typ_ptr in
  let this = C.makeFormalVar yielding "frame" frame_no_this.typ_ptr in
  let frame =
    { frame_no_this with lval = C.var this; exp = var2exp this; }
  in
  let ( specifics_tbl, specifics_init ) =
    let specifics = C.var(
        C.makeTempVar yielding ~name:"specifics"
                      ( C.TPtr( frame.specific_type, [] ) ) ) in
    let specifics_val = frame.specific_sel( C.var this ) in
    let specifics_init_instr = C.Set( specifics, specifics_val, fdef_loc ) in
    let specifics_init = C.mkStmt( C.Instr[ specifics_init_instr ] ) in
    ( frame.specifics( C.Lval( specifics ) ), specifics_init )
  in
  let v = new coroutinifyYieldingVisitor
              yielding specifics_tbl frame is_activity_entry
  in
  let y = C.visitCilFunction ( v :> C.cilVisitor ) yielding in
  let stmts =
    let hasComputedGoto =
      try let _ = C.visitCilFunction ( new hasAddrOfLabelVisitor ) y in
          false
      with FoundIt -> true
    in
    if hasComputedGoto then
      let goto_stmt =
        let ret_addr_field = C.Lval( frame.return_addr_sel( var2exp this ) ) in
        let empty_block = C.mkBlock( [ C.mkEmptyStmt() ] ) in
        let goto = C.mkStmt( C.ComputedGoto( ret_addr_field, fdef_loc ) ) in
        C.mkStmt( C.If( ret_addr_field, C.mkBlock( [ goto ] ), empty_block, fdef_loc ) )
      in
      specifics_init :: goto_stmt :: y.C.sbody.C.bstmts
    else
      specifics_init :: y.C.sbody.C.bstmts
  in
  let () = y.C.sbody.C.bstmts <- stmts in
  y

let make_no_yield no_yield frame_info =
  let v = new coroutinifyNoYieldVisitor no_yield frame_info in
  C.visitCilFunction ( v :> C.cilVisitor ) no_yield

(* XXX Need to replace &fn with &__indirect_fn ... Maybe not ... *)
(* XXX Need to replace fun ptr types w/ our craziness *)
(* XXX varargs seems tricky *)

(* For this function definition
 *     rt f( p1, p2, p3 ) { ... }
 * Generate the function for indirect calling:
 * NOTE: This function can be called in two different contexts:
 *   - no_yield mode (indicated by caller == NULL)
 *   - yielding mode prologue (indicated by ret != NULL)
 *
 *     frame_p __indirect_f( frame_p caller, void *ret_addr, rt *lhs, p1, p2, p3 )
 *     {
 *         /* UNIMP assertions */
 *         assert( lhs ); /* lhs only exists if non-void return type */
 *         assert( ret <==> caller );
 *         if( caller ) /* Caller in yielding mode */
 *         {
 *     #if yielding version exists
 *             return __prologue_f( caller, ret, lhs, p1, p2, p3 );
 *     #else
 *             *lhs = __no_yield_f( p1, p2, p3 );
 *     #endif
 *         }
 *         else /* Caller in no_yield mode */
 *         {
 *     #if no_yield version exists
 *             *lhs = __no_yield_f( p1, p2, p3 );
 *     #else
 *             ERROR Cannot call yielding-only functions in no_yield mode!!!
 *             exit();
 *     #endif
 *         }
 *         return caller;
 *     }
 *)
let make_indirect indirect no_yield prologue frame =
  let loc = indirect.C.svar.C.vdecl in
  let makeFormal = C.makeFormalVar indirect in
  let caller   = var2exp( makeFormal "caller"  frame.typ_ptr ) in
  let ret_addr = var2exp( makeFormal "ret_ptr" C.voidPtrType ) in
  let lhs_opt  = match frame.ret_type with
      C.TVoid _ -> None
    | t -> Some( var2exp( makeFormal "lhs" ( C.TPtr( t, [] ) ) ) )
  in
  let app_formals =
    L.map ( fun( n, t, a, v ) -> var2exp( makeFormal n t ) ) frame.formals
  in
  let yielding_blk =
    let callee = C.var( C.makeTempVar indirect ~name:"callee" frame.typ_ptr ) in
    let ps = match lhs_opt with
        None -> caller::ret_addr::app_formals
      | Some lhs -> caller::ret_addr::lhs::app_formals
    in
    let call = C.mkStmt( C.Instr(
        [ C.Call( Some callee, var2exp prologue, ps, loc ) ] ) ) in
    let r = C.mkStmt( C.Return( Some( C.Lval callee ), loc ) ) in
    C.mkBlock [ call; r ]
  in
  let no_yield_blk =
    let lhs = opt_map( fun l -> ( C.Mem l, C.NoOffset ) ) lhs_opt in
    C.mkBlock [ C.mkStmt( C.Instr(
        [ C.Call( lhs, var2exp no_yield, app_formals, loc ) ] ) ) ]
  in
  let body =
    let i = C.mkStmt( C.If( caller, yielding_blk, no_yield_blk, loc ) ) in
    let r = C.mkStmt( C.Return( Some( caller ), loc ) ) in
    C.mkBlock[ i; r ]
  in
  let () =
    let real_type = match indirect.C.svar.C.vtype with
        C.TFun( _, ps, va, attrs ) -> C.TFun( frame.typ_ptr, ps, va, attrs )
      | _ -> E.s( E.bug "ANGRY" )
    in
    let () = C.setFunctionType indirect real_type in
    indirect.C.sbody <- body
  in
  indirect

(* XXX varargs charcoal functions blah! *)

(* Kinda dumb, but fill in missing LHSs.  We need this because returns
 * unconditionally write their return values _somewhere_. *)
class phase1 = object(self)
  inherit C.nopCilVisitor

  val mutable fdef_opt = None

  method vglob g =
    match g with
    | C.GFun( fdef, loc ) when type_is_charcoal_fn fdef.C.svar.C.vtype ->
       let () = fdef_opt <- Some fdef in
       C.ChangeDoChildrenPost( [g], fun g' -> let () = fdef_opt <- None in g' )
    | _ -> C.SkipChildren

  method vinst i =
    match fdef_opt, i with
      None, _ -> E.s( E.bug "982376056023" )
    | Some fdef, C.Call( None, callee, params, loc ) ->
      let ( t, _, _, _) = getTFunInfoP ( C.typeOf callee ) "OH NOES" in
      ( match t with
          C.TVoid _ -> C.SkipChildren
        | _ ->
           let lhs = C.var( C.makeTempVar fdef ~name:"lhs" t ) in
           C.ChangeTo [ C.Call( Some lhs, callee, params, loc ) ]
      )
    | _ -> C.SkipChildren

end

(* When we find the definition of the generic frame type, examine it and
 * record some stuff. *)
let examine_frame_t_struct ci dummy_var =
  let dummy_exp  = C.zero in
  let dummy_lval = ( C.Var dummy_var, C.NoOffset ) in
  let dummy_type = C.voidType in

  let ac, s, r, ce, cr =
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
  {
    (* generic: *)
    specific_cast   = specific_cast;
    typ             = frame_typ;
    typ_ptr         = frame_ptr_typ;
    activity_sel    = ( fun e -> ( C.Mem e, C.Field( ac,  C.NoOffset ) ) );
    caller_sel      = ( fun e -> ( C.Mem e, C.Field( cr,  C.NoOffset ) ) );
    callee_sel      = ( fun e -> ( C.Mem e, C.Field( ce,  C.NoOffset ) ) );
    return_addr_sel = ( fun e -> ( C.Mem e, C.Field( r,   C.NoOffset ) ) );
    (* function-specific *)
    specific_sel    = ( fun e -> dummy_exp );
    lhs_sel         = ( fun e -> dummy_lval );
    specific_type   = dummy_type;
    yielding        = dummy_var;
    specifics       = ( fun _ -> IH.create 0 );
    ret_type        = dummy_type;
    formals         = [];
    vararg          = false;
    attrs           = [];
    (* context-specific *)
    lval            = dummy_lval;
    exp             = C.zero;
  }

let make_function_skeletons f name =
  let () = remove_charcoal_linkage f in
  let y  = C.copyFunction f ( spf "%s%s"  yielding_pfx name ) in
  let n  = C.copyFunction f ( spf "%s%s"  no_yield_pfx name ) in
  let p  = C.emptyFunction  ( spf "%s%s"  prologue_pfx name ) in
  let i  = C.emptyFunction  ( spf "%s%s"  indirect_pfx name ) in
  let () = add_charcoal_linkage f in
  ( y, n, p, i )

class phase2 = object(self)
  inherit C.nopCilVisitor

  val mutable frame_opt = None
  val mutable frame_struct = None
  val mutable activity_struct = None

  method get_frame () =
    match frame_opt with
      Some f -> f
    | None -> E.s( E.bug "Should really have a frame here" )

  (* If we have encountered the def of crcl(frame_t), extract its fields. *)
  method fill_in_frame v =
    let () =
      match ( frame_opt, frame_struct, activity_struct ) with
        ( None, Some fci, Some aci ) ->
        let frame_info = examine_frame_t_struct fci v in
        let () = frame_opt <- Some( frame_info ) in
        let () = frame_struct <- None in
        let () = activity_struct <- None in
        ()
      | _ -> ()
    in
    match ( frame_opt, frame_struct, activity_struct ) with
        ( None, _, _ ) -> ()
      | ( Some frame_info, None, None ) ->
       let name = v.C.vname in
       let () = try let uid = H.find builtin_uids name in
                    IH.replace builtins uid v
                with Not_found -> () in
       (* let () = XXX *)
       (*   else if name = crcl "yield" then *)
       (*     let () = if not( type_is_charcoal_fn v.C.vtype ) then *)
       (*                add_charcoal_linkage_var v *)
       (*     in *)
       (*     frame_opt <- Some{ frame_info with yield = v } *)
       (* in *)
       ()
    | ( Some _, _, _ ) -> E.s( E.error "Too much frame" )

  method vglob g =
    match ( g, frame_opt ) with
    | ( C.GFun( original, loc ), Some generic_frame ) ->
       let orig_var  = original.C.svar in
       let orig_name = orig_var.C.vname in
       (* let () = trc( P.dprintf "P2 DEFN %b %s\n" *)
       (*     (type_is_charcoal_fn orig_var.C.vtype) orig_name ) in *)
       if type_is_charcoal_fn original.C.svar.C.vtype then
         let( yielding, no_yield, prologue, indirect ) =
           make_function_skeletons original orig_name
         in
         let ( tags, specific_frame_info ) =
           make_specific yielding orig_var generic_frame
         in
         let () =
           let f v = add_fun_decl v no_yield.C.svar prologue.C.svar indirect.C.svar in
           L.iter f [ orig_var; yielding.C.svar; no_yield.C.svar;
                      prologue.C.svar; indirect.C.svar ]
         in
         let () =
           let frame =
             { specific_frame_info with yielding = yielding.C.svar}
           in
           let f v = add_fun_def v frame in
           L.iter f [ orig_var; yielding.C.svar; no_yield.C.svar;
                      prologue.C.svar; indirect.C.svar ]
         in
         (* "p" before "y" *)
         let decls = L.map ( fun f -> C.GVarDecl( f.C.svar, loc ) )
                           [ yielding; no_yield; prologue; indirect; original ]
         in
         let funs = L.map ( fun f -> C.GFun( f, loc ) )
                          [ yielding; no_yield; prologue; indirect; original ]
         in
         C.ChangeTo ( tags @ decls @ funs )

       else
           C.SkipChildren

    | ( C.GFun( f, _ ), None ) ->
       E.s( E.error "GFun before frame_t def  :(  %s !" f.C.svar.C.vname )

    | C.GVarDecl( v, _ ), _ ->
       (* let () = trc( P.dprintf "P2 DECL %b %s %a\n" *)
       (*     (type_is_charcoal_fn v.C.vtype) v.C.vname C.d_type v.C.vtype ) in *)
       let () = self#fill_in_frame v in
       C.DoChildren

    (* We need to find various struct definitions *)
    | C.GCompTag( ci, loc ), _ ->
       (* let () = Pf.printf "ffv - gct - %s\n" ci.C.cname in *)
       if ci.C.cname = crcl "frame_t" then
         let () = frame_struct <- Some ci in
         C.DoChildren
       else if ci.C.cname = "activity_t" then
         let () = activity_struct <- Some ci in
         C.DoChildren
       else
         C.DoChildren

    | C.GVar( v, _, _ ), _ ->
       let () = self#fill_in_frame v in
       C.DoChildren

    | _ -> C.DoChildren
end

(*
 * Translate:
 *     rt f( x, y, z );
 * to:
 *     rt      __no_yield_f( x, y, z );
 *     frame_p __prologue_f( frame_p, void *, rt *, x, y, z );
 * XXX indirect frame_p f( frame_p, void *, rt *, ... );
 *)
let coroutinifyVariableDeclaration var loc frame =
  if var.C.vid = (setjmp()).C.vid || var.C.vid = (longjmp()).C.vid then
    C.ChangeTo []
  else if type_is_charcoal_fn var.C.vtype then
    let ( n, p, i ) =
      match IH.tryfind crcl_fun_decls var.C.vid with
        Some( n, p, i ) -> ( n, p, i )
      | None ->
         (* no-yield *)
         let name = var.C.vname in
         let n = C.makeGlobalVar ( spf "%s%s" no_yield_pfx name ) var.C.vtype in
         (* prologue *)
         let ( rt, ps_opt, vararg, attrs ) = getTFunInfo var.C.vtype "DECL" in
         let ps_no_lhs = opt_default [] ps_opt in
         (* let () = trc( P.dprintf "DECL %s %d rt: %a\n" var.C.vname *)
         (*     ( L.length ps_no_lhs ) C.d_type rt ) in *)
         let ps = match rt with
             C.TVoid _ -> ps_no_lhs
           | _ -> ( "lhs", C.TPtr( rt, [] ), [] )::ps_no_lhs
         in
         let params = ( "frm", frame.typ_ptr, [] )::( "ret_addr", C.voidPtrType, [] )
                      ::ps in
         let pt = C.TFun( frame.typ_ptr, Some params, vararg, attrs ) in
         let p = C.makeGlobalVar ( spf "%s%s" prologue_pfx name ) pt in
         (* indirect XXX fix indirect type *)
         let i = C.makeGlobalVar ( spf "%s%s" indirect_pfx name ) pt in
         let () = add_fun_decl var n p i in
         ( n, p, i )
    in
    let decl x = C.GVarDecl( x, loc ) in
    change_do_children ( L.map decl [ n; p; i ] )
  else
    C.DoChildren

let completeFunctionTranslation fdef loc =
  let fvar = fdef.C.svar in
  try
    let frame = IH.find crcl_fun_defs fvar.C.vid in
    let ( n, p, i ) = IH.find crcl_fun_decls fvar.C.vid in
    let funs =
      if fvar.C.vid = n.C.vid then
        (* let () = trc( P.dprintf "*** no-yield\n" ) in *)
        let no_yield = make_no_yield fdef frame in
        [ C.GFun( no_yield, loc ) ]
      else if fvar.C.vid = p.C.vid then
        (* let () = trc( P.dprintf "*** prologue\n" ) in *)
        let prologue = make_prologue fdef frame in
        [ C.GFun( prologue, loc ) ]
      else if fvar.C.vid = frame.yielding.C.vid then
        (* let () = trc( P.dprintf "*** yielding\n" ) in *)
        let n = after_prefix fvar.C.vname yielding_pfx in
        let is_activity_entry =
          (* XXX can't call main. blah. *)
          starts_with n ( crcl "act_" ) || n = crcl "application_main"
        in
        let yielding = make_yielding fdef frame is_activity_entry in
        [ C.GFun( yielding, loc ) ]
      else if fvar.C.vid = i.C.vid then
        (* let () = trc( P.dprintf "*** indirect\n" ) in *)
        let indirect = make_indirect fdef n p frame in
        [ C.GFun( indirect, loc ) ]
      else (* The original definiton *)
        (* let () = trc( P.dprintf "*** original\n" ) in *)
        []
    in
    change_do_children funs
  with Not_found -> C.DoChildren

class phase3 generic_frame = object( self )
  inherit C.nopCilVisitor
  val mutable setjmp_erase = false

  method vglob g =
    if setjmp_erase then
      let () =
        match g with
        | C.GType( ti, _ ) ->
           if starts_with ti.C.tname "__charcoal_setjmp_tricky_hack_post" then
             setjmp_erase <- false
        | _ -> ()
      in
      C.ChangeTo []
    else
      match g with
      | C.GFun( fdef, loc ) ->
         (* let () = trc( P.dprintf "P3 DEFN %b %s\n" *)
         (*     (type_is_charcoal_fn fdef.C.svar.C.vtype) fdef.C.svar.C.vname ) in *)
         completeFunctionTranslation fdef loc
      | C.GVarDecl( var, loc ) ->
         (* let () = trc( P.dprintf "P3 DECL %b %s %a\n" *)
         (*     (type_is_charcoal_fn var.C.vtype) var.C.vname C.d_type var.C.vtype ) in *)
         coroutinifyVariableDeclaration var loc generic_frame
      | C.GPragma( C.Attr( a, _ ), _ ) ->
         if a = "cilnoremove" then
           C.ChangeTo []
         else
           C.DoChildren
      | C.GType( ti, _ ) ->
         if starts_with ti.C.tname "__charcoal_setjmp_tricky_hack_pre" then
           let () = setjmp_erase <- true in
           C.ChangeTo []
         else
           C.DoChildren
      | _ -> C.DoChildren

  method vtype t =
    let () = () (* trc( P.dprintf "P3 type %a\n" C.d_type t ) *) in
    match t with
      C.TPtr( C.TFun( ret_type, params_opt, varargs, fun_attrs ), ptr_attrs ) ->
      ( match remove_charcoal_linkage_from_attrs false fun_attrs with
          Some others ->
          let ymps = [ ( "caller", generic_frame.typ_ptr, [] );
                     ( "ret_addr", C.voidPtrType, [] ) ]
          in
          let params = opt_default [] params_opt in
          let ps = match ret_type with
              C.TVoid _ -> ymps @ params
            | t -> ymps @ [ ( "lhs", C.TPtr( t, [] ), [] ) ] @ params
          in
          change_do_children
            ( C.TPtr( C.TFun( generic_frame.typ_ptr, Some ps, varargs, others ), ptr_attrs ) )
        | None -> C.DoChildren )
    | _ -> C.DoChildren

  method vattr attr =
    let () = () (* trc( P.dprintf "P3 attr\n" ) *) in
    match attr with
      C.Attr( "linkage_charcoal", [] ) -> C.ChangeTo []
    | _ -> C.DoChildren

         (* let () = *)
         (*   match lookup_fn_translation_var orig_var with *)
         (*     None -> () *)
         (*   | Some ( n, p, e ) -> *)
         (*      let () =  no_yield.C.svar <- n in *)
         (*      let () =  prologue.C.svar <- p in *)
         (*      let () = epilogueB.C.svar <- e in *)
         (*      () *)
         (* in *)

         (* let p  = make_prologue  prologue  frame_info yielding.C.sformals in *)
         (* let y  = make_yielding  yielding  frame_info is_activity_entry in *)
         (* let eA = make_epilogueA epilogueA frame_info in *)
         (* let eB = make_epilogueB epilogueB frame_info in *)
         (* let i  = make_indirect  original  frame_info in *)

  method vvrbl v =
    (* let () = trc( P.dprintf "Phase 3 %s %d %d %d\n" v.C.vname v.C.vid *)
    (*                         (setjmp_c()).C.vid (longjmp_c()).C.vid ) in *)
    if v.C.vid = (setjmp_c()).C.vid then
      C.ChangeTo( setjmp() )
    else if v.C.vid = (longjmp_c()).C.vid then
      C.ChangeTo( longjmp() )
    else
      C.SkipChildren

end

(*
 * Phase 1:
 * - Fill in missing LHSs
 * Phase 2:
 * - Find the runtime builtin stuff
 * - Translate Charcoal function _definitions_ to dummy pro/yield/...
 * Phase 3:
 * - Translate Charcoal function _declarations_
 * - Complete translation of function definitions
 *)
let do_coroutinify( f : C.file ) =
  let () = C.visitCilFile ( new phase1 ) f in
  let phase2obj = new phase2 in
  let () = C.visitCilFile ( phase2obj :> C.cilVisitor ) f in
  let () =
    let check name uid =
      if not( IH.mem builtins uid ) then
        E.s( E.bug "Missing builtin %s" name )
    in
    H.iter check builtin_uids
  in
  let () = C.visitCilFile ( new phase3 ( phase2obj#get_frame() ) ) f in
  ()

let feature : C.featureDescr =
  { C.fd_name = "coroutinify";
    C.fd_enabled = Cilutil.doCoroutinify;
    C.fd_description = "heap-allocate call frames";
    C.fd_extraopt = [];
    C.fd_doit = do_coroutinify;
    C.fd_post_check = true ;
  }

(* Graveyard *)
    (*     let () = let p () = match g with *)
    (*   | C.GType( t, l )    -> Pf.printf "T %s\n%!" t.C.tname *)
    (*   | C.GCompTag( c, l ) -> Pf.printf "CT %s\n%!" c.C.cname *)
    (*   | C.GCompTagDecl( c, l ) -> Pf.printf "CTD %s\n%!" c.C.cname *)
    (*   | C.GEnumTag( e, l ) -> Pf.printf "ET\n%!" *)
    (*   | C.GEnumTagDecl( e, l ) -> Pf.printf "ETD\n%!" *)
    (*   | C.GVarDecl( v, l ) -> ()(\*Pf.printf "VD %s\n%!" v.C.vname*\) *)
    (*   | C.GVar( v, i, l ) -> Pf.printf "V %s\n%!" v.C.vname *)
    (*   | C.GFun( f, l ) -> Pf.printf "F %s\n%!" f.C.svar.C.vname *)
    (*   | C.GAsm( s, l ) -> Pf.printf "A\n%!" *)
    (*   | C.GPragma( a, l ) -> Pf.printf "P\n%!" *)
    (*   | C.GText( s ) -> Pf.printf "T\n%!" *)
    (* in (\* p *\) () in *)
