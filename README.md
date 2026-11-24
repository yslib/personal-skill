# personal-skill

I keep my coding preferences here so I can reuse them when working with AI coding agents. Each skill captures decisions I would otherwise have to explain in every project. A small example shows what those decisions look like at a call site.

## Example: from an OCaml prototype to Rust and C++

I use OCaml to sketch data types before choosing how to represent them in a production codebase. Consider a simple expression tree:

```ocaml
type expr =
  | Lit of int32
  | Add of expr * expr
```

In Rust, I would use an `Expr` enum and match on its variants. For an owned tree, the two operands of `Add` can each be a `Box<Expr>`. For a shared IR stored in an arena, I would use two `ExprId` values instead.

In C++, I prefer a base class that stores a kind tag and concrete classes such as `LitExpr` and `AddExpr`. A caller can write `if (const auto* add = expr.as<AddExpr>())` to inspect a node. An X-macro table can keep the kind declarations, type registrations, and dispatch cases in sync.

## Applying preferences

Target language, ownership model, naming style, and C++ standard can be specified per task. For example:

> Generate both versions. Use arena IDs for child nodes, follow the existing Rust naming, and ask me to choose the C++ accessor style before generating the headers.

An explicit request to change a default carries that preference into future tasks.
