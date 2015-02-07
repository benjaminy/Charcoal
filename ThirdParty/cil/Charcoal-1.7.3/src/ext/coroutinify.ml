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

module C = Cil
module E = Errormsg

class bar( newname: string ) = object( self )
                                       (*inherit C.copyFunctionVisitor*)
end

class coroutinifyVisitor = object(self)
  inherit C.nopCilVisitor
  method vglob g =
    match g with
    | GFun( fdec, loc ) ->
       let () = E.s( E.unimp "funs!" ) in
       C.ChangeDoChildrenPost ( [g], fun g -> g )
end

let do_coroutinify( f : C.file ) =
  let () = C.visitCilFile (new coroutinifyVisitor) f in
  ()

let feature : C.featureDescr =
  { fd_name = "coroutinify";
    fd_enabled = Cilutil.doCoroutinify;
    fd_description = "heap-allocate call frames";
    fd_extraopt = [];
    fd_doit = do_coroutinify;
    fd_post_check = true ;
  }

