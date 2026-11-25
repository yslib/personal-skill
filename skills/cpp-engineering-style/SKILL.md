---
name: cpp-engineering-style
description: Apply Lucida's C++ engineering preferences to libraries, runtimes, SDKs, and multi-backend systems. Use also for Rust-only compiler and DSL projects, C++/Rust IR demos, lowering passes, opcode plans, and ports between languages. Design flat HIR/MIR/LIR storage with typed IDs, IndexedMap, side tables, kind traits, and per-stage builders. Covers CMake, language features, exception-free code, C ABI, ownership, sum types, and static dispatch.
---

# C++ engineering style

Start with the unit of change. For a library that will have multiple maintainers, consumers, or backends, make the build, ownership, and error contracts visible at their boundaries. For a small demo, use the few files and features its question requires. A single-target example of an explicit result does not need a plugin SDK or a module helper.

For compiler work, keep the AST recursive by default and give each requested lowered stage flat, ID-addressed storage, even in a small demo. Scale down the number of stages, kinds, and tables to fit the task. Read [Data-oriented IR and plans](references/ir-storage.md) before choosing the node representation. Use the same design for other pass-oriented graphs when their access patterns fit.

For a Rust IR implementation, follow the [Rust storage and builder conventions](references/rust-ir-storage.md). Apply the shared stage boundaries and choose the Rust construction API there. Use the C++ toolchain and ABI guidance when the project includes those boundaries.

## Agree on the project choices

Inspect the target's build files, public headers, callers, toolchains, and existing error paths. Before **implementing** an unresolved design choice, ask the user one short, concrete question showing the recommended option and its tradeoff. Bundle related choices. Wait for the answer on affected work. If the user explicitly requests a draft with assumptions or asks to skip questions, identify those assumptions and proceed. For a review, report findings without implying a change of policy.

| When the choice matters | Ask about these consequences |
| --- | --- |
| New long-lived library | Which artifacts ship independently? One SDK assembled from internal modules, or individually installable libraries? Who may include a private header? |
| Compiler matrix | Which host and device front ends, standard libraries, build configurations, and downstream consumers set the minimum? Prefer C++26 where they support the features used. |
| Backend and language boundary | Which backends are known at build time? Must a separately built plugin or another language call this code? What is the useful unit of dispatch? |
| Errors and allocation | Which targets belong to the exception-free low-level runtime? Must host OOM be recoverable? Do any dependencies lack a failure-returning API? |
| Lifetime and performance | Who owns each result and how long can work outlive the call? Is sharing cross-thread? Which calls are hot enough to measure? |

Ask only about decisions relevant to this task that the user or project has not already settled. For example, “Will backends ship independently? For a single binary I would instantiate CPU and GPU policies directly. For independently released plugins I would publish a narrow C operations table.” A small change inside an established module usually needs no questionnaire.

## Engineering items

| Item | Apply it when | Primary reference |
| --- | --- | --- |
| 1. Give modules a common build vocabulary. | Several modules share an include layout or packaging rule. | [Modules and CMake](references/modules.md), [reusable helper](assets/ProjectModule.cmake) |
| 2. Put the public contract where callers can see it. | A type or function gains an external caller or a privileged internal peer. | [Files and language features](references/source-style.md) |
| 3. Select modern language features per target. | Host, device, or installed headers impose different requirements. | [Files and language features](references/source-style.md) |
| 4. Represent recoverable errors explicitly. | Logic needs composable fallible operations, or an ABI caller needs a stable failure protocol. | [Errors and FFI](references/errors-and-ffi.md) |
| 5. Erase the backend once and specialize the work inside it. | A runtime discovers backends, or an algorithm already knows its vendor. | [Backends and ownership](references/backends-and-ownership.md) |
| 6. Make ownership transfers cheap and visible. | C clients or async operations keep a resource beyond a call. | [Backends and ownership](references/backends-and-ownership.md) |
| 7. Choose sum types by lifetime and interface. | A simple closed value fits `std::variant`; an engineered object family needs a domain-specific kind and checked downcasts. | [Sum types and type dispatch](references/sums-and-dispatch.md) |
| 8. Flatten composite type codes into static dispatch. | A batch kernel is specialized on several runtime type tags. | [Sum types and type dispatch](references/sums-and-dispatch.md), [working example](assets/TypeDispatch.hpp) |
| 9. Design IR storage around the passes that read it. | A compiler stage, IR demo, bytecode plan, or a port needs traversable records and explicit relationships. | [Data-oriented IR and plans](references/ir-storage.md), [Rust implementation](references/rust-ir-storage.md) |

For kuai-derived examples and differences between repository practice and these preferences, see the [source notes](references/kuai.md). Work in the user's language and maintain this skill in English.

## Use from another skill

Resolve `cpp-engineering-style` by its skill name when installed. In a checkout, open the `SKILL.md` in the sibling `cpp-engineering-style/` directory under `skills/`. The calling skill supplies the domain semantics, target language, existing project decisions, and question policy. Read only the engineering items relevant to that task. Resolve project constraints and explicit user choices before applying a personal default. If the guide is unavailable, use the calling skill's local instructions and name any material assumption. Return to the calling task once the C++ choices are settled.

## Check the result at its boundary

Pick the checks the change actually needs. For a build change, compile a consumer of the installed public target. For a new C API, compile a C caller, exercise a failure and its out-parameters, then destroy a successful result through the documented owner. For plugin or async work, cover an in-flight lifetime and teardown. For performance claims, name the counted operations or report a measurement. Tell the user which project choices were applied and what was verified.
