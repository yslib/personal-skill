# Options and naming profiles

All options are optional. Read their initial values from [defaults.yaml](../assets/defaults.yaml). A request such as “C++ only, prefix types with Ku and fields with m_, and generate it without asking” is equivalent to structured input. Merge nested mappings by key and replace lists as a whole. Report unknown keys or conflicting settings explicitly.

| Option | Values | How to apply it |
| --- | --- | --- |
| `targets` | `[cpp]`, `[rust]`, `[rust, cpp]` | Generate each listed target. |
| `questions` | `auto / always / never` | With `auto`, use known preferences and ask once about unresolved choices that affect the public API, ownership, or identity on a full generation task. With `always`, show the unresolved choices and recommended spellings, or confirm the complete settings if everything is specified. Wait for the user before generating. With `never`, proceed under stated assumptions. If a missing fact affects correctness, make the draft conditional on it. |
| `naming.cpp` | `project / kuai / standalone / snake` | With `project`, inspect nearby classes and callers. Fall back to `standalone` for an empty project. For `kuai`, follow the target module's vocabulary and prefixes. |
| `naming.rust` | `project / idiomatic` | Fall back to `idiomatic` for an empty project. |
| `naming.overrides` | Optional fields grouped by language | Support `type_case`, `type_prefix`, `variant_case`, `variant_prefix`, `method_case`, `field_case`, and `field_prefix`. Case values are `PascalCase`, `camelCase`, `snake_case`, and `SCREAMING_SNAKE_CASE`. Follow the existing API for boolean prefixes and free-function exceptions. |
| `ownership` | `auto / tree / shared / arena-id / borrowed` | With `auto`, inspect construction and use. Assume `tree` for a simple example with no observable identity. Use `shared` or `arena-id` for shared or hash-consed nodes. For `borrowed`, identify the owner and when the borrow becomes invalid. |
| `cpp.kind` | `auto / enum / strong-integer` | Use an enum for a closed standalone ADT and the existing strong integer type for a registered kuai Object domain. Follow the host protocol for open extensions. |
| `cpp.standard` | `project / 20 / 23 / 26` | Read the target's build configuration. Prefer C++26 for standalone output. Honor an explicitly requested lower standard. |
| `cpp.contracts` | `project / assert / cxx26` | Reuse project assertion helpers. Standalone code defaults to `assert`. With `cxx26`, use a compile probe before emitting `contract_assert` for side-effect-free internal invariants. Keep required runtime checks explicit. |
| `cpp.reflection` | `project / off / cxx26` | Follow the host convention. Standalone code defaults to an X-macro. With `cxx26`, derive facts the compiler can reflect and retain a table for domain-specific mappings. |

Preserve source semantics and the target API and ABI first. For remaining choices, use explicit task settings, nearby project code, personal defaults, then built-in fallbacks, in that order. After selecting a profile, apply its individual overrides. For example, `field_prefix: m_` applies to new fields. Renaming a published field requires an explicit request to change the API. Identify settings that cannot be satisfied together.

## Choose names at a call site

Use the same expression type to show what each profile looks like. The kuai row illustrates a potential new leaf. Check the actual kind domain and available numbers when integrating it.

| Profile | Types and constructors | Kind, type query, and field spelling |
| --- | --- | --- |
| C++ standalone | `Expr`, `AddExpr`, `ExprKind::Add` | `expr.getKind()`, `expr.as<AddExpr>()`, `m_left` |
| C++ kuai | `KuExpr`, `KuAddExpr`, `kAddExpr` (illustrative leaf constant) | `expr.getKind()`, `expr.as<KuAddExpr>()`, `m_left` |
| C++ snake | `expr`, `add_expr`, `expr_kind::add` | `node.get_kind()`, `node.as<add_expr>()`, `left_` |
| Rust idiomatic | `Expr`, `Expr::Add` | `match expr`, a named field such as `source_line`, or a positional pattern |

Apply type prefixes to type names. Configure kind spelling separately. Convert the field's case before adding `field_prefix`. An explicit field prefix replaces the snake profile's trailing underscore. For Rust keywords, use a valid raw identifier or a project-compatible alternative. Disambiguate names that collide after case conversion and record their source spellings.

Ask a concrete question such as “For this module, do you prefer `expr.getKind()` and `m_sourceLine`, or `expr.get_kind()` and `source_line_`? You can add `Ku` to type names independently. Rust will use `Expr::Add`.” Ask only about choices the user has left open.

## Override settings for one task

```yaml
targets: [cpp, rust]
questions: never
naming:
  cpp: standalone
  overrides:
    cpp:
      type_prefix: Ast
      field_case: snake_case
      field_prefix: m_
ownership: arena-id
cpp:
  standard: 26
  contracts: assert
```

This request produces names such as `AstExpr`, `AstAddExpr`, and `m_source_line`. Child edges use the strong type `AstExprId`. The next task starts from the stored defaults. If the user explicitly asks to use snake case for future standalone projects, set `naming.cpp` to `snake` in the defaults.
