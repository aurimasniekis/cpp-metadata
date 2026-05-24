#pragma once

#include <md/object.hpp>
#include <md/value.hpp>

#include <array>
#include <charconv>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

namespace md {

namespace detail {

inline void write_json_string(std::ostream& os, const std::string_view s) {
    os.put('"');
    for (const char c : s) {
        switch (c) {
        case '"':
            os << "\\\"";
            break;
        case '\\':
            os << "\\\\";
            break;
        case '\b':
            os << "\\b";
            break;
        case '\f':
            os << "\\f";
            break;
        case '\n':
            os << "\\n";
            break;
        case '\r':
            os << "\\r";
            break;
        case '\t':
            os << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                static constexpr std::array<char, 16> hex = {
                    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
                os << "\\u00";
                os.put(hex[(static_cast<unsigned char>(c) >> 4U) & 0xFU]);
                os.put(hex[static_cast<unsigned char>(c) & 0xFU]);
            } else {
                os.put(c);
            }
        }
    }
    os.put('"');
}

// Shortest round-trip decimal via std::to_chars: produces the fewest digits
// that, when parsed back to the same type, recover the exact same value.
// So `3.14` (a double) prints as "3.14", not "3.1400000000000001".
template <class T>
inline void write_json_floating(std::ostream& os, const T v) {
    static_assert(std::is_floating_point_v<T>);
    std::array<char, 64> buf{};
    auto [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v);
    (void)ec;  // 64 bytes is enough for any IEEE 754 binary32/binary64 shortest form
    os.write(buf.data(), end - buf.data());
}

void write_json(std::ostream& os, const Value& v);

inline void write_json(std::ostream& os, const Array& a) {
    os.put('[');
    bool first = true;
    for (const auto& el : a) {
        if (!first) {
            os.put(',');
        }
        first = false;
        write_json(os, el);
    }
    os.put(']');
}

inline void write_json(std::ostream& os, const Object& o) {
    os.put('{');
    bool first = true;
    for (const auto& [k, val] : o) {
        if (!first) {
            os.put(',');
        }
        first = false;
        write_json_string(os, k);
        os.put(':');
        write_json(os, val);
    }
    os.put('}');
}

inline void write_json(std::ostream& os, const Value& v) {
    std::visit(
        [&](const auto& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                os << "null";
            } else if constexpr (std::is_same_v<T, bool>) {
                os << (x ? "true" : "false");
            } else if constexpr (std::is_same_v<T, std::int64_t> ||
                                 std::is_same_v<T, std::uint64_t>) {
                os << x;
            } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                write_json_floating(os, x);
            } else if constexpr (std::is_same_v<T, std::string>) {
                write_json_string(os, x);
            } else if constexpr (std::is_same_v<T, Array>) {
                write_json(os, x);
            } else if constexpr (std::is_same_v<T, std::unique_ptr<Object>>) {
                write_json(os, *x);
            }
        },
        v.raw());
}

}  // namespace detail

/// Stream `v` as compact JSON to `os`.
inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    detail::write_json(os, v);
    return os;
}
/// Stream `o` as compact JSON to `os`.
inline std::ostream& operator<<(std::ostream& os, const Object& o) {
    detail::write_json(os, o);
    return os;
}
/// Stream `a` as compact JSON to `os`.
inline std::ostream& operator<<(std::ostream& os, const Array& a) {
    detail::write_json(os, a);
    return os;
}

}  // namespace md
