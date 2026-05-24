#pragma once

/// @file
/// @brief Optional cpp-parcel adapter. Wraps `md::Value`, `md::Object`, and
///        `md::Array` as primitive-style `parcel::BaseCell` types so they
///        round-trip through a `parcel::ParcelRegistry` like any other cell.
///
/// Wire kinds:
///   - `md:v` — `md::ValueCell`  (storage: `md::Value`)
///   - `md:o` — `md::ObjectCell` (storage: `md::Object`)
///   - `md:a` — `md::ArrayCell`  (storage: `md::Array`)
///
/// This header is auto-included from `<md/metadata.hpp>` when
/// `<parcel/parcel.h>` is on the include path. Including it directly when
/// cpp-parcel (or the nlohmann/json adapter it builds on) is unavailable
/// will fail at the `#include` line below — match the `<md/json.hpp>`
/// gating style.

#include <md/json.hpp>  // brings ADL to_json/from_json for md::Value/Object
#include <md/object.hpp>
#include <md/ostream.hpp>  // operator<< for to_string()
#include <md/value.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <parcel/parcel.h>

namespace md {

/// @brief Parcel cell wrapping an `md::Value`.
///
/// Wire kind: `md:v`. The `"v"` slot carries the value in its natural JSON
/// shape via the `<md/json.hpp>` ADL hooks.
class ValueCell : public parcel::BaseCell<ValueCell, Value> {
    using base_t = parcel::BaseCell<ValueCell, Value>;

public:
    /// @brief Inherit `BaseCell`'s default and perfect-forwarding constructors,
    ///        so `ValueCell{}`, `ValueCell{42}`, `ValueCell{"hi"}`, and
    ///        `ValueCell{md::Value{...}}` all work directly.
    using base_t::base_t;
    /// @brief Inherit `BaseCell::operator=` so a `ValueCell` can be reassigned
    ///        from anything `md::Value` accepts.
    using base_t::operator=;

    /// @brief Wire-stable kind id reported under the `"k"` slot.
    static constexpr std::string_view kind_id = "md:v";

    /// @brief Compact textual rendering of the wrapped `md::Value`, identical
    ///        to streaming via `operator<<` (escaped JSON-shaped output).
    [[nodiscard]] std::string to_string() const override {
        std::ostringstream os;
        os << this->value;
        return os.str();
    }

    /// @brief Parcel deserialization entry point. Validates `"k" == "md:v"`
    ///        and decodes `"v"` as an `md::Value` via the `<md/json.hpp>`
    ///        ADL hooks.
    /// @param j Incoming `{"k","v"}` envelope.
    /// @throws parcel::ParcelException on shape or kind mismatch.
    [[maybe_unused]] static parcel::cell_t from_json(parcel::json_t const& j,
                                                     parcel::ParcelRegistry const&) {
        return base_t::from_json_strict(j);
    }

    /// @brief Registry descriptor for `ValueCell`. Carries display metadata
    ///        (name, icon, color) used by introspection tooling.
    static parcel::cell_type_descriptor_t descriptor() {
        static const auto d = std::make_shared<parcel::SimpleCellTypeDescriptor<ValueCell>>(
            parcel::descriptor::MetaInfo{
                .name = "md::Value",
                .description = "Dynamic JSON-shaped value from the metadata library.",
                .icon = "mdi:variable",
                .color = "#673AB7",  // Material Deep Purple 500
            });
        return d;
    }
};

/// @brief Parcel cell wrapping an `md::Object` (a.k.a. `md::Metadata`).
///
/// Wire kind: `md:o`. The `"v"` slot is a JSON object whose values are
/// themselves serialized through the `md::Value` JSON adapter.
class ObjectCell : public parcel::BaseCell<ObjectCell, Object> {
    using base_t = parcel::BaseCell<ObjectCell, Object>;

public:
    /// @brief Inherit `BaseCell`'s default and forwarding constructors.
    using base_t::base_t;
    /// @brief Inherit `BaseCell::operator=`.
    using base_t::operator=;

    /// @brief Brace-init passthrough so `md::ObjectCell{{"k", 1}, {"k2", "v"}}`
    ///        mirrors `md::Object{...}` exactly; no wrapper type at the call
    ///        site.
    /// @param il Initializer list of `{key, value}` pairs.
    ObjectCell(std::initializer_list<Object::value_type> il) : base_t(Object(il)) {}

    /// @brief Wire-stable kind id reported under the `"k"` slot.
    static constexpr std::string_view kind_id = "md:o";

    /// @brief Compact textual rendering of the wrapped `md::Object`,
    ///        identical to streaming via `operator<<`.
    [[nodiscard]] std::string to_string() const override {
        std::ostringstream os;
        os << this->value;
        return os.str();
    }

    /// @brief Parcel deserialization entry point. Validates `"k" == "md:o"`
    ///        and decodes `"v"` as an `md::Object` via the `<md/json.hpp>`
    ///        ADL hooks.
    /// @throws parcel::ParcelException on shape or kind mismatch.
    [[maybe_unused]] static parcel::cell_t from_json(parcel::json_t const& j,
                                                     parcel::ParcelRegistry const&) {
        return base_t::from_json_strict(j);
    }

    /// @brief Registry descriptor for `ObjectCell`.
    static parcel::cell_type_descriptor_t descriptor() {
        static const auto d = std::make_shared<parcel::SimpleCellTypeDescriptor<ObjectCell>>(
            parcel::descriptor::MetaInfo{
                .name = "md::Object",
                .description = "Unordered string-keyed map of md::Value (md::Object/Metadata).",
                .icon = "mdi:code-braces",
                .color = "#FFA000",  // Material Amber 700
            });
        return d;
    }
};

/// @brief Parcel cell wrapping an `md::Array` (`std::vector<md::Value>`).
///
/// Wire kind: `md:a`. The `"v"` slot is a JSON array whose elements are
/// serialized through the `md::Value` JSON adapter.
class ArrayCell : public parcel::BaseCell<ArrayCell, Array> {
    using base_t = parcel::BaseCell<ArrayCell, Array>;

public:
    /// @brief Inherit `BaseCell`'s default and forwarding constructors.
    using base_t::base_t;
    /// @brief Inherit `BaseCell::operator=`.
    using base_t::operator=;

    /// @brief Brace-init passthrough so `md::ArrayCell{"a", "b", "c"}` mirrors
    ///        `md::Array{...}`. Constrained to ≥2 elements so that
    ///        `ArrayCell{some_array}` continues to forward to `BaseCell` —
    ///        `Array → Value` is implicit, so a 1-element overload would
    ///        silently wrap the array as a single-element array.
    template <class T0, class T1, class... Rest>
        requires(std::constructible_from<Value, T0 &&> && std::constructible_from<Value, T1 &&> &&
                 (std::constructible_from<Value, Rest &&> && ...))
    ArrayCell(T0&& a, T1&& b, Rest&&... rest)
        : base_t(Array{Value(std::forward<T0>(a)),
                       Value(std::forward<T1>(b)),
                       Value(std::forward<Rest>(rest))...}) {}

    /// @brief Wire-stable kind id reported under the `"k"` slot.
    static constexpr std::string_view kind_id = "md:a";

    /// @brief Compact textual rendering of the wrapped `md::Array`, identical
    ///        to streaming via `operator<<`.
    [[nodiscard]] std::string to_string() const override {
        std::ostringstream os;
        os << this->value;
        return os.str();
    }

    /// @brief Parcel deserialization entry point. Validates `"k" == "md:a"`
    ///        and decodes `"v"` as an `md::Array` via the `<md/json.hpp>`
    ///        ADL hooks.
    /// @throws parcel::ParcelException on shape or kind mismatch.
    [[maybe_unused]] static parcel::cell_t from_json(parcel::json_t const& j,
                                                     parcel::ParcelRegistry const&) {
        return base_t::from_json_strict(j);
    }

    /// @brief Registry descriptor for `ArrayCell`.
    static parcel::cell_type_descriptor_t descriptor() {
        static const auto d = std::make_shared<parcel::SimpleCellTypeDescriptor<ArrayCell>>(
            parcel::descriptor::MetaInfo{
                .name = "md::Array",
                .description = "Heterogeneous vector of md::Value (md::Array).",
                .icon = "mdi:code-brackets",
                .color = "#43A047",  // Material Green 600
            });
        return d;
    }
};

/// @brief Register all three md cells (`md:v`, `md:o`, `md:a`) on @p registry.
///
/// Equivalent to three separate `register_kind(...)` calls — the one-liner
/// every consumer otherwise has to repeat. Re-registering a kind on an
/// existing registry is a parcel-level error, so call this exactly once
/// per registry.
inline void register_cells(parcel::ParcelRegistry& registry) {
    registry.register_kind(ValueCell::descriptor());
    registry.register_kind(ObjectCell::descriptor());
    registry.register_kind(ArrayCell::descriptor());
}

}  // namespace md

// FieldsBuilder/parcel::cell(...) inference hooks. Each macro expands to a
// `parcel::default_cell_for<Storage>` specialization at namespace scope.
PARCEL_DEFAULT_CELL(md::ValueCell);
PARCEL_DEFAULT_CELL(md::ObjectCell);
PARCEL_DEFAULT_CELL(md::ArrayCell);

#define METADATA_HAS_PARCEL 1
