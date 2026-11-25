---
name: rust-engineering-style
description: Apply Lucida's Rust engineering preferences to compiler and DSL projects, small IR demos, lowering passes, opcode plans, and the Rust side of cross-language ports. Use for data-oriented HIR, MIR, or LIR storage, typed entity IDs, side tables, enum record layouts, and per-stage builders.
---

# Rust engineering style

For a Rust compiler or IR demo, implement the stages the task exercises. Keep the source AST recursive where it helps the parser. Give each lowered body its own flat storage, connect records through typed IDs, and use Rust enums to describe the shape of a record.

Follow [IR storage and builders](references/ir-storage.md) to choose which entities need identity, place hot and cold data, and expose construction through a builder borrowing the destination. Trace one expression through lowering and a read-only consumer. Let an actual pass or generic caller justify additional tables or traits.

For a cross-language port, preserve the IR semantics and identity relationships. Apply these conventions to the Rust implementation and use the companion `cpp-engineering-style` skill for the C++ side.
