val list_map : ('a -> 'b) -> 'a list -> 'b list

(** This has the semantics of (=) on OCaml 3.07 and earlier.  It can
   handle cyclic values as long as a structure in the cycle has a unique
   name or id in some field that occurs before any fields that have cyclic
   pointers. *)
val equals: 'a -> 'a -> bool

val (|-) : ( 'a -> 'b ) -> ( 'b -> 'c ) -> 'a -> 'c
val (-|) : ( 'b -> 'c ) -> ( 'a -> 'b ) -> 'a -> 'c
val map2 : ( 'a -> 'b ) -> ( 'a * 'a ) -> ( 'b * 'b )
val map3 : ( 'a -> 'b ) -> ( 'a * 'a * 'a ) -> ( 'b * 'b * 'b )
val opt_map : ( 'a -> 'b ) -> ( 'a option ) -> ( 'b option )
val opt_default : 'a -> ( 'a option ) -> 'a
val starts_with : string -> string -> bool
val after_prefix : string -> string -> string
