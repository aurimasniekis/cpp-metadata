#pragma once

#include <md/error.hpp>
#include <md/value.hpp>

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

namespace md {

/// Ordered-by-insertion-time-ish string-keyed map of `Value`s, with
/// transparent string-view lookup and JSON-flavored metadata helpers.
class Object {
public:
    /// Underlying associative container type.
    using map_type =
        std::unordered_map<std::string, Value, detail::TransparentStringHash, std::equal_to<>>;

    /// Key type (always `std::string`).
    using key_type = map_type::key_type;
    /// Mapped value type (always `Value`).
    using mapped_type = map_type::mapped_type;
    /// `std::pair<const key_type, mapped_type>`.
    using value_type = map_type::value_type;
    /// Unsigned size type.
    using size_type = map_type::size_type;
    /// Signed difference type for iterators.
    using difference_type = map_type::difference_type;
    /// Hash functor type.
    using hasher = map_type::hasher;
    /// Key-equality functor type.
    using key_equal = map_type::key_equal;
    /// Reference to a stored `value_type`.
    using reference = map_type::reference;
    /// Const reference to a stored `value_type`.
    using const_reference = map_type::const_reference;
    /// Pointer to a stored `value_type`.
    using pointer = map_type::pointer;
    /// Const pointer to a stored `value_type`.
    using const_pointer = map_type::const_pointer;
    /// Mutable iterator type.
    using iterator = map_type::iterator;
    /// Const iterator type.
    using const_iterator = map_type::const_iterator;

    /// Construct an empty `Object`.
    Object() = default;

    /// Construct from a braced list of key/value pairs.
    Object(const std::initializer_list<value_type> il) : map_(il) {}

    /// Construct from an iterator range of key/value pairs.
    template <class InputIt>
    Object(InputIt first, InputIt last) : map_(first, last) {}

    // --- map-like surface ---------------------------------------------

    /// Iterator to the first element.
    iterator begin() noexcept {
        return map_.begin();
    }
    /// Const iterator to the first element.
    const_iterator begin() const noexcept {
        return map_.begin();
    }
    /// Const iterator to the first element.
    const_iterator cbegin() const noexcept {
        return map_.cbegin();
    }
    /// Iterator past the last element.
    iterator end() noexcept {
        return map_.end();
    }
    /// Const iterator past the last element.
    const_iterator end() const noexcept {
        return map_.end();
    }
    /// Const iterator past the last element.
    const_iterator cend() const noexcept {
        return map_.cend();
    }

    /// True if the object has no entries.
    [[nodiscard]] bool empty() const noexcept {
        return map_.empty();
    }
    /// Number of entries.
    [[nodiscard]] size_type size() const noexcept {
        return map_.size();
    }

    /// Remove all entries.
    void clear() noexcept {
        map_.clear();
    }
    /// Reserve storage for at least `n` entries.
    void reserve(const size_type n) {
        map_.reserve(n);
    }

    /// Access (and default-insert if missing) the value for `key`.
    Value& operator[](const std::string_view key) {
        if (const auto it = map_.find(key); it != map_.end()) {
            return it->second;
        }
        return map_.emplace(std::string(key), Value{}).first->second;
    }
    /// Access (and default-insert if missing) the value for `key`.
    Value& operator[](const std::string& key) {
        return map_[key];
    }
    /// Access (and default-insert if missing) the value for `key`.
    Value& operator[](std::string&& key) {
        return map_[std::move(key)];
    }
    /// Access (and default-insert if missing) the value for `key`.
    Value& operator[](const char* key) {
        return (*this)[std::string_view{key}];
    }

    /// Access the value for `key`; throws `std::out_of_range` if missing.
    Value& at(const std::string_view key) {
        if (const auto it = map_.find(key); it != map_.end()) {
            return it->second;
        }
        throw std::out_of_range("Object::at: key not found");
    }
    /// Access the value for `key`; throws `std::out_of_range` if missing.
    const Value& at(const std::string_view key) const {
        if (const auto it = map_.find(key); it != map_.end()) {
            return it->second;
        }
        throw std::out_of_range("Object::at: key not found");
    }

    /// Find an entry by key, returning `end()` on miss.
    iterator find(const std::string_view key) {
        return map_.find(key);
    }
    /// Find an entry by key, returning `end()` on miss.
    const_iterator find(const std::string_view key) const {
        return map_.find(key);
    }

    /// Return `1` if `key` is present, otherwise `0`.
    [[nodiscard]] size_type count(const std::string_view key) const {
        return map_.count(key);
    }

    /// Insert or overwrite the entry for `k`.
    template <class K, class V>
    std::pair<iterator, bool> insert_or_assign(K&& k, V&& v) {
        return map_.insert_or_assign(std::forward<K>(k), std::forward<V>(v));
    }

    /// Construct an entry in place; no effect if `k` already exists.
    template <class K, class V>
    std::pair<iterator, bool> emplace(K&& k, V&& v) {
        return map_.emplace(std::forward<K>(k), std::forward<V>(v));
    }

    /// Insert an entry; no effect if the key already exists.
    std::pair<iterator, bool> insert(const value_type& v) {
        return map_.insert(v);
    }
    /// Insert an entry; no effect if the key already exists.
    std::pair<iterator, bool> insert(value_type&& v) {
        return map_.insert(std::move(v));
    }

    /// Erase the entry for `key`; returns `1` if removed, otherwise `0`.
    size_type erase(const std::string_view key) {
        if (const auto it = map_.find(key); it != map_.end()) {
            map_.erase(it);
            return 1;
        }
        return 0;
    }
    /// Erase the entry at `pos`; returns the next iterator.
    iterator erase(const const_iterator pos) {
        return map_.erase(pos);
    }

    // --- metadata helpers (methods) -----------------------------------

    /// True if `key` is present in the object.
    [[nodiscard]] bool contains(const std::string_view key) const {
        return map_.contains(key);
    }

    /// Return a pointer to the value for `key`, or `nullptr` if absent.
    [[nodiscard]] Value* find_ptr(const std::string_view key) {
        if (const auto it = map_.find(key); it != map_.end()) {
            return &it->second;
        }
        return nullptr;
    }
    /// Return a pointer to the value for `key`, or `nullptr` if absent.
    [[nodiscard]] const Value* find_ptr(const std::string_view key) const {
        if (const auto it = map_.find(key); it != map_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    /// Return the value for `key`; throws `missing_key_error` if absent.
    Value& require(const std::string_view key) {
        if (auto* p = find_ptr(key); p != nullptr) {
            return *p;
        }
        throw missing_key_error("Object::require: missing key '" + std::string(key) + "'");
    }
    /// Return the value for `key`; throws `missing_key_error` if absent.
    const Value& require(const std::string_view key) const {
        if (const auto* p = find_ptr(key); p != nullptr) {
            return *p;
        }
        throw missing_key_error("Object::require: missing key '" + std::string(key) + "'");
    }

    /// Return the string at `key`; throws on missing key or type mismatch.
    std::string& require_string(const std::string_view key) {
        Value& v = require(key);
        if (auto* p = v.as_string_if(); p != nullptr) {
            return *p;
        }
        throw type_error("Object::require_string: '" + std::string(key) + "' is not a string");
    }
    /// Return the string at `key`; throws on missing key or type mismatch.
    const std::string& require_string(const std::string_view key) const {
        const Value& v = require(key);
        if (const auto* p = v.as_string_if(); p != nullptr) {
            return *p;
        }
        throw type_error("Object::require_string: '" + std::string(key) + "' is not a string");
    }

    /// Return the array at `key`; throws on missing key or type mismatch.
    Array& require_array(const std::string_view key) {
        Value& v = require(key);
        if (auto* p = v.as_array_if(); p != nullptr) {
            return *p;
        }
        throw type_error("Object::require_array: '" + std::string(key) + "' is not an array");
    }
    /// Return the array at `key`; throws on missing key or type mismatch.
    const Array& require_array(const std::string_view key) const {
        const Value& v = require(key);
        if (const auto* p = v.as_array_if(); p != nullptr) {
            return *p;
        }
        throw type_error("Object::require_array: '" + std::string(key) + "' is not an array");
    }

    /// Return the nested object at `key`; throws on missing key or type mismatch.
    Object& require_object(const std::string_view key) {
        Value& v = require(key);
        if (auto* p = v.as_object_if(); p != nullptr) {
            return *p;
        }
        throw type_error("Object::require_object: '" + std::string(key) + "' is not an object");
    }
    /// Return the nested object at `key`; throws on missing key or type mismatch.
    const Object& require_object(const std::string_view key) const {
        const Value& v = require(key);
        if (const auto* p = v.as_object_if(); p != nullptr) {
            return *p;
        }
        throw type_error("Object::require_object: '" + std::string(key) + "' is not an object");
    }

    /// Pointer to the string at `key`, or `nullptr` if absent or wrong type.
    [[nodiscard]] const std::string* get_string_if(const std::string_view key) const {
        if (const auto* v = find_ptr(key); v != nullptr) {
            return v->as_string_if();
        }
        return nullptr;
    }
    /// Pointer to the array at `key`, or `nullptr` if absent or wrong type.
    [[nodiscard]] const Array* get_array_if(const std::string_view key) const {
        if (const auto* v = find_ptr(key); v != nullptr) {
            return v->as_array_if();
        }
        return nullptr;
    }
    /// Pointer to the nested object at `key`, or `nullptr` if absent or wrong type.
    [[nodiscard]] const Object* get_object_if(const std::string_view key) const {
        if (const auto* v = find_ptr(key); v != nullptr) {
            return v->as_object_if();
        }
        return nullptr;
    }

    // --- path helpers (definitions live in path.hpp) ------------------

    /// Find a value by dotted path (e.g. `"a.b[0].c"`), or `nullptr` on miss.
    [[nodiscard]] Value* find_path(std::string_view path);
    /// Find a value by dotted path (e.g. `"a.b[0].c"`), or `nullptr` on miss.
    [[nodiscard]] const Value* find_path(std::string_view path) const;
    /// Return the value at `path`; throws on miss or malformed path.
    [[nodiscard]] const Value& require_path(std::string_view path) const;
    /// Return the value at `path`; throws on miss or malformed path.
    [[nodiscard]] Value& require_path(std::string_view path);
    /// True if `path` resolves to a value in the object.
    [[nodiscard]] bool contains_path(std::string_view path) const;

    // --- deep merge: source wins on non-object conflicts; arrays replaced ---

    /// Deep-merge `source` into this object; nested objects recurse, other
    /// alternatives are overwritten, and arrays are replaced wholesale.
    void merge(const Object& source);

    /// Access the underlying `std::unordered_map`.
    [[nodiscard]] map_type& raw() noexcept {
        return map_;
    }
    /// Access the underlying `std::unordered_map`.
    [[nodiscard]] const map_type& raw() const noexcept {
        return map_;
    }

    /// Deep value-equality compare two `Object`s.
    friend bool operator==(const Object& a, const Object& b) noexcept {
        return a.map_ == b.map_;
    }
    /// Negation of `operator==`.
    friend bool operator!=(const Object& a, const Object& b) noexcept {
        return !(a == b);
    }

private:
    map_type map_;
};

/// Convenience alias — `Metadata` is the canonical name for a top-level `Object`.
using Metadata = Object;

// =====================================================================
//   Value out-of-line definitions — defined here so that Object is
//   complete by the time we touch ~unique_ptr<Object>, *Object, etc.
// =====================================================================

inline Value::Value() noexcept : v_(nullptr) {}
inline Value::Value(std::nullptr_t) noexcept : v_(nullptr) {}

template <class B>
    requires std::same_as<B, bool>
inline Value::Value(B b) noexcept : v_(b) {}

template <detail::SignedIntLike T>
inline Value::Value(T x) noexcept : v_(static_cast<std::int64_t>(x)) {}

template <detail::UnsignedIntLike T>
inline Value::Value(T x) noexcept : v_(static_cast<std::uint64_t>(x)) {}

template <detail::FloatLike T>
inline Value::Value(T x) noexcept {
    if constexpr (std::is_same_v<std::remove_cv_t<T>, float>) {
        v_ = x;
    } else {
        v_ = static_cast<double>(x);
    }
}

inline Value::Value(std::string s) : v_(std::move(s)) {}
inline Value::Value(const std::string_view s) : v_(std::string(s)) {}
inline Value::Value(const char* s) : v_(std::string(s)) {}

inline Value::Value(Array a) : v_(std::move(a)) {}

inline Value::Value(Object o) : v_(std::make_unique<Object>(std::move(o))) {}

inline Value::Value(const std::initializer_list<std::pair<const std::string, Value>> il)
    : v_(std::make_unique<Object>(Object(il))) {}

inline Value::Value(Value&& other) noexcept : v_(std::move(other.v_)) {}

inline Value& Value::operator=(Value&& other) noexcept {
    v_ = std::move(other.v_);
    return *this;
}

inline Value::Value(const Value& other) {
    std::visit(
        [&](const auto& x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Object>>) {
                v_ = std::make_unique<Object>(*x);
            } else {
                v_ = x;
            }
        },
        other.v_);
}

inline Value& Value::operator=(const Value& other) {
    if (this == &other) {
        return *this;
    }
    Value tmp(other);
    v_ = std::move(tmp.v_);
    return *this;
}

inline Value& Value::operator=(const std::initializer_list<Value> il) {
    v_ = Array(il);
    return *this;
}

inline Value&
Value::operator=(const std::initializer_list<std::pair<const std::string, Value>> il) {
    v_ = std::make_unique<Object>(Object(il));
    return *this;
}

// NOLINTNEXTLINE(readability-redundant-inline-specifier)
inline Value::~Value() = default;

inline bool Value::is_null() const noexcept {
    return std::holds_alternative<std::nullptr_t>(v_);
}
inline bool Value::is_bool() const noexcept {
    return std::holds_alternative<bool>(v_);
}
inline bool Value::is_int() const noexcept {
    return std::holds_alternative<std::int64_t>(v_);
}
inline bool Value::is_uint() const noexcept {
    return std::holds_alternative<std::uint64_t>(v_);
}
inline bool Value::is_float() const noexcept {
    return std::holds_alternative<float>(v_);
}
inline bool Value::is_double() const noexcept {
    return std::holds_alternative<double>(v_);
}
inline bool Value::is_number() const noexcept {
    return is_int() || is_uint() || is_float() || is_double();
}
inline bool Value::is_string() const noexcept {
    return std::holds_alternative<std::string>(v_);
}
inline bool Value::is_array() const noexcept {
    return std::holds_alternative<Array>(v_);
}
inline bool Value::is_object() const noexcept {
    return std::holds_alternative<std::unique_ptr<Object>>(v_);
}

inline bool Value::as_bool() const {
    return std::get<bool>(v_);
}
inline std::int64_t Value::as_int() const {
    return std::get<std::int64_t>(v_);
}
inline std::uint64_t Value::as_uint() const {
    return std::get<std::uint64_t>(v_);
}

inline float Value::as_float() const {
    return std::get<float>(v_);
}

inline double Value::as_double() const {
    if (const auto* p = std::get_if<double>(&v_); p != nullptr) {
        return *p;
    }
    if (const auto* p = std::get_if<float>(&v_); p != nullptr) {
        return static_cast<double>(*p);
    }
    if (const auto* p = std::get_if<std::int64_t>(&v_); p != nullptr) {
        return static_cast<double>(*p);
    }
    if (const auto* p = std::get_if<std::uint64_t>(&v_); p != nullptr) {
        return static_cast<double>(*p);
    }
    throw type_error("Value::as_double: value is not a number");
}

inline std::string& Value::as_string() {
    return std::get<std::string>(v_);
}
inline const std::string& Value::as_string() const {
    return std::get<std::string>(v_);
}
inline Array& Value::as_array() {
    return std::get<Array>(v_);
}
inline const Array& Value::as_array() const {
    return std::get<Array>(v_);
}

inline Object& Value::as_object() {
    return *std::get<std::unique_ptr<Object>>(v_);
}
inline const Object& Value::as_object() const {
    return *std::get<std::unique_ptr<Object>>(v_);
}

inline bool* Value::as_bool_if() noexcept {
    return std::get_if<bool>(&v_);
}
inline const bool* Value::as_bool_if() const noexcept {
    return std::get_if<bool>(&v_);
}
inline std::int64_t* Value::as_int_if() noexcept {
    return std::get_if<std::int64_t>(&v_);
}
inline const std::int64_t* Value::as_int_if() const noexcept {
    return std::get_if<std::int64_t>(&v_);
}
inline std::uint64_t* Value::as_uint_if() noexcept {
    return std::get_if<std::uint64_t>(&v_);
}
inline const std::uint64_t* Value::as_uint_if() const noexcept {
    return std::get_if<std::uint64_t>(&v_);
}
inline float* Value::as_float_if() noexcept {
    return std::get_if<float>(&v_);
}
inline const float* Value::as_float_if() const noexcept {
    return std::get_if<float>(&v_);
}
inline double* Value::as_double_if() noexcept {
    return std::get_if<double>(&v_);
}
inline const double* Value::as_double_if() const noexcept {
    return std::get_if<double>(&v_);
}

inline std::string* Value::as_string_if() noexcept {
    return std::get_if<std::string>(&v_);
}
inline const std::string* Value::as_string_if() const noexcept {
    return std::get_if<std::string>(&v_);
}
inline Array* Value::as_array_if() noexcept {
    return std::get_if<Array>(&v_);
}
inline const Array* Value::as_array_if() const noexcept {
    return std::get_if<Array>(&v_);
}

inline Object* Value::as_object_if() noexcept {
    if (const auto* p = std::get_if<std::unique_ptr<Object>>(&v_); p != nullptr) {
        return p->get();
    }
    return nullptr;
}
inline const Object* Value::as_object_if() const noexcept {
    if (const auto* p = std::get_if<std::unique_ptr<Object>>(&v_); p != nullptr) {
        return p->get();
    }
    return nullptr;
}

inline bool operator==(const Value& a, const Value& b) noexcept {
    if (a.v_.index() != b.v_.index()) {
        return false;
    }
    return std::visit(
        [&](const auto& x) -> bool {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Object>>) {
                const auto& y = std::get<std::unique_ptr<Object>>(b.v_);
                return *x == *y;
            } else {
                const auto& y = std::get<T>(b.v_);
                return x == y;
            }
        },
        a.v_);
}

inline bool operator!=(const Value& a, const Value& b) noexcept {
    return !(a == b);
}

template <class T>
inline T* Value::get_if() noexcept {
    return std::get_if<T>(&v_);
}
template <class T>
inline const T* Value::get_if() const noexcept {
    return std::get_if<T>(&v_);
}

template <class T>
inline T Value::value_or(T fallback) const {
    if (auto* p = std::get_if<T>(&v_)) {
        return *p;
    }
    return fallback;
}

inline Value::variant_type& Value::raw() noexcept {
    return v_;
}
inline const Value::variant_type& Value::raw() const noexcept {
    return v_;
}

inline std::size_t Value::index() const noexcept {
    return v_.index();
}

// --- Value factory helpers (out-of-line because string ones touch std::string) ---

inline Value null() noexcept {
    return Value{};
}
inline Value boolean(const bool b) noexcept {
    return Value{b};  // routes through the constrained <same_as<bool>> template
}
inline Value string(std::string s) {
    return Value{std::move(s)};
}
inline Value string(const std::string_view s) {
    return Value{s};
}
inline Value string(const char* s) {
    return Value{s};
}

// --- object() factory helpers ---

/// Construct an empty `Object`.
[[nodiscard]] inline Object object() {
    return Object{};
}
/// Construct an `Object` from a braced list of key/value pairs.
[[nodiscard]] inline Object object(const std::initializer_list<Object::value_type> il) {
    return Object(il);
}

// --- Object::merge (defined after Value is complete) ---

inline void Object::merge(const Object& source) {
    for (const auto& [k, v] : source.map_) {
        auto it = map_.find(k);
        if (it == map_.end()) {
            map_.emplace(k, v);
            continue;
        }
        Object* dst_obj = it->second.as_object_if();
        if (const Object* src_obj = v.as_object_if(); dst_obj != nullptr && src_obj != nullptr) {
            dst_obj->merge(*src_obj);
        } else {
            it->second = v;
        }
    }
}

}  // namespace md
