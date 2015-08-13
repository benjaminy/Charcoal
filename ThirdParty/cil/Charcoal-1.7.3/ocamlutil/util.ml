let list_array_map f l =
  Array.to_list (Array.map f (Array.of_list l))
 
let rec count_map f l ctr =
  match l with
  | [] -> []
  | [x] -> [f x]
  | [x;y] ->
          (* order matters! *)
          let x' = f x in
          let y' = f y in
          [x'; y']
  | [x;y;z] ->
          let x' = f x in
          let y' = f y in
          let z' = f z in
          [x'; y'; z']
  | x :: y :: z :: w :: tl ->
          let x' = f x in
          let y' = f y in
          let z' = f z in
          let w' = f w in
          x' :: y' :: z' :: w' ::
      (if ctr > 500 then list_array_map f tl
       else count_map f tl (ctr + 1))
 
let list_map f l = count_map f l 0

let equals x1 x2 : bool =
  (compare x1 x2) = 0

let (|-) f1 f2 x = f2 ( f1 x )
let (-|) f1 f2 x = f1 ( f2 x )
let map2 f ( x1, x2 ) = ( f x1, f x2 )
let map3 f ( x1, x2, x3 ) = ( f x1, f x2, f x3 )
let opt_map f x = match x with None -> None | Some y -> Some( f y )
let opt_default d x = match x with None -> d | Some y -> y
let starts_with haystack needle =
  try needle = String.sub haystack 0 ( String.length needle )
  with Invalid_argument _ -> false
let after_prefix s prefix =
  let p = String.length prefix in
  try String.sub s p ( String.length s - p )
  with Invalid_argument _ -> ""
