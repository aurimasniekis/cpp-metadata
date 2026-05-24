#pragma once

#include <md/object.hpp>
#include <md/value.hpp>

#include <string>
#include <string_view>

namespace md {

// Free-function counterparts to Object methods, for users who prefer the
// functional style. Each is a thin forward.

/// True if `key` is present in `o`.
[[nodiscard]] inline bool contains(const Object& o, const std::string_view key) {
    return o.contains(key);
}

/// Return a pointer to the value for `key`, or `nullptr` if absent.
[[nodiscard]] inline Value* find_ptr(Object& o, const std::string_view key) {
    return o.find_ptr(key);
}
/// Return a pointer to the value for `key`, or `nullptr` if absent.
[[nodiscard]] inline const Value* find_ptr(const Object& o, const std::string_view key) {
    return o.find_ptr(key);
}

/// Return the value for `key`; throws `missing_key_error` if absent.
inline Value& require(Object& o, const std::string_view key) {
    return o.require(key);
}
/// Return the value for `key`; throws `missing_key_error` if absent.
inline const Value& require(const Object& o, const std::string_view key) {
    return o.require(key);
}

/// Return the string at `key`; throws on missing key or type mismatch.
inline std::string& require_string(Object& o, const std::string_view key) {
    return o.require_string(key);
}
/// Return the string at `key`; throws on missing key or type mismatch.
inline const std::string& require_string(const Object& o, const std::string_view key) {
    return o.require_string(key);
}

/// Return the array at `key`; throws on missing key or type mismatch.
inline Array& require_array(Object& o, const std::string_view key) {
    return o.require_array(key);
}
/// Return the array at `key`; throws on missing key or type mismatch.
inline const Array& require_array(const Object& o, const std::string_view key) {
    return o.require_array(key);
}

/// Return the nested object at `key`; throws on missing key or type mismatch.
inline Object& require_object(Object& o, const std::string_view key) {
    return o.require_object(key);
}
/// Return the nested object at `key`; throws on missing key or type mismatch.
inline const Object& require_object(const Object& o, const std::string_view key) {
    return o.require_object(key);
}

/// Pointer to the string at `key`, or `nullptr` if absent or wrong type.
[[nodiscard]] inline const std::string* get_string_if(const Object& o, const std::string_view key) {
    return o.get_string_if(key);
}
/// Pointer to the array at `key`, or `nullptr` if absent or wrong type.
[[nodiscard]] inline const Array* get_array_if(const Object& o, const std::string_view key) {
    return o.get_array_if(key);
}
/// Pointer to the nested object at `key`, or `nullptr` if absent or wrong type.
[[nodiscard]] inline const Object* get_object_if(const Object& o, const std::string_view key) {
    return o.get_object_if(key);
}

/// Deep-merge `src` into `dst`; see `Object::merge`.
inline void merge(Object& dst, const Object& src) {
    dst.merge(src);
}

}  // namespace md
