type expr =
  | Unit
  | Lit of int32
  | Add of expr * expr
  | Located of { line : int32; inner : expr }

let rec node_count = function
  | Unit | Lit _ -> 1
  | Add (left, right) -> 1 + node_count left + node_count right
  | Located { inner; _ } -> 1 + node_count inner

let example = Located { line = 12l; inner = Add (Lit 7l, Unit) }
