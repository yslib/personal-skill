#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

namespace example {

#define EXAMPLE_EXPR_ITEMS(X) \
    X(Unit, UnitExpr)         \
    X(Lit, LitExpr)           \
    X(Add, AddExpr)           \
    X(Located, LocatedExpr)

enum class ExprKind {
#define EXAMPLE_KIND(tag, type) tag,
    EXAMPLE_EXPR_ITEMS(EXAMPLE_KIND)
#undef EXAMPLE_KIND
};

class Expr;
template<class Kind>
struct RttiDomain;
template<>
struct RttiDomain<ExprKind> {
    using Root = Expr;
};

template<class T>
struct LeafTraits;

#define EXAMPLE_REGISTER(tag, type)                  \
    class type;                                      \
    template<> struct LeafTraits<type> {             \
        static constexpr auto kind = ExprKind::tag;  \
    };
EXAMPLE_EXPR_ITEMS(EXAMPLE_REGISTER)
#undef EXAMPLE_REGISTER

template<class T, class Kind>
concept RegisteredIn = requires {
    { LeafTraits<std::remove_cv_t<T>>::kind } -> std::same_as<const Kind&>;
};

template<class Kind>
class Rtti {
    friend typename RttiDomain<Kind>::Root;

    explicit constexpr Rtti(Kind kind) noexcept : m_kind(kind) {}
    const Kind m_kind;

protected:
    ~Rtti() = default;

public:
    Rtti(const Rtti&) = delete;
    Rtti& operator=(const Rtti&) = delete;

    [[nodiscard]] constexpr Kind getKind() const noexcept { return m_kind; }

    template<class T> requires RegisteredIn<T, Kind>
    [[nodiscard]] const T* as() const & noexcept {
        static_assert(std::derived_from<std::remove_cv_t<T>, Rtti>);
        return m_kind == LeafTraits<std::remove_cv_t<T>>::kind
            ? static_cast<const T*>(this) : nullptr;
    }

    template<class T> requires RegisteredIn<T, Kind>
    [[nodiscard]] T* as() & noexcept {
        return const_cast<T*>(std::as_const(*this).template as<T>());
    }

    template<class T> const T* as() const && = delete;

    template<class T> requires RegisteredIn<T, Kind>
    [[nodiscard]] const T& cast() const & noexcept {
        // Precondition: this live object has T's registered kind.
        const T* p = as<T>();
#if defined(EXAMPLE_USE_CONTRACTS) && EXAMPLE_USE_CONTRACTS
# if !defined(__cpp_contracts) || __cpp_contracts < 202502L
#  error "EXAMPLE_USE_CONTRACTS requires C++26 contracts"
# endif
        contract_assert(p != nullptr);
#else
        assert(p != nullptr);
#endif
        return *p;
    }

    template<class T> requires RegisteredIn<T, Kind>
    [[nodiscard]] T& cast() & noexcept {
        return const_cast<T&>(std::as_const(*this).template cast<T>());
    }

    template<class T> const T& cast() const && = delete;
};

// Only the registered leaves may initialize Expr's kind.
class Expr : public Rtti<ExprKind> {
#define EXAMPLE_FRIEND(tag, type) friend class type;
    EXAMPLE_EXPR_ITEMS(EXAMPLE_FRIEND)
#undef EXAMPLE_FRIEND
    explicit constexpr Expr(ExprKind kind) noexcept : Rtti(kind) {}

public:
    virtual ~Expr() = default;
};

class UnitExpr final : public Expr {
public:
    UnitExpr() noexcept : Expr(LeafTraits<UnitExpr>::kind) {}
};

class LitExpr final : public Expr {
public:
    explicit LitExpr(std::int32_t value) noexcept : Expr(LeafTraits<LitExpr>::kind), m_value(value) {}
    [[nodiscard]] std::int32_t value() const noexcept { return m_value; }

private:
    std::int32_t m_value;
};

using ExprPtr = std::unique_ptr<Expr>;

namespace detail {
// A null child has no corresponding OCaml value; every build rejects it.
inline ExprPtr requireChild(ExprPtr child) noexcept {
    if (!child) std::abort();
    return child;
}
} // namespace detail

class AddExpr final : public Expr {
public:
    AddExpr(ExprPtr left, ExprPtr right) noexcept
        : Expr(LeafTraits<AddExpr>::kind),
          m_left(detail::requireChild(std::move(left))),
          m_right(detail::requireChild(std::move(right))) {}

    [[nodiscard]] const Expr& left() const & noexcept { return *m_left; }
    [[nodiscard]] const Expr& right() const & noexcept { return *m_right; }
    const Expr& left() const && = delete;
    const Expr& right() const && = delete;

private:
    ExprPtr m_left;
    ExprPtr m_right;
};

class LocatedExpr final : public Expr {
public:
    LocatedExpr(std::int32_t line, ExprPtr inner) noexcept
        : Expr(LeafTraits<LocatedExpr>::kind), m_line(line),
          m_inner(detail::requireChild(std::move(inner))) {}

    [[nodiscard]] std::int32_t line() const noexcept { return m_line; }
    [[nodiscard]] const Expr& inner() const & noexcept { return *m_inner; }
    const Expr& inner() const && = delete;

private:
    std::int32_t m_line;
    ExprPtr m_inner;
};

template<class Visitor>
decltype(auto) visitExpr(const Expr& expr, Visitor&& visit) {
    switch (expr.getKind()) {
#define EXAMPLE_VISIT(tag, type) \
    case ExprKind::tag: return std::forward<Visitor>(visit)(expr.cast<type>());
        EXAMPLE_EXPR_ITEMS(EXAMPLE_VISIT)
#undef EXAMPLE_VISIT
    }
    std::abort();
}
#undef EXAMPLE_EXPR_ITEMS

struct CountNodes {
    std::size_t operator()(const UnitExpr&) const noexcept { return 1; }
    std::size_t operator()(const LitExpr&) const noexcept { return 1; }
    std::size_t operator()(const AddExpr&) const;
    std::size_t operator()(const LocatedExpr&) const;
};

inline std::size_t CountNodes::operator()(const AddExpr& expr) const {
    return 1 + visitExpr(expr.left(), *this) + visitExpr(expr.right(), *this);
}

inline std::size_t CountNodes::operator()(const LocatedExpr& expr) const {
    return 1 + visitExpr(expr.inner(), *this);
}

} // namespace example
