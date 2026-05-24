#pragma once

#include <md/error.hpp>

#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace md {

class Value;
class Object;

/// Sequence container alias used for the array alternative of `Value`.
using Array = std::vector<Value>;

namespace detail {

// Transparent hash + equal-to lets std::unordered_map<std::string, ...> accept
// std::string_view / const char* in find/contains/erase without allocating.
struct TransparentStringHash {
    using is_transparent = void;

    [[nodiscard]] std::size_t operator()(const std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
    [[nodiscard]] std::size_t operator()(const std::string& s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
    [[nodiscard]] std::size_t operator()(const char* s) const noexcept {
        return std::hash<std::string_view>{}(std::string_view{s});
    }
};

// Concepts that exclude bool and character types so that Value{true} stays
// a bool and Value{'a'} doesn't sneak in as an integer.
template <class T>
concept SignedIntLike =
    std::is_integral_v<T> && std::is_signed_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool> &&
    !std::is_same_v<std::remove_cv_t<T>, char> && !std::is_same_v<std::remove_cv_t<T>, wchar_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char8_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char16_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char32_t>;

template <class T>
concept UnsignedIntLike =
    std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool> &&
    !std::is_same_v<std::remove_cv_t<T>, char> && !std::is_same_v<std::remove_cv_t<T>, wchar_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char8_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char16_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char32_t>;

template <class T>
concept FloatLike = std::is_floating_point_v<T>;

}  // namespace detail

/// Discriminated union holding one of the JSON-like alternatives
/// (null, bool, signed/unsigned integer, float/double, string, array, or
/// nested object).
class Value {
public:
    // Object is indirected through unique_ptr so Value can recursively hold an
    // Object that itself stores Values. Without indirection, std::variant
    // probes traits like is_*_constructible<Object> at instantiation time,
    // which would require Object complete here.
    /// Underlying `std::variant` type that stores the active alternative.
    using variant_type = std::variant<std::nullptr_t,
                                      bool,
                                      std::int64_t,
                                      std::uint64_t,
                                      float,
                                      double,
                                      std::string,
                                      Array,
                                      std::unique_ptr<Object>>;

    // Every member that touches v_ is DECLARED here and DEFINED out-of-line
    // in object.hpp. Reason: instantiating variant<..., unique_ptr<Object>>
    // operations (including the constructor's exception-handling rollback)
    // requires unique_ptr<Object>::~unique_ptr to be instantiable, which
    // requires Object complete — which isn't the case until object.hpp.

    /// Construct a null-valued `Value`.
    Value() noexcept;
    /// Move-construct from another `Value`, leaving the source in a valid but unspecified state.
    Value(Value&& other) noexcept;
    /// Move-assign from another `Value`.
    Value& operator=(Value&& other) noexcept;
    /// Deep-copy construct from another `Value`.
    Value(const Value& other);
    /// Deep-copy assign from another `Value`.
    Value& operator=(const Value& other);
    /// Destructor.
    ~Value();

    // Braced-list assignment for ergonomic in-place writes:
    //   m["tags"] = {"a", "b"};          // → Array
    //   m["sub"]  = {{"k", 1}, {"k2", 2}}; // → Object (pair-like elements)
    // The two overloads disambiguate by element type: bare values pick the
    // Array overload; brace-pair elements pick the Object overload (Value
    // has no 2-arg constructor, so they only match the pair form).
    // An empty list `= {}` is ambiguous — use `md::Array{}` or `md::Object{}`
    // explicitly when you mean empty.
    /// Assign an `Array` from a braced list of values.
    Value& operator=(std::initializer_list<Value> il);
    /// Assign an `Object` from a braced list of key/value pairs.
    Value& operator=(std::initializer_list<std::pair<const std::string, Value>> il);

    /// Construct a null-valued `Value`.
    Value(std::nullptr_t) noexcept;

    /// Construct a boolean-valued `Value`.
    template <class B>
        requires std::same_as<B, bool>
    Value(B b) noexcept;

    /// Construct a signed-integer `Value` (stored as `std::int64_t`).
    template <detail::SignedIntLike T>
    Value(T x) noexcept;

    /// Construct an unsigned-integer `Value` (stored as `std::uint64_t`).
    template <detail::UnsignedIntLike T>
    Value(T x) noexcept;

    // FloatLike covers float/double/long double; the constructor stores
    // single-precision input as `float` and everything else as `double`.
    /// Construct a floating-point `Value`; `float` stays `float`, others widen to `double`.
    template <detail::FloatLike T>
    Value(T x) noexcept;

    /// Construct a string-valued `Value` from an owning `std::string`.
    Value(std::string s);
    /// Construct a string-valued `Value` from a `std::string_view`.
    Value(std::string_view s);
    /// Construct a string-valued `Value` from a C string.
    Value(const char* s);

    /// Construct an array-valued `Value`.
    Value(Array a);
    /// Construct an object-valued `Value`.
    Value(Object o);

    // Build an Object directly from a pair-shaped braced list. Lets nested
    // objects parse naturally:
    //   m["sub"] = {{"a", {{"b", 1}}}};  // outer Object, inner Object
    // We deliberately do NOT add `Value(initializer_list<Value>)` for arrays
    // because that would change the meaning of `Value{42}` from scalar to
    // single-element Array.
    /// Construct an object-valued `Value` from a braced list of key/value pairs.
    Value(std::initializer_list<std::pair<const std::string, Value>> il);

    /// Deleted to prevent silent pointer-to-bool conversions.
    template <class T>
    Value(T*) = delete;

    // --- predicates --------------------------------------------------

    /// True if the value holds `nullptr`.
    [[nodiscard]] bool is_null() const noexcept;
    /// True if the value holds a `bool`.
    [[nodiscard]] bool is_bool() const noexcept;
    /// True if the value holds a signed integer (`std::int64_t`).
    [[nodiscard]] bool is_int() const noexcept;
    /// True if the value holds an unsigned integer (`std::uint64_t`).
    [[nodiscard]] bool is_uint() const noexcept;
    /// True if the value holds a `float`.
    [[nodiscard]] bool is_float() const noexcept;
    /// True if the value holds a `double`.
    [[nodiscard]] bool is_double() const noexcept;
    /// True if the value holds any numeric alternative.
    [[nodiscard]] bool is_number() const noexcept;
    /// True if the value holds a `std::string`.
    [[nodiscard]] bool is_string() const noexcept;
    /// True if the value holds an `Array`.
    [[nodiscard]] bool is_array() const noexcept;
    /// True if the value holds a nested `Object`.
    [[nodiscard]] bool is_object() const noexcept;

    // --- strict accessors -------------------------------------------

    /// Return the bool; throws `std::bad_variant_access` on type mismatch.
    [[nodiscard]] bool as_bool() const;
    /// Return the signed integer; throws on type mismatch.
    [[nodiscard]] std::int64_t as_int() const;
    /// Return the unsigned integer; throws on type mismatch.
    [[nodiscard]] std::uint64_t as_uint() const;
    /// Return the `float`; strict — throws if the value isn't a `float`.
    [[nodiscard]] float as_float() const;
    /// Return as `double`, widening from `int64`/`uint64`/`float` as needed.
    [[nodiscard]] double as_double() const;

    /// Access the string alternative; throws on type mismatch.
    [[nodiscard]] std::string& as_string();
    /// Access the string alternative; throws on type mismatch.
    [[nodiscard]] const std::string& as_string() const;
    /// Access the array alternative; throws on type mismatch.
    [[nodiscard]] Array& as_array();
    /// Access the array alternative; throws on type mismatch.
    [[nodiscard]] const Array& as_array() const;
    /// Access the nested object; throws on type mismatch.
    [[nodiscard]] Object& as_object();
    /// Access the nested object; throws on type mismatch.
    [[nodiscard]] const Object& as_object() const;

    // --- pointer accessors (noexcept) -------------------------------

    /// Return a pointer to the bool, or `nullptr` if the value isn't a bool.
    [[nodiscard]] bool* as_bool_if() noexcept;
    /// Return a pointer to the bool, or `nullptr` if the value isn't a bool.
    [[nodiscard]] const bool* as_bool_if() const noexcept;
    /// Return a pointer to the signed integer, or `nullptr` if not held.
    [[nodiscard]] std::int64_t* as_int_if() noexcept;
    /// Return a pointer to the signed integer, or `nullptr` if not held.
    [[nodiscard]] const std::int64_t* as_int_if() const noexcept;
    /// Return a pointer to the unsigned integer, or `nullptr` if not held.
    [[nodiscard]] std::uint64_t* as_uint_if() noexcept;
    /// Return a pointer to the unsigned integer, or `nullptr` if not held.
    [[nodiscard]] const std::uint64_t* as_uint_if() const noexcept;
    /// Return a pointer to the `float`, or `nullptr` if not held.
    [[nodiscard]] float* as_float_if() noexcept;
    /// Return a pointer to the `float`, or `nullptr` if not held.
    [[nodiscard]] const float* as_float_if() const noexcept;
    /// Return a pointer to the `double`, or `nullptr` if not held.
    [[nodiscard]] double* as_double_if() noexcept;
    /// Return a pointer to the `double`, or `nullptr` if not held.
    [[nodiscard]] const double* as_double_if() const noexcept;
    /// Return a pointer to the string, or `nullptr` if not held.
    [[nodiscard]] std::string* as_string_if() noexcept;
    /// Return a pointer to the string, or `nullptr` if not held.
    [[nodiscard]] const std::string* as_string_if() const noexcept;
    /// Return a pointer to the array, or `nullptr` if not held.
    [[nodiscard]] Array* as_array_if() noexcept;
    /// Return a pointer to the array, or `nullptr` if not held.
    [[nodiscard]] const Array* as_array_if() const noexcept;
    /// Return a pointer to the nested object, or `nullptr` if not held.
    [[nodiscard]] Object* as_object_if() noexcept;
    /// Return a pointer to the nested object, or `nullptr` if not held.
    [[nodiscard]] const Object* as_object_if() const noexcept;

    /// Return a pointer to the alternative of type `T`, or `nullptr` if not held.
    template <class T>
    [[nodiscard]] T* get_if() noexcept;
    /// Return a pointer to the alternative of type `T`, or `nullptr` if not held.
    template <class T>
    [[nodiscard]] const T* get_if() const noexcept;

    /// Return the held `T` by value, or `fallback` if a different alternative is active.
    template <class T>
    [[nodiscard]] T value_or(T fallback) const;

    /// Return the underlying `std::variant` for advanced access.
    [[nodiscard]] variant_type& raw() noexcept;
    /// Return the underlying `std::variant` for advanced access.
    [[nodiscard]] const variant_type& raw() const noexcept;

    /// Return the zero-based index of the active alternative.
    [[nodiscard]] std::size_t index() const noexcept;

    /// Deep value-equality compare two `Value`s.
    friend bool operator==(const Value& a, const Value& b) noexcept;
    /// Negation of `operator==`.
    friend bool operator!=(const Value& a, const Value& b) noexcept;

private:
    variant_type v_;
};

// --- factory helper declarations ----------------------------------------

/// Construct a null-valued `Value`.
[[nodiscard]] Value null() noexcept;
/// Construct a boolean-valued `Value`.
[[nodiscard]] Value boolean(bool b) noexcept;

/// Construct a numeric `Value` from any signed, unsigned, or floating type.
template <class T>
    requires(detail::SignedIntLike<T> || detail::UnsignedIntLike<T> || detail::FloatLike<T>)
[[nodiscard]] inline Value number(T v) noexcept {
    return Value{v};
}

/// Construct a string `Value` from an owning `std::string`.
[[nodiscard]] Value string(std::string s);
/// Construct a string `Value` from a `std::string_view`.
[[nodiscard]] Value string(std::string_view s);
/// Construct a string `Value` from a C string.
[[nodiscard]] Value string(const char* s);

/// Construct an empty `Array`.
[[nodiscard]] inline Array array() {
    return Array{};
}
/// Construct an `Array` from a braced list of values.
[[nodiscard]] inline Array array(const std::initializer_list<Value> il) {
    return Array(il);
}

// object() / object({...}) factories are defined in object.hpp.

}  // namespace md
