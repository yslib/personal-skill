# Personal skills

I keep one reusable coding task per directory under [skills/](skills/). Each skill has a `SKILL.md` entry point and keeps its mapping rules, defaults, and examples alongside it.

## Skill index

| Skill | What it does |
| --- | --- |
| [Map OCaml types to Rust and C++](skills/map-ocaml-types/SKILL.md) | Take type definitions from a working OCaml prototype and generate Rust enums and structs or C++ types with kind tags and checked downcasts. Choose the target language, ownership model, and naming conventions per task. |

To see how one recursive type maps across languages, compare the [OCaml prototype](skills/map-ocaml-types/assets/expr.ml), [Rust enum](skills/map-ocaml-types/assets/expr.rs), and [C++ object hierarchy](skills/map-ocaml-types/assets/expr.hpp). The skill's [options](skills/map-ocaml-types/references/options.md) describe how to override the [defaults](skills/map-ocaml-types/assets/defaults.yaml) for one run.
