#pragma once

#include <md/object.hpp>
#include <md/value.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>

namespace md::detail {

inline std::size_t hash_combine(const std::size_t seed, const std::size_t v) noexcept {
    // Boost-style mix; documented as stable within a process only.
    return seed ^ (v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

// Per-alternative salt — keeps Value{0} (int), Value{0u} (uint), Value{0.0}
// (double), and Value{false} from colliding.
constexpr std::size_t alt_salt(const std::size_t idx) noexcept {
    constexpr std::size_t base = 0xcbf29ce484222325ULL;
    return base + (idx * 0x100000001b3ULL);
}

std::size_t hash_value(const Value& v) noexcept;

inline std::size_t hash_array(const Array& a) noexcept {
    std::size_t h = alt_salt(7);
    for (const auto& el : a) {
        h = hash_combine(h, hash_value(el));
    }
    return h;
}

// Order-independent so two Objects that compare equal hash equal regardless
// of insertion order.
inline std::size_t hash_object(const Object& o) noexcept {
    std::size_t acc = 0;
    for (const auto& [k, val] : o) {
        std::size_t entry = std::hash<std::string_view>{}(std::string_view{k});
        entry = hash_combine(entry, hash_value(val));
        acc ^= entry;
    }
    // Mix in a non-zero salt so an empty object hashes distinctly from "all
    // entry-hashes XORed to zero by accident".
    return acc ^ alt_salt(8);
}

inline std::size_t hash_value(const Value& v) noexcept {
    return std::visit(
        [&](const auto& x) noexcept -> std::size_t {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                return alt_salt(0);
            } else if constexpr (std::is_same_v<T, bool>) {
                return hash_combine(alt_salt(1), std::hash<bool>{}(x));
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                return hash_combine(alt_salt(2), std::hash<std::int64_t>{}(x));
            } else if constexpr (std::is_same_v<T, std::uint64_t>) {
                return hash_combine(alt_salt(3), std::hash<std::uint64_t>{}(x));
            } else if constexpr (std::is_same_v<T, float>) {
                return hash_combine(alt_salt(4), std::hash<float>{}(x));
            } else if constexpr (std::is_same_v<T, double>) {
                return hash_combine(alt_salt(5), std::hash<double>{}(x));
            } else if constexpr (std::is_same_v<T, std::string>) {
                return hash_combine(alt_salt(6),
                                    std::hash<std::string_view>{}(std::string_view{x}));
            } else if constexpr (std::is_same_v<T, Array>) {
                return hash_array(x);
            } else if constexpr (std::is_same_v<T, std::unique_ptr<Object>>) {
                return hash_object(*x);
            } else {
                return 0;
            }
        },
        v.raw());
}

}  // namespace md::detail

/// `std::hash` specialization for `md::Value` (stable within a single process).
template <>
struct std::hash<md::Value> {
    /// Hash an `md::Value`.
    std::size_t operator()(const md::Value& v) const noexcept {
        return md::detail::hash_value(v);
    }
};

/// `std::hash` specialization for `md::Object`; order-independent so
/// equal objects hash equal regardless of insertion order.
template <>
struct std::hash<md::Object> {
    /// Hash an `md::Object`.
    std::size_t operator()(const md::Object& o) const noexcept {
        return md::detail::hash_object(o);
    }
};

/// `std::hash` specialization for `md::Array`.
template <>
struct std::hash<md::Array> {
    /// Hash an `md::Array`.
    std::size_t operator()(const md::Array& a) const noexcept {
        return md::detail::hash_array(a);
    }
};
