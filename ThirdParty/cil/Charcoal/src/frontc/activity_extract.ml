(* Exctract activity bodies out into top-level procedures *)

module E = Errormsg
module T = Trace
module P = Pretty
module C = Cabs
module V = Cabsvisit

(* NOTE: For nested activities, if the inner uses a by-ref from way out,
   add the new var name to the inner activate's by-val list. *)

class extract_activity_class : V.cabsVisitor =
object (self)
  inherit V.nopCabsVisitor as super

  val mutable activate_depth = 0
  val mutable fun_def_depth = 0

  method vdef d : C.definition list V.visitAction = match d with
      C.FUNDEF(fname, body, loc, lend) ->
        let after defs =
          let () = fun_def_depth <- fun_def_depth - 1 in
          match defs with
              [def] -> defs
            | _ -> failwith "wrong number of defs"
        in
        (* XXX add the parameters to the local defs *)
        let () = fun_def_depth <- fun_def_depth + 1 in
        V.ChangeDoChildrenPost ([d], after)
    | C.DECDEF (name_group, loc) -> V.DoChildren
      (* XXX: I guess there might be typedefs and shit like that *)
    | _ -> V.DoChildren

  method vexpr e = match e with
      C.ACTIVATE (by_val_vars, stmt_body) ->
        if activate_depth > 0 then V.DoChildren
        else V.DoChildren
    | C.VARIABLE v ->
      if activate_depth > 0 then
        match None (* XXX *) with
            None       -> V.SkipChildren
          | Some _ -> V.ChangeTo (C.UNARY (C.MEMOF, e))
      else
        V.SkipChildren
    | _ -> V.DoChildren

  method vEnterScope () = ()
  method vExitScope  () = ()

end (* extract_activity_class *)

let rec extract_one def : C.definition list option =
  None

let rec extract_all defs : C.definition list =
  match defs with
      [] -> []
    | def::rest ->
      (match extract_one def with
          None -> def::(extract_all defs)
        | Some new_defs -> (extract_all new_defs) @ (extract_all rest))

let activity_extract cabs : (string * C.definition list) =
  let foo,defs = cabs in
  let defs' = extract_all defs in
  foo,defs'
