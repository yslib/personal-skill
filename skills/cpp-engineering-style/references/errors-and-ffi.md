# Item 4. Select the error representation at the boundary

## C++ logic and low-level runtime

For planning, configuration, resource acquisition, and orchestration, prefer `[[nodiscard]] std::expected<T, E>`. It is a sum type: success carries a value, failure carries an error. Use a small status or a typed diagnostic for `E` according to what callers can do with it. Translate once at the layer that has the context. For example, a factory can return `std::expected<OwnedDevice, Status>` and a status-returning caller can check `if (!device) return device.error();` before moving `*device`. Keep `std::expected` in C++ source-level APIs whose participants agree on the C++ environment. The abstraction can also work on hot paths. Assess its actual payload size, copies, and generated code there.

**Compile the low-level runtime with exceptions disabled.** Use explicit failure returns from the point of failure through the C entry point. Avoid throwing allocation and access paths, including operations hidden inside library calls. Use checked access or inspect `expected` before dereferencing it. `noexcept` alone only specifies how an escaping exception is handled. It does not turn throwing code into a failure-returning implementation.

Agree on the allocator contract. If OOM is recoverable, use a fallible allocator or `new (std::nothrow)` with a genuinely non-throwing constructor and fallible member initialization. A `std::vector` growth, a string allocation for diagnostics, or a throwing constructor still needs its own solution. If a dependency only exposes a throwing API, surface that incompatibility before selecting it for the low-level runtime. kuai's remaining throwing checks and catch blocks are implementation debt. Exclude them from new templates and generated examples.

## Publish a small, stable C failure protocol

Use a documented, fixed-width status such as `int32_t` plus explicit constants. Use a caller-provided output slot for a result and document its exact state on every return path. A create operation can clear a valid output slot on entry, acquire into a local RAII owner, and publish one owned handle only after success. If acquisition fails after a partial success, roll back earlier acquisitions. A sizing query can legitimately report the required count on `BUFFER_TOO_SMALL`. Specify this per operation.

- **Interoperability.** C, Rust, Python bindings, and independently compiled plugins can inspect the same integer without sharing C++ layouts, an exception runtime, or stack unwinding conventions. Stable numbers, calling convention, and supported platform ABI still need an explicit agreement.
- **Fast path.** A scalar return and caller-supplied slot permit allocation-free reporting of ordinary success or failure. Pay for detailed formatting only when requested. A status code alone can lose context: keep vendor details or a diagnostic sink available on the error path when supportability matters.
- **Error taxonomy.** Distinguish ordinary outcomes such as `NOT_READY`, recoverable errors, and violated internal invariants. Handle expected control states at the call site; assertions are useful for programmer errors whose conditions are already established. Public inputs that promise an error on invalid data need release-mode checks.
- **Async work.** Submission status says whether the operation was accepted. Completion carries the eventual result. Specify whether waiting for work also waits for callbacks to return, when buffers can be reused, and who keeps callback state alive.

## Separate propagation, classification, and retry policy

Keep one closed status catalog with explicit values if several outputs depend on it. Expand an X-macro list of `(name, number, text)` into C constants, diagnostic text, and exhaustive internal classifiers. Translate each vendor error in a typed function such as `to_status(native_status)`. Map unknown vendor codes to a documented generic failure, retaining the native detail when practical. Handle retry, polling, and buffer growth where their operation-specific policy is known.

For a status-returning function whose non-success outcomes should all propagate, use a small statement macro like this. Substitute the project's types and prefix.

```c
#define RT_RETURN_IF_ERROR(call_expr)                    \
    do {                                                 \
        const rt_status_t rt_checked_status_ = (call_expr); \
        if (rt_checked_status_ != RT_OK)                 \
            return rt_checked_status_;                   \
    } while (0)
```

Evaluate the call once and return the original numeric status. Reserve the macro's prefixed local name, document the early return, and keep it out of expressions. Before using it, establish RAII ownership in C++ or provide a common cleanup path in C. When an operation treats `NOT_READY` as progress, branch on that status explicitly before propagating other failures. A macro that emits enum cases and one that exits the caller solve different problems.

## Keep FFI calls coarse and borrowing precise

Use opaque, typed handles for identity, pointer-plus-count views for contiguous buffers, and explicit element type, shape, strides, byte sizes, memory space, and stream association where applicable. Empty views, nullability, alignment, and units belong to the operation's contract. For bulk work, pass a span or batch through one call. Document whether a function copies, borrows until return, or holds a lease until completion. Pin or retain an owner for deferred work, release it on completion, and make cancellation or failed submission release it exactly once. State whether callbacks can run inline or on another thread and whether they may re-enter the runtime.

For independently released producers, design ABI evolution up front: a versioned entry symbol or a negotiated prefix with a size/version, plus tests against the older contract. Read optional fields only after verifying the supplied size. Export the required C symbols with the platform's visibility and calling convention. Keep templates, STL objects, and ownership-specific C++ layouts private. Verify with a real C translation unit and measure dispatch count, copies, allocations, refcount traffic, and synchronization on representative payloads.
