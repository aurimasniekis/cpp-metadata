#pragma once

#include <md/object.hpp>
#include <md/ostream.hpp>
#include <md/value.hpp>

#include <format>
#include <sstream>
#include <string>

namespace md::detail {

template <class T>
struct CompactJsonFormatter {
    template <class ParseCtx>
    constexpr auto parse(ParseCtx& ctx) -> typename ParseCtx::iterator {
        auto it = ctx.begin();
        auto end = ctx.end();
        if (it != end && *it != '}') {
            throw std::format_error(
                "metadata formatter: only the default (empty) format spec is supported");
        }
        return it;
    }

    template <class FormatCtx>
    auto format(const T& v, FormatCtx& ctx) const -> typename FormatCtx::iterator {
        std::ostringstream os;
        detail::write_json(os, v);
        const std::string s = os.str();
        return std::ranges::copy(s, ctx.out()).out;
    }
};

}  // namespace md::detail

/// `std::format` specialization that emits an `md::Value` as compact JSON.
template <>
struct std::formatter<md::Value, char> : md::detail::CompactJsonFormatter<md::Value> {};

/// `std::format` specialization that emits an `md::Object` as compact JSON.
template <>
struct std::formatter<md::Object, char> : md::detail::CompactJsonFormatter<md::Object> {};

/// `std::format` specialization that emits an `md::Array` as compact JSON.
template <>
struct std::formatter<md::Array, char> : md::detail::CompactJsonFormatter<md::Array> {};
