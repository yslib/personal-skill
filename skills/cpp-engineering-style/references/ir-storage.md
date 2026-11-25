# Item 9. Design IR storage around its passes

## Choose the unit of traversal

Start from a pass that will consume this stage. Identify what it visits in sequence, which relationships it follows, and what survives a rewrite. Use `AST → HIR → MIR → LIR → opcode plan` as the usual vocabulary. Implement the stages the task actually exercises. A one-stage interpreter demo can lower a recursive AST into one flat IR and execute it directly.

| Representation | Example of what the builder records |
| --- | --- |
| AST | Source nesting and syntax. Use recursive expressions and statements. Follow the parser's ownership conventions. |
| HIR | Resolved symbols and typed expressions when applicable. Preserve structured expressions, regions, and nesting with IDs and ranges of IDs. A nested `if` in the language can live in flat tables. |
| MIR | Operations and values. Add block order, successor edges, or SSA definitions where the chosen semantics need them. |
| LIR | Lowered operations and concrete operands such as slots, registers, buffers, or immediates, as the target requires. |
| Plan and executor | The plan owns the executable opcodes, operands, constants, and bindings it needs. The executor reads the plan and owns or borrows per-run state. Make any dependency on a longer-lived context explicit. |

## Give each identity one domain and one owner

Use names such as `hir::ExprId`, `mir::OpId`, `mir::ValueId`, and `lir::OpId`. Use separate strong types for stages and entity kinds. Keep the backing integer at the container and encoding boundary. Two stores of the same stage can still issue the same numeric ID. Keep an ID with its owning store or use an explicit remapping when moving between them.

Prefer an existing vector-backed `IndexedMap<Id, Record>` or its equivalent. For an owning entity table, expose allocation that returns the new ID, typed lookup, and dense iteration. For a side table, accept IDs already issued by the entity owner. In a declaration, `IndexedMap<OpId, OpHeader>` owns one row per op. A `vector<OpId>` in a block is an ordered list of references. Storage order can differ from instruction order, successor order, or execution order. Model each order the semantics observes.

Choose an ID lifetime that matches the passes. Append-only slots preserve indices across growth. Element references and spans can still expire when the backing arrays move. For removal or compaction, use the project's explicit invalidation strategy, such as rebuilding a destination store with an old-to-new map. Use generations only where IDs can survive slot reuse.

## Lay out one hot pass first

| Access pattern | Starting layout |
| --- | --- |
| A pass reads a small set of fields together on nearly every visit | A dense array of compact headers or records. Inline cheap, frequently used operands where it helps. |
| Only one kind or a few related kinds need a large payload | A typed payload table. Keep the payload index in the header and select the table through the kind. Several kinds may share a payload layout. |
| A relation is variable-length, e.g. call arguments or block members | A shared typed list pool and a typed range (offset and count). Copy input IDs into the owning pool when building. For frequent edits, build in a temporary list or choose an editable representation, then freeze if useful. |
| A minority of passes read metadata | A side table keyed by the entity ID. Use a dense parallel column for common entries, a sparse table for rare entries, and pass-local tables for derived facts. Recompute or invalidate derived facts after relevant rewrites. |

For example, a pass that counts calls can scan just the opcode field of the headers. A call-specific pass can then fetch the callee and its argument range. A diagnostic pass can separately look up source spans. Let these reads determine the split. A small fixed-arity demo may need only a dense header table and a couple of inline IDs. Add pools, payload tables, and separate columns when there is data to put in them. A C++ variant containing IDs and compact inline data can itself be a flat record.

## Put the opcode schema in one place

For C++, use a stage-local enum and traits keyed by its enumerators. Even for a short C++ demo, show one typed construction call from the lowering pass so the intended extension point is visible. Give each kind the facts that callers consume, such as its construction input, payload representation, operand/result shape, effects, terminator status, or verifier. Keep a fact in one schema and let construction, inspection, verification, or decoding share it. Introduce a definition list, X-macro, or generated switch when it removes real duplication.

For example, a small value-producing MIR builder could expose this C++ interface. Ordinary declarations and error types follow the project.

```cpp
enum class Op { Add, Call };
struct AddArgs  { ValueId lhs, rhs; };
struct CallArgs { SymbolId callee; ValueIdView arguments; };

template<Op> struct OpTraits;
template<> struct OpTraits<Op::Add> {
    using Args = AddArgs;
    static constexpr bool is_commutative = true;
};
template<> struct OpTraits<Op::Call> {
    using Args = CallArgs;
    static constexpr bool is_commutative = false;
};

class Builder {
public:
    explicit Builder(Storage& storage);
    template<Op K> ValueId emit(typename OpTraits<K>::Args args);
private:
    Storage& storage_;
};
```

At the lowering site, write `ValueId sum = builder.emit<Op::Add>({lhs, rhs});`. A canonicalizer for modular integer arithmetic can read `OpTraits<K>::is_commutative` to decide whether to normalize operand order. Assign properties according to the opcode's actual semantics.

Inside `emit<Op::Add>`, store the two value IDs inline if the pass reads them together. Inside `emit<Op::Call>`, copy the argument IDs from the borrowed view into the pool and store the callee and owned range in the call payload. Keep raw slot selection private. Return the identity the caller needs. A store, jump, or multi-result operation can have a different return contract. For a runtime opcode, dispatch once to a typed handler through an exhaustive switch or generated table. Keep the execution algorithm in the pass or executor.

## Build and read through the stage boundary

- Give each implemented layer a builder that borrows its storage. Put ID allocation, payload and pool insertion, and related table updates behind its operations. A HIR builder might expose `build<ExprKind::Call>(...)`, a MIR builder `emit<Op::Add>(...)`, and a plan builder label binding and fixups. Maintain a current block only in stages that use one.
- Lower from a read-only source view into the destination builder. Preserve sharing by mapping source IDs to destination IDs when semantics require it. Keep lowering policy in the lowering pass and storage invariants in the builder. Analyses can iterate typed IDs and obtain short-lived read views without a builder.
- Trace one shared value through construction and consumption. For `s = add(a, b); t = mul(s, s)`, the multiply holds the same `ValueId` twice and the stage owns one definition of `s`. For the operations present, check ID domains, pool bounds, kind/payload pairing, and stage-specific invariants such as valid branch targets. Run the smallest normal lowering-to-consumer example.
- Carry the layout and invariants across languages. In Rust, for example, use distinct ID newtypes, a typed vector wrapper, and an enum with exhaustive matching or typed builder methods. Select idiomatic dispatch and borrowing for the target language. Reuse this design for query plans and task DAGs when passes need the same identity, scan, and metadata patterns.

## Borrow small pieces from LLVM and MLIR

- Use LLVM's vector-backed dense key mapping as a precedent for an `IndexedMap`. Keep the project's existing key type and container interface. [LLVM Programmer's Manual, IndexedMap](https://llvm.org/docs/ProgrammersManual.html#llvm-adt-indexedmap-h)
- Give a block builder an insertion point when a lowering pass must place instructions in different blocks or in the middle of a block. A builder that always appends needs only its destination. [LLVM IRBuilderBase](https://llvm.org/doxygen/classllvm_1_1IRBuilderBase.html)
- Follow MLIR ODS's practice of declaring each operation's facts once. Express them with C++ enum-keyed specializations, typed arguments, and ordinary functions. Check local shape during construction and verify cross-operation facts, such as successor consistency, when the stage is complete. A new opcode should have an obvious place for its schema and an exhaustive dispatch site for each consumer that needs it. LLVM libraries, TableGen, and dialect infrastructure are optional. [MLIR Operation Definition Specification](https://mlir.llvm.org/docs/DefiningDialects/Operations/)
