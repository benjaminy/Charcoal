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

let change_do_children x = C.ChangeDoChildrenPost( x, fun e -> e )

let label_counter = ref 1
let fresh_return_label loc =
  let n = !label_counter in
  let () = label_counter := n + 1 in
  C.Label( spf "__charcoal_return_%d" n, loc, false )

let charcoal_pfx     = "__charcoal_"
let unyielding_pfx   = "__charcoal_fn_unyielding_"
let locals_pfx       = "__charcoal_fn_locals_"
let specific_pfx     = "__charcoal_fn_specific_"
let prologue_pfx     = "__charcoal_fn_prologue_"
let yielding_pfx     = "__charcoal_fn_yielding_"
let epilogue_pfx     = "__charcoal_fn_epilogue_"
let after_return_pfx = "__charcoal_fn_after_return_"
let charcoal_pfx_len = String.length charcoal_pfx

let crcl_fun_kind name =
  let c = ( charcoal_pfx = Str.string_before name charcoal_pfx_len ) in
  ( c, if c then Str.string_after name charcoal_pfx_len else name )


type frame_info =
  {
    (* generic: *)
    specific_cast   : C.typ -> C.lval -> C.exp;
    typ             : C.typ;
    callee_sel      : C.exp -> C.lval;
    goto_sel        : C.exp -> C.lval;
    gen_prologue    : C.varinfo;
    gen_epilogue    : C.varinfo;
    gen_after_ret   : C.varinfo;
    (* function-specific *)
    sizeof_specific : C.exp;
    locals_sel      : C.lval -> C.lval;
    return_sel      : C.lval -> C.lval;
    locals_type     : C.typ;
    epilogue        : C.varinfo;
    yielding        : C.varinfo;
    locals          : C.exp -> ( ( C.offset -> C.lval ) IH.t );
    (* context-specific *)
    lval            : C.lval;
    exp             : C.exp;
  }

class bar( newname: string ) = object( self )
                                       (*inherit C.copyFunctionVisitor*)
end

class coroutinifyUnyieldingVisitor = object(self)
  inherit C.nopCilVisitor
end

let function_vars : ( C.exp * C.exp * C.exp ) IH.t = IH.create 42
let lookup_fn_translation v = IH.tryfind function_vars v.C.vid
let add_fn_translation v u p a = IH.replace function_vars v.C.vid ( u, p, a )

(*
  try IH.find function_vars v.C.vid
  with Not_found ->
    (* XXX fix types: *)
    let mvi = C.makeVarinfo true in
    let i = mvi (spf "%s%s" prologue_pfx   v.C.vname) C.voidType in
    let y = mvi (spf "%s%s" yielding_pfx   v.C.vname) C.voidType in
    let u = mvi (spf "%s%s" unyielding_pfx v.C.vname) C.voidType in
 *)

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
let make_specific fdec frame_info =
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
    match List.filter ( fun v -> v.C.vreferenced )
                      ( fdec.C.sformals @ fdec.C.slocals )
    with
      [] -> ( unit_type, ( fun _ -> IH.create 0 ), [] )
    | all ->
       let field v =
         let () = sanity v in
         ( v.C.vname, v.C.vtype, None, [(*attrs*)], v.C.vdecl )
       in
       let fields = List.map field all in
       let struct_name = spf "%s%s" locals_pfx fname.C.vname in
       let ci = C.mkCompInfo true struct_name ( fun _ -> fields ) [(*attrs*)] in
       let locals_ptr = ref C.zero in
       let field_select t v =
         let field =
           { C.fcomp = ci; C.fname = v.C.vname; C.ftype = v.C.vtype;
             C.fbitfield = None; C.fattr = [(*attrs*)]; C.floc = v.C.vdecl }
         in
         IH.replace t v.C.vid ( fun o -> ( C.Mem !locals_ptr, C.Field( field, o ) ) )
       in
       let locals = IH.create( List.length all ) in
       let () = List.iter ( field_select locals ) all in
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
 *        ( __locals_f ) *locals = &locals_cast( frame );
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
let make_prologue original frame =
  let orig_var = original.C.svar in
  let fdef_loc = orig_var.C.vdecl in
  (* XXX function type? *)
  let prologue = C.emptyFunction( spf "%s%s" prologue_pfx orig_var.C.vname ) in
  let () = C.setFormals prologue original.C.sformals in
  let this = C.makeLocalVar prologue "frame" frame.typ in
  let call =
    let ret_ptr = C.makeFormalVar prologue ~where:"^" "ret_ptr" C.voidPtrType in
    let caller  = C.makeFormalVar prologue ~where:"^" "caller"  frame.typ in
    let ps = List.map ( fun v -> C.Lval( C.var v ) )
                      [ ret_ptr; caller; frame.yielding ]
    in
    let gp = C.Lval( C.var frame.gen_prologue ) in
    C.Call( Some( C.var this ), gp, frame.sizeof_specific::ps, fdef_loc )
  in
  (* If the original function has zero parameters, skip this *)
  let param_assignments = match original.C.sformals with
      [] -> []
    | fs ->
       let ( locals, locals_init ) =
         let locals = C.makeLocalVar prologue "locals" frame.locals_type in
         let locals_val = C.AddrOf( frame.locals_sel( C.var this ) ) in
         let locals_init = C.Set( C.var locals, locals_val, fdef_loc ) in
         ( frame.locals( C.Lval( C.var locals ) ), locals_init )
       in
       let assign_param v =
         let local_var =
           match IH.tryfind locals v.C.vid with
             Some f -> f C.NoOffset
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
let make_epilogue original frame =
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
  let this = C.var( C.makeFormalVar epilogue "frame" frame.typ ) in
  let caller = C.var( C.makeTempVar epilogue frame.typ ) in
  let instrs =
    let ge = C.Lval( C.var frame.gen_epilogue ) in
    let call_instr = C.Call( Some caller, ge, [ C.Lval this ], fdef_loc ) in
    match return_type with
      C.TVoid _ -> [ call_instr ]
    | _ ->
       let rval = C.Lval( C.var( C.makeFormalVar epilogue "v" return_type ) ) in
       [ C.Set( frame.return_sel this, rval, fdef_loc ); call_instr ]
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
 *             *lhs = return_cast( frame->callee );
 *         __generic_after_return( frame );
 *     }
 *)
let make_after_return original frame =
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
  let this = C.var( C.makeFormalVar after_ret "frame" frame.typ ) in
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
    let ( _, prologue, after_return ) = match fn_exp with
        C.Lval( C.Var v, C.NoOffset ) ->
        (match lookup_fn_translation v with
           Some x -> x
         | None -> E.s( E.unimp "Oh noes; indirect calls" ) )
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
      let call = C.Call( None, after_return, params, loc ) in
      let r = C.mkStmt( C.Instr[ call ] ) in
      let () = r.C.labels <- [ fresh_return_label loc ] in
      r
    in
    let callee = C.var( C.makeTempVar fdec frame.typ ) in
    let ps = frame.exp::( C.AddrOfLabel( ref after_call ) )::params in
    let prologue_call =
      let call = C.Call( Some callee, prologue, ps, loc ) in
      C.mkStmt( C.Instr[ call ] )
    in
    let return_callee = C.mkStmt( C.Return( Some( C.Lval callee ), loc ) )  in
    [ prologue_call; return_callee; after_call ]

  (* Something other than a call *)
  | _ -> [ C.mkStmt( C.Instr[ instr ] ) ]


(* Translate returns from:
 *     return exp;
 * to:
 *     return __epilogue_f( frame, exp );
 *)
let coroutinify_return fdec rval_opt loc frame =
  let caller = C.var( C.makeTempVar fdec frame.typ ) in
  let params = match rval_opt with
      None ->   [ frame.exp ]
    | Some e -> [ frame.exp; e ]
  in
  let ep = C.Lval( C.var frame.epilogue ) in
  let call = C.mkStmt( C.Instr[ C.Call( Some caller, ep, params, loc ) ] ) in
  let return_stmt = C.mkStmt( C.Return( Some( C.Lval caller ), loc ) ) in
  change_do_children( C.mkStmt( C.Block( C.mkBlock[ call; return_stmt ] ) ) )


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
       coroutinify_return fdec rval_opt loc frame

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
let make_yielding fdec frame_info =
  let fdef_var = fdec.C.svar in
  let fdef_loc = fdef_var.C.vdecl in
  let yielding =
    let new_name = spf "%s%s" yielding_pfx fdef_var.C.vname in
    C.copyFunction fdec new_name
  in
  let () = C.setFormals yielding [] in
  let () = yielding.C.slocals <- [] in
  let this = C.makeFormalVar yielding "frame" frame_info.typ in
  let ( locals_tbl, locals_init ) =
    let locals = C.var( C.makeTempVar yielding frame_info.locals_type ) in
    let locals_val = C.AddrOf( frame_info.locals_sel( C.var this ) ) in
    let locals_init = C.Set( locals, locals_val, fdef_loc ) in
    ( frame_info.locals( C.Lval( locals ) ), locals_init )
  in
  let goto_stmt =
    let goto_field = C.Lval( frame_info.goto_sel( C.Lval( C.var this ) ) ) in
    let empty_block = C.mkBlock( [ C.mkEmptyStmt() ] ) in
    let goto = C.mkStmt( C.ComputedGoto( goto_field, fdef_loc ) ) in
    C.mkStmt( C.If( goto_field, C.mkBlock( [ goto ] ), empty_block, fdef_loc ) )
  in
  (* XXX coroutinify!!! *)
  (* XXX return type *)
  yielding


(* In the body of f, change calls to unyielding versions *)
let make_unyielding fdec =
  fdec

(* When we find the definition of the generic frame type, examine it and
 * record some stuff. *)
let process_generic_frame ci dummy_var =
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
      List.iter find_field fields
    in
    let () = List.iter extract ci.C.cfields in
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
    callee_sel      = ( fun e -> ( C.Mem e, C.Field( ce, C.NoOffset ) ) );
    goto_sel        = ( fun e -> ( C.Mem e, C.Field( g, C.NoOffset ) ) );
    gen_prologue    = dummy_var;
    gen_epilogue    = dummy_var;
    gen_after_ret   = dummy_var;
    (* function-specific *)
    sizeof_specific = dummy_exp;
    locals_sel      = ( fun e -> dummy_lval );
    return_sel      = ( fun e -> dummy_lval );
    locals_type     = dummy_type;
    yielding        = dummy_var;
    epilogue        = dummy_var;
    locals          = ( fun _ -> IH.create 0 );
    (* context-specific *)
    lval            = dummy_lval;
    exp             = dummy_exp;
  }

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
          ( _, None ) -> E.s( E.error "frame_t should come before any fdefs!" )
        | ( ( false, _ ), Some generic_frame_info ) ->
(* XXX GCompTag??? *)
          let ( tags, frame_info ) =
            make_specific fdec generic_frame_info
          in
          let e = make_epilogue fdec frame_info in
          let frame_info =
            { frame_info with epilogue = e.C.svar }
          in
          let y = make_yielding fdec frame_info in
          let frame_info =
            { frame_info with yielding = y.C.svar }
          in
          let p = make_prologue     fdec frame_info in
          let a = make_after_return fdec frame_info in
          let u = make_unyielding   fdec in
          let () =
            let e fdec = C.Lval( C.var( fdec.C.svar ) ) in
            add_fn_translation fdec.C.svar ( e u ) ( e p ) ( e a )
          in
          (* make fn ptr struct *)
          change_do_children [g]
        | ( ( true, crcl_runtime_fn_name ), Some frame ) ->
           let () =
             if crcl_runtime_fn_name = "fn_generic_prologue" then
               frame_opt <-
                 Some{ frame with gen_prologue = fdec.C.svar };
             if crcl_runtime_fn_name = "fn_generic_epilogue" then
               frame_opt <-
                 Some{ frame with gen_epilogue = fdec.C.svar };
             if crcl_runtime_fn_name = "fn_generic_after_return" then
               frame_opt <-
                 Some{ frame with gen_after_ret = fdec.C.svar };
           in
           C.SkipChildren
       )

    (* We need to find various struct definitions *)
    | C.GCompTag( ci, loc ) ->
       if ci.C.cname = "__charcoal_frame_t" then
         match dummy_var with
           None -> E.s( E.error "Must be var?!?" )
         | Some v ->
            let () = frame_opt <- Some( process_generic_frame ci v ) in
            C.DoChildren
       else
         C.DoChildren

    | C.GVarDecl( v, _ ) ->
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

