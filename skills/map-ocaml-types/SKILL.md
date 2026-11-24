---
name: map-ocaml-types
description: Map OCaml type declarations, records, variants, and mutually recursive groups to Rust or C++, update existing mappings, and maintain Lucida's representation preferences. Use for ASTs, IRs, runtime objects, and data models that need Rust enums or C++ variants, kind tables, tag-based RTTI, and checked downcasts with configurable naming and ownership.
---

# Map OCaml types to Rust and C++

Use the OCaml declarations to determine which values exist and how they behave. Use the target project to determine API conventions, layout, and ownership. Before generating code, identify how a caller constructs a value, who owns it, and how another caller inspects it. Respond in the user's working language and keep project identifiers in their original spelling.

## Resolve options

1. Read the [personal defaults](assets/defaults.yaml) and [option and naming profiles](references/options.md). Accept natural language, key-value pairs, or YAML. A per-task override changes only the settings it specifies.
2. Inspect nearby type declarations, call sites, build options, and assertion helpers. Reuse existing types and utilities in a project. For a standalone example, select the applicable built-in profile.
3. Follow the `questions` setting. When naming is undecided, show two spellings of the same constructor and access expression so the user can pick a profile or change one part. When asked for a draft, proceed with known preferences and identify provisional choices.
4. Change a stored preference only when the user asks to make it a default or save it in the skill.

## Generate the mapping

1. Record each type's name, parameters, constructors, payload arity and grouping, field names and order, and mutability. Use `.mli` visibility and call sites to identify observable sharing, physical equality, encoding, and ABI requirements. For mutually recursive types, find every cycle in the by-value layout, accounting for indirection already supplied by containers or pointers.
2. Choose representations from the [mapping guide](references/mapping.md) for the requested languages. In Rust, prefer enums, structs, and exhaustive matches. In C++, use `std::variant` for a simple closed value sum and an RTTI base, one kind tag, and concrete classes for an engineered object family. Resolve `cpp.sum` from how callers use the type. Use lightweight value types for plain records and payload-free enums.
3. For maintained C++ targets, apply the shared [C++ engineering preferences](https://github.com/yslib/personal-skill/blob/main/skills/cpp-engineering-style/SKILL.md). Resolve the installed skill by name, `cpp-engineering-style`, or read its sibling directory in a checkout. Pass along ownership, toolchain, naming, and question settings already resolved here. Read its sum-type and flat-dispatch rules if combinations of runtime tags select typed operations. For a small standalone mapping, the local rules suffice. For C++, consult the [object model and C++26 guide](references/cpp.md). When targeting kuai, check the current branch and reuse its infrastructure. In a standalone project, take only the parts the data model needs.
4. Compare the [OCaml input](assets/expr.ml), [Rust output](assets/expr.rs), and [C++ output](assets/expr.hpp) for a complete example. They model a read-only expression tree with unique ownership and no external ABI. Adapt the representation to the actual options.
5. When updating generated code, identify added, removed, and renamed constructors. Update fields, dispatch, and callers together. Preserve published tag values and calling conventions, and explain intentional semantic changes. Edit files or return code blocks as requested, following the target project's layout.

## Check the result

- Trace one value from construction through an upcast or borrow, dispatch, and field access. Check that each source constructor is representable, payload arity survives, and mutually recursive definitions can be constructed and destroyed.
- Cover every Rust match arm. For C++, check a successful query, a wrong kind, const propagation, cross-domain misuse, destruction, and how a new branch surfaces in existing visitors. Exercise unknown raw tags when a decoder exists. If shared identity matters, test two parents that refer to the same node.
- Compile through the project's normal entry point when possible. For a standalone example, use `rustc --edition=2021 --crate-type lib` on the Rust file and a small translation unit that includes the C++ header. Keep build products in a temporary directory. Identify which configurations you ran and which you reviewed.
- Return the code or its location. Briefly explain choices that affect layout, ownership, ABI, or naming. A small declaration may need only the equivalent target types. To maintain this skill, edit the defaults file for preference changes and update the corresponding examples when a semantic rule changes.
