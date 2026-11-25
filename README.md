# Personal skills

I keep one reusable coding task per directory under [skills/](skills/). I write skill instructions, references, metadata, and examples in English. Each skill has a `SKILL.md` entry point and keeps its mapping rules, defaults, and examples alongside it.

## Skill index

| Skill | What it does |
| --- | --- |
| [C++ engineering style](skills/cpp-engineering-style/SKILL.md) | Choose conventions for a maintained C++ project, including CMake modules, exception-free runtimes, C ABI and ownership, sum types, and flat dispatch over type-code combinations. For compiler work, see [flat IR storage and builders](skills/cpp-engineering-style/references/ir-storage.md) and [Rust IR storage and builders](skills/cpp-engineering-style/references/rust-ir-storage.md). Includes a [module helper](skills/cpp-engineering-style/assets/ProjectModule.cmake) and a [working dispatch example](skills/cpp-engineering-style/assets/TypeDispatch.hpp). |
| [Map OCaml types to Rust and C++](skills/map-ocaml-types/SKILL.md) | Take type definitions from a working OCaml prototype and generate Rust enums and structs or C++ values and tagged object hierarchies. Reuses the C++ engineering skill for production targets. Choose the target language, ownership model, and naming conventions per task. |

To see how one recursive type maps across languages, compare the [OCaml prototype](skills/map-ocaml-types/assets/expr.ml), [Rust enum](skills/map-ocaml-types/assets/expr.rs), and [C++ object hierarchy](skills/map-ocaml-types/assets/expr.hpp). The skill's [options](skills/map-ocaml-types/references/options.md) describe how to override the [defaults](skills/map-ocaml-types/assets/defaults.yaml) for one run.
