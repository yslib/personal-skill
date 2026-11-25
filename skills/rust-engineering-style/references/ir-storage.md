# Rust IR storage and builders

Keep the source AST recursive where it helps the parser. Give each implemented HIR, MIR, or LIR body its own flat storage, including in demos. Lower from a shared borrow of the previous stage into a builder borrowing the destination mutably. When a stage emits an executable plan, keep its permanent opcode data in the plan and per-run values in the executor.

## Choose which records need identities

- Use distinct, cheaply copied ID newtypes such as `hir::ExprId`, `mir::InstId`, and `mir::ValueId`. Let the owning table issue them. Prefer an existing typed vector such as rustc's `IndexVec<K, V>` or Cranelift's `PrimaryMap<K, V>`. Use `SecondaryMap<K, T>` or another ID-keyed table to attach facts to existing entities. Keep the underlying integer and any checked narrowing inside the container boundary.
- Give each ID an owner and a validity period. Two functions can both have an instruction numbered zero. Keep IDs scoped to their function, preserve indices during append, and remap live IDs when compacting. Passes can keep an ID across edits if its slot stays valid. Borrow the record only for the read or write. For an in-place rewrite, copy the operand IDs you need, end that borrow, then mutate the store.
- Keep statements directly in a block's `Vec<Statement>` if they are only consumed in block order. Introduce an instruction table when instructions need independent identity for cross-references, reordering, or analysis. In that design, a block's `Vec<InstId>` is an order, the instruction table owns the records, and CFG successors are explicit `BlockId` edges. A separate layout becomes useful when transformations move instructions without changing their identities.

## Choose the row shape from a reader

For HIR, store expressions in a typed vector and let an expression kind such as `Binary { op, lhs: ExprId, rhs: ExprId }` encode its links. An enum of records with ID fields supports a flat table. For SSA, distinguish `InstId` from `ValueId` when a value can be a block parameter, a particular instruction result, or one of several results.

| Data | Suggested starting point |
| --- | --- |
| Integer add and multiply | Use a shared binary format such as `Binary { op: BinOp, args: [ValueId; 2] }`. Inline both IDs for a pass that inspects the operands together. |
| Variable-length call arguments | Accept a borrowed slice in the builder and copy its IDs into an owned list or list pool. Store a compact range or list handle when pooling pays off. |
| A rare kind with a large inline payload | Measure `size_of::<InstData>()` and time a representative pass. Move the payload to a typed table if it inflates the stride of ordinary instructions enough to matter. |
| Source spans, annotations, or derived analysis | Key the table by an existing ID. Choose dense or sparse storage by occupancy. Keep pass-local facts with the pass and invalidate them after relevant edits. |

## Expose typed construction in idiomatic Rust

Give each stage a builder holding `&mut Body` or `&mut Function`. Keep the current block only where insertion order needs it. Start with named methods. For example, `builder.iadd(a, b)` returns a value, `builder.call(callee, args)` follows the call's result convention, and `builder.ret(value)` ends the current block. Share insertion logic privately between operations with the same physical format.

Put properties needed by runtime passes in an exhaustive `match` on the opcode, behind a method such as `Opcode::properties()`. Introduce a zero-sized marker and an `OpSpec` trait with associated `Args` and `Output` when a concrete generic consumer needs `emit::<IAdd>(args)`. This is the Rust counterpart to C++ enum-keyed traits. If several consumers duplicate the opcode schema, use one declarative macro or generation step to keep their definitions in sync.

Trace `let s = a + b in s * s` through lowering. Bind the result of `builder.iadd(a, b)` once, then pass that same `ValueId` to both arguments of `builder.imul(s, s)`. Check that the two uses resolve to one definition and that a read-only consumer can traverse the finished body after the builder borrow ends.

## Implementation precedents

Use rustc THIR as a reference for ID-linked expression records and typed tables. Its HIR also uses borrowed child expressions. Its MIR illustrates block-local statement vectors. Pick the appropriate granularity for each stage. [rustc THIR tables](https://doc.rust-lang.org/nightly/nightly-rustc/rustc_middle/thir/struct.Thir.html) · [THIR expression kinds](https://doc.rust-lang.org/nightly/nightly-rustc/rustc_middle/thir/enum.ExprKind.html) · [rustc MIR](https://rustc-dev-guide.rust-lang.org/mir/index.html)

Consult Cranelift when an SSA layer needs data-flow entities, a separate program order, dense side tables, pooled lists, or a named instruction-building API. Reuse the project's existing containers where they fit. [Cranelift entities](https://docs.rs/cranelift-entity/latest/cranelift_entity/) · [DataFlowGraph](https://docs.rs/cranelift-codegen/latest/cranelift_codegen/ir/dfg/struct.DataFlowGraph.html) · [InstBuilder](https://docs.rs/cranelift-codegen/latest/cranelift_codegen/ir/trait.InstBuilder.html)
