# Items 7 and 8. Represent sums and dispatch on combinations

## Item 7. Choose a sum representation from its callers

| Shape | Default C++ representation | Consequences to discuss |
| --- | --- | --- |
| Alternatives carry no payload | `enum class` | Natural switch, no allocation; choose stable external numbers only for a protocol. |
| A small closed set of values with local consumers | `std::variant<A, B, ...>` and a typed visitor | Value semantics and local exhaustive handling. Account for the largest alternative, alignment, move/copy requirements, and a rebuild when alternatives change. Use distinct wrapper types when two alternatives carry the same primitive but have different meanings. |
| A maintained object family with identity, shared base behavior, arenas/handles, intermediate categories, or cross-module consumers | One domain-specific kind on a lightweight RTTI base, concrete payload classes, and a checked `as<T>()` | Stable object access without exposing a variant layout. Tag and dynamic type must agree. Define ownership separately. For a public ABI, expose opaque handles and an explicit tag protocol. |
| Success or error | `std::expected<T, E>` at a C++ logic boundary, fixed-width status at a C ABI | Preserve success and failure semantics directly. |

Size in lines or number of cases alone does not settle this. For `Literal | Parameter` local to a parser, ask whether either alternative actually needs persistent identity before introducing a base class. For runtime objects that must support `object.as<Tensor>()` and be retained by other modules, prefer the tagged family. Allow a large object to contain a small `variant` field.

In the tagged model, restrict who constructs an object with a particular kind. Check registration and domain at compile time; a checked query preserves const and returns null for a different live kind. Use an unchecked cast only after a dominating kind test. Keep the discriminant immutable across the object's lifetime. A single definition list can drive kind constants, type registrations, names, and exhaustive dispatch. Use range-based intermediate categories only if tag allocation preserves the hierarchy. Keep stable wire numbers independent of internal dense indices.

## Item 8. Flatten a finite product of type codes into one static dispatch

When an operation on a batch depends on multiple runtime type tags, collect its discriminating axes once. Examples include left and right scalar type, or scalar/tensor form for each argument. For two dense ordinal domains with cardinalities `L` and `R`:

1. Normalize each external type code to a validated dense ordinal. Check each axis individually before combining: an invalid right ordinal equal to `R` aliases the first case in the next left row. For sparse wire IDs use a mapping. For an ordered contiguous range, prove density against the compile-time type list and subtract its first code.
2. Encode `key = left * R + right` and verify that `L * R` fits the index type and the chosen dispatch implementation. More axes use mixed radix, e.g. `(a * B + b) * C + c`.
3. Use one runtime switch whose generated arms call `fn.template operator()<K>()` with a compile-time constant. Inside an arm, recover `Left = tuple_element_t<K / R, LeftTypes>` and `Right = tuple_element_t<K % R, RightTypes>`. Invoke a typed operation such as `apply<Vendor, Left, Right>(batch)`. One runtime-to-template dispatch now covers the batch.
4. For a mostly dense domain with a few unsupported combinations, use an `if constexpr` predicate to avoid instantiating an ill-formed kernel and return an unsupported-combination status. For a sparse legal set, register only supported tuples in a compact index or switch over generated legal keys. Keep genuinely dynamic dimensions as data.
5. Verify encode/decode for all legal tuples, reject invalid tags (including negative values and holes), and test unsupported combinations. Inspect dispatch overhead, binary size, compilation time, and behavior on real workloads. A switch may lower to a jump table or comparisons. A table of typed function-pointer thunks is another option when an indirect call and reduced inlining are acceptable.

[TypeDispatch.hpp](../assets/TypeDispatch.hpp) is a small standalone example with sparse wire codes, three C++ types, a nine-case product, and a compile-time registry check. kuai's `KuTypeVisit.h` proves density for its primitive type sequence, `KuObjectView.h` computes a pair index and separate scalar/tensor kind indices, and `KuStaticSwitch.h` generates the constant cases. Its upper bound of 512 is an implementation choice. Inspect actual axis sizes before adopting that bound.

The backend operations table and the type switch solve independent binding decisions. Use the table for a vendor that becomes known at runtime; inside that vendor, use type-code dispatch to enter the correct typed kernel. Avoid rechecking both decisions for every element.
