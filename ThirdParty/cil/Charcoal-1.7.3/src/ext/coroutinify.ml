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
  inherit C.copyFunctionVisitor
             
class foo = object(self)
  inherit C.nopCilVisitor
  method vglob g =
    match g with
      GVarDecl( vinfo, loc ) -> E.s( E.unimp "funs?" )
    | GFun( fdec, loc ) -> E.s( E.unimp "funs!" )
        
    C.ChangeDoChildrenPost( [g], fn g -> g )
             
let feature : featureDescr =
  { fd_name = "coroutinify";
    fd_enabled = Cilutil.doCoroutinify;
    fd_description = "heap-allocate call frames";
    fd_extraopt = [];
    fd_doit = ();
    fd_post_check = true ;
  }

