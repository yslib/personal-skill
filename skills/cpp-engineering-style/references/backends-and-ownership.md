# Items 5 and 6. Bind the backend and account for each reference

## Item 5. Select runtime type erasure or static polymorphism by binding time

| What the caller knows | Representation | Cost and use |
| --- | --- | --- |
| A runtime chooses a backend, or a client speaks C | An opaque handle identifies the object. A borrowed context and a C function-pointer table erase the backend type. This is an explicit vtable, also called an operations table. | A call through a slot normally uses an indirect branch. Make the unit a batch, launch, transfer, or allocation. |
| An implementation already knows the vendor at compile time | Parameterize the implementation on a vendor policy or traits specialization, e.g. `reduce<Vendor, T>`. Constrain the operations where helpful. | The concrete callee is statically known, giving the compiler an opportunity to inline it. Specializations cost compilation time and text size. |
| The host loads a plugin and the plugin implements typed algorithms | Bind the C slots to small adapter thunks instantiated for the concrete vendor. Inside the thunk, recover the correct context and call a policy operation. | Dispatch dynamically at the external boundary, then carry the concrete types through the algorithm. |

A handle hides representation. A table chooses behavior. A static policy binds an implementation. Each answers a different question. CRTP is one technique for sharing static wrappers among vendor policies; a concrete type with static functions or a traits specialization is often sufficient.

**A concrete call in kuai.** The host device calls `m_vendorApi.stream_create(m_vendorApi.ctx, &m_defaultStream)`. The NVIDIA module builds the table with `KuVendor::makeApi(&state)`. Its `stream_create` slot points to `KuVendorApiThunk<Vendor, &Vendor::onKuStreamCreate>::invoke`, which invokes the selected operation and normalizes its native result with `Vendor::toStatus`. Separately, algorithms with a known `Vendor` use static vendor traits, as `algo::transform` does. One static wrapper per operation keeps SDK types and platform-specific includes on the implementation side. See the [source notes](kuai.md).

Define the lifetime and thread-safety of both the table and its context. Copying table entries still borrows their function addresses and context. Hold the provider module until all handles, callbacks, and in-flight work that can call its code are gone. Validate required slots when accepting a table. If a single binary already knows its only backend, use the policy directly. A header-only inner loop does not need a plugin interface merely to select a template argument.

## Item 6. Choose ownership from the actual retain/release contract

Prefer a value, unique owner, or borrow when that expresses the lifetime. Use **intrusive reference counting** when the object representation is under the library's control and several C or language-binding clients need shared ownership. Place the count in the object and expose typed `retain` and `release` operations. Wrap the same pointer in an RAII owner inside C++. The exporting module performs final destruction with its matching allocator.

| Operation | Reference-count effect | Caller contract |
| --- | --- | --- |
| Create or publish an owned handle | One live reference is transferred on success | Receiver releases once; failure leaves the documented output state. |
| Borrow or call `get()` | None | Use only while the guaranteeing owner and relevant operation are live. |
| Retain or copy an owning wrapper | Increment once | Acquire from a live reference; never resurrect a released object. |
| Adopt an already-owned reference | None | Construct the wrapper without incrementing again. |
| Move, detach, or transfer to C | None when a reference is transferred | Invalidate the old ownership slot and give the receiver its release obligation. |
| Release | Decrement once; final release destroys | Ensure work and callbacks still using the object hold their own lifetime guarantee. |

This avoids a separate control-block allocation and allows one-pointer handles and allocation-free borrowing. The object still stores a count, and retaining or releasing it on multiple threads can generate contended atomic traffic. A thread-safe reference count protects lifetime, not payload mutation. Review initial count, memory ordering, final deletion, and cycles. Separate back-edges or use an explicit weak-reference design when cycles are possible. In a hot loop, borrow from a surrounding owner rather than repeatedly copying an owning wrapper. A parent-owned device may only need a borrowed handle whose lifetime is tied to the instance.
