#pragma once

#include <nlohmann/json.hpp>

#include <md/error.hpp>
#include <md/object.hpp>
#include <md/value.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace md {

namespace detail {

inline void to_nlohmann(nlohmann::json& j, const Value& v);

inline void to_nlohmann(nlohmann::json& j, const Array& a) {
    j = nlohmann::json::array();
    for (const auto& el : a) {
        nlohmann::json sub;
        to_nlohmann(sub, el);
        j.push_back(std::move(sub));
    }
}

inline void to_nlohmann(nlohmann::json& j, const Object& o) {
    j = nlohmann::json::object();
    for (const auto& [k, val] : o) {
        nlohmann::json sub;
        to_nlohmann(sub, val);
        j[k] = std::move(sub);
    }
}

inline void to_nlohmann(nlohmann::json& j, const Value& v) {
    if (v.is_null()) {
        j = nullptr;
    } else if (const auto* pb = v.as_bool_if()) {
        j = *pb;
    } else if (const auto* pi = v.as_int_if()) {
        j = *pi;
    } else if (const auto* pu = v.as_uint_if()) {
        j = *pu;
    } else if (const auto* pf = v.as_float_if()) {
        j = *pf;
    } else if (const auto* pd = v.as_double_if()) {
        j = *pd;
    } else if (const auto* ps = v.as_string_if()) {
        j = *ps;
    } else if (const auto* pa = v.as_array_if()) {
        to_nlohmann(j, *pa);
    } else if (const auto* po = v.as_object_if()) {
        to_nlohmann(j, *po);
    }
}

inline Value from_nlohmann(const nlohmann::json& j) {
    switch (j.type()) {
    case nlohmann::json::value_t::null:
        return Value{};
    case nlohmann::json::value_t::boolean:
        return Value{j.get<bool>()};
    case nlohmann::json::value_t::number_unsigned:
        return Value{j.get<std::uint64_t>()};
    case nlohmann::json::value_t::number_integer:
        return Value{j.get<std::int64_t>()};
    case nlohmann::json::value_t::number_float:
        return Value{j.get<double>()};
    case nlohmann::json::value_t::string:
        return Value{j.get<std::string>()};
    case nlohmann::json::value_t::array: {
        Array a;
        a.reserve(j.size());
        for (const auto& el : j) {
            a.emplace_back(from_nlohmann(el));
        }
        return Value{std::move(a)};
    }
    case nlohmann::json::value_t::object: {
        Object o;
        o.reserve(j.size());
        for (auto it = j.begin(); it != j.end(); ++it) {
            o.insert_or_assign(it.key(), from_nlohmann(it.value()));
        }
        return Value{std::move(o)};
    }
    default:
        throw type_error("md::from_json: unsupported nlohmann value type");
    }
}

}  // namespace detail

/// ADL hook: serialize an `md::Value` into a `nlohmann::json`.
inline void to_json(nlohmann::json& j, const Value& v) {
    detail::to_nlohmann(j, v);
}
/// ADL hook: deserialize a `nlohmann::json` into an `md::Value`.
inline void from_json(const nlohmann::json& j, Value& v) {
    v = detail::from_nlohmann(j);
}

/// ADL hook: serialize an `md::Object` into a `nlohmann::json`.
inline void to_json(nlohmann::json& j, const Object& o) {
    detail::to_nlohmann(j, o);
}
/// ADL hook: deserialize a `nlohmann::json` into an `md::Object`;
/// throws `type_error` if `j` isn't a JSON object.
inline void from_json(const nlohmann::json& j, Object& o) {
    Value v = detail::from_nlohmann(j);
    if (auto* p = v.as_object_if()) {
        o = std::move(*p);
    } else {
        throw type_error("md::from_json(Object): JSON value is not an object");
    }
}

// Convenience non-ADL forms — slightly nicer at call sites that already use
// `md::to_json(x)` / `md::from_json(j)`.
/// Convenience: return a fresh `nlohmann::json` for the given `md::Value`.
[[nodiscard]] inline nlohmann::json to_json(const Value& v) {
    nlohmann::json j;
    detail::to_nlohmann(j, v);
    return j;
}
/// Convenience: return a fresh `nlohmann::json` for the given `md::Object`.
[[nodiscard]] inline nlohmann::json to_json(const Object& o) {
    nlohmann::json j;
    detail::to_nlohmann(j, o);
    return j;
}
/// Convenience: deserialize a `nlohmann::json` into a new `md::Value`.
[[nodiscard]] inline Value from_json(const nlohmann::json& j) {
    return detail::from_nlohmann(j);
}

}  // namespace md
