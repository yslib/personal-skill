#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <tuple>
#include <utility>

namespace dispatch_example {

enum class Status : std::int32_t { ok, unsupported_type, unsupported_pair };

namespace detail {

// Wire IDs can have gaps. Their numeric values are independent of tuple order.
using Scalars = std::tuple<std::int32_t, float, double>;
constexpr std::size_t count = std::tuple_size_v<Scalars>;
template<class T> struct WireCode;
template<> struct WireCode<std::int32_t> { static constexpr int value = 10; };
template<> struct WireCode<float>        { static constexpr int value = 30; };
template<> struct WireCode<double>       { static constexpr int value = 90; };

// Each table position is the corresponding tuple ordinal.
inline constexpr auto wire_codes = []<std::size_t... I>(std::index_sequence<I...>) {
    return std::array{WireCode<std::tuple_element_t<I, Scalars>>::value...};
}(std::make_index_sequence<count>{});

constexpr std::size_t ordinal(int wire) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
        if (wire_codes[i] == wire) return i;
    }
    return count;
}

consteval bool valid_registry() {
    for (std::size_t i = 0; i < count; ++i) {
        if (ordinal(wire_codes[i]) != i) return false;
    }
    return true;
}
static_assert(valid_registry(), "wire codes must be unique");

template<std::size_t N, class Fn>
Status static_switch(std::size_t index, Fn&& fn) {
    static_assert(N > 0 && N <= 16);
    switch (index) {
#define DISPATCH_EXAMPLE_CASE(i) \
    case i: if constexpr ((i) < N) return std::forward<Fn>(fn).template operator()<(i)>(); break;
#define DISPATCH_EXAMPLE_FOUR(base) \
    DISPATCH_EXAMPLE_CASE(base)     \
    DISPATCH_EXAMPLE_CASE(base + 1) \
    DISPATCH_EXAMPLE_CASE(base + 2) \
    DISPATCH_EXAMPLE_CASE(base + 3)
        DISPATCH_EXAMPLE_FOUR(0)
        DISPATCH_EXAMPLE_FOUR(4)
        DISPATCH_EXAMPLE_FOUR(8)
        DISPATCH_EXAMPLE_FOUR(12)
#undef DISPATCH_EXAMPLE_FOUR
#undef DISPATCH_EXAMPLE_CASE
    }
    std::abort(); // Internal invariant: dispatch_pair validated both ordinals.
}

} // namespace detail

// The visitor implements operator()<Left, Right>() and returns Status.
// It can capture the already-validated batch/views by reference.
template<class Fn>
[[nodiscard]] Status dispatch_pair(int left_wire, int right_wire, Fn&& fn) {
    using detail::count;
    static_assert(count != 0 && count <= 16 / count, "dispatch domain exceeds the example switch");
    const auto left = detail::ordinal(left_wire);
    const auto right = detail::ordinal(right_wire);
    if (left == count || right == count) return Status::unsupported_type;

    const auto key = left * count + right;
    return detail::static_switch<count * count>(key, [&]<std::size_t K>() -> Status {
        using Left = std::tuple_element_t<K / count, detail::Scalars>;
        using Right = std::tuple_element_t<K % count, detail::Scalars>;
        return std::forward<Fn>(fn).template operator()<Left, Right>();
    });
}

} // namespace dispatch_example
