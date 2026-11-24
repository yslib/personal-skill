# C++ objects, casts, and language features

## Integrate with kuai

These source links refer to `yslib/kuai` at `2b883e6`. Check the corresponding files on the target branch before generating code.

| Source | Conventions to reuse |
| --- | --- |
| [KuRtti.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuRtti.h) | `KuRtti<Kind>` stores the kind. `KU_RTTI_LEAF` tests equality. `KU_RTTI_RANGE` tests `begin < kind && kind < end`. The traits check the descriptor's `object_type` against the target to prevent a derived class from inheriting its parent's registration. `as<T>()` checks registration, domain, and inheritance, propagates const, and returns null on a runtime mismatch. `ku_isa<T>(ptr)` handles null. |
| [KuObjectDefs.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuObjectDefs.h), [KuObject.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuObject.h) | `KuObjectKind` is a domain-specific strong integer, `KuRttiKind<KuObjectKindTag>`. Section and item tables generate kind constants, range boundaries, and strings. Preserve assigned numbers and distinguish built-in leaves, user ranges, and boundary sentinels. `KuObject` registers itself as the `KU_UNKNOWN` leaf. Use an ordinary upcast from a leaf to the root. |
| [KuDeviceData.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuDeviceData.h), [KuTensor.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuTensor.h) | `KuTensor` passes `kTensor` to `KuDeviceData` during construction. The leaf and intermediate class register an exact kind and a range, respectively. The same object can be queried as either type. |
| [KuPointer.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuPointer.h), [KuRefCounted.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuRefCounted.h) | `KuObject` inherits both RTTI and reference counting. For intrusive ownership, reuse `ku_sp<T>` and `ku_make_sp<T>`. An `as<T>()` result borrows the object and leaves the reference count intact. Distinguish retain, adopt, and borrow at ownership boundaries. |
| [KuDebug.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuDebug.h) | `KU_ASSERT` skips evaluation under `NDEBUG`. `KU_VERIFY` always evaluates its expression, but release builds may discard the result. `KU_UNREACHABLE` triggers a failure. The `cast<T>()` defined in `KuRtti.h` checks inheritance statically. Use it where a dominating kind check already proves the dynamic type. |
| [ku_object.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/kuai_c/include/kuai/kuai_c/ku_object.h), [ku_object.rs](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/crates/kurt/src/ku_object.rs) | Public C kind numbers have a separate protocol. For example, `KU_OBJECT_TENSOR` is 2 and the internal `kTensor` is 1001. The safe Rust wrapper sits above the sys layer and explicitly matches raw external tags. Generate an adapter when the user requests cross-language interoperation. |
| [KurtBuildOptions.cmake](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/cmake/KurtBuildOptions.cmake) | This revision sets C++23 for MSVC and C++26 for the other branch. Check the target flags and probe individual features. |

## Choose the representation of a closed ADT

For a small local value sum, define a named payload per alternative and use `std::variant` with an exhaustive visitor. For a maintained object family with shared identity, private payloads, or typed queries across modules, use the tagged hierarchy below. The expression-tree asset illustrates that profile. For multiple runtime type codes selecting one specialized operation, follow Item 8 in [C++ engineering style](https://github.com/yslib/personal-skill/blob/main/skills/cpp-engineering-style/references/sums-and-dispatch.md).

### Generate a tagged object family

1. Define a domain-specific `enum class ExprKind` with one entry per constructor. Store the discriminator once in `Rtti<ExprKind>`. Let the root `Expr` and its leaves use the inherited tag. Use value representations for plain records, payload-free enums, and scalar wrappers. Add a fixed underlying width, explicit numbers, and boundary validation when an inter-module protocol needs stable numbers.
2. Generate mechanical kind declarations, type-to-kind registrations, names, or visitor arms from one X-macro list. Write fields, invariants, and domain behavior in ordinary C++. Give macros a domain prefix and `#undef` them after use. Check which callers must change when a list entry is added. Dispatch with an exhaustive switch. Reject unknown external integers at decode time and use the project's failure path for an unreachable internal tail.
3. Restrict root construction to the registered leaves so that callers cannot forge a kind. Disable public base copying and assignment to prevent slicing and preserve the agreement between tag and dynamic type. Use ordinary upcasts. Make `as<T>()` a const-preserving, nullable borrow. Reject unregistered and cross-domain targets at compile time. Document the precondition of `cast<T>()` and consider a debug assertion for internal proofs. Validate public inputs in release builds as well.
4. Give a generic family such as `'a expr` a domain identity for each instantiation. For example, use `ExprKindTag<T>` or restrict queries on `Expr<T>` to leaves with the same `T`. A `Lit` tag alone cannot justify converting `Lit<int>` to `Lit<double>`. For dynamically selected payload types, use a second discriminator or controlled type erasure.
5. Use RTTI for classification. Mark leaves `final` by default. A root deleted through `unique_ptr<Expr>` needs a virtual destructor. For arena-owned nodes, identify who runs nontrivial destructors. Add virtual business operations only when needed. An arena with strong IDs can own `unique_ptr<Expr>` or use the project's node pool, with edges storing IDs. Specify stable addresses only when the storage provides them.
6. Preserve `const` and `[[nodiscard]]`. Use `noexcept` only for operations guaranteed not to throw. Establish non-null edges during construction and prefer read-only accessors for immutable payloads. Preserve observable mutations for `mutable` fields. Generate arithmetic, comparisons, cloning, and traversal as the target semantics require.

## Select C++ features

| Use case | Implementation and fallback |
| --- | --- |
| Check an internal downcast precondition | Reuse the project's assertion helper. When contracts are requested, the build enables them, and `__cpp_contracts >= 202502L`, use `contract_assert(p != nullptr);`. Keep predicates free of side effects. Contract modes include ignore and observe. Use explicit control flow to reject invalid external input. |
| Enumerate declared enum values | With `<meta>` and `__cpp_impl_reflection`, use `std::meta::enumerators_of(^^ExprKind)` in a constant-evaluation context to verify enum metadata. The probe below works without expansion statements. Keep stable wire names, kind-to-leaf mappings, and assigned ranges in the domain table. |
| Share const and mutable accessor implementations | A C++23 explicit object parameter, `this Self&`, can share the implementation and restrict borrows from temporaries. For C++20, provide `&` and `const &` overloads and delete rvalue borrowing overloads. `std::expected` also belongs to C++23. |
| Choose a supported implementation | Compile a small feature probe using the target flags, standard library, and front end, including relevant CUDA host and device passes. Compile the new syntax only on a verified path. Preserve the API, failure semantics, and ABI on the fallback path. |

The following reflection example assumes a standalone, closed enum with no aliases.

```cpp
#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202603L && __has_include(<meta>)
#include <meta>
enum class ProbeKind { Lit, Add };
static_assert(std::meta::enumerators_of(^^ProbeKind).size() == 2);
#endif
```

In a build-time feature probe, add an `#else` with `#error` so that skipping the test body cannot count as success. In a compatibility header, use the fallback in that branch. See [contract evaluation](https://eel.is/c++draft/basic.contract.eval) and [reflection queries](https://eel.is/c++draft/meta.reflection.member.queries).
