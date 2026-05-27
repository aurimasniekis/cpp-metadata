# Metadata

[![CI](https://github.com/aurimasniekis/cpp-metadata/actions/workflows/ci.yml/badge.svg)](https://github.com/aurimasniekis/cpp-metadata/actions/workflows/ci.yml)
[![Docs](https://github.com/aurimasniekis/cpp-metadata/actions/workflows/docs.yml/badge.svg)](https://aurimasniekis.github.io/cpp-metadata/)

A small, header-only C++23 library for building **typed dynamic key/value
trees** that look and behave like a JSON document in memory. Use it whenever
you would otherwise reach for `std::unordered_map<std::string, std::any>` to
hold a bag of mixed-type properties: device/sensor metadata, configuration
trees, plugin parameters, scratch attributes on records, etc.

The CMake project and target are named `metadata`, but the C++ namespace is
`md`. There is no parser, no schema, and no networking layer — just the
in-memory shape, with an optional nlohmann/json bridge for the rare moment
you actually want to round-trip to text.

---

## Why use this library?

- **Good for** small-to-mid in-memory property bags that mix booleans,
  integers, floats, strings, arrays, and nested objects.
- **Good for** "JSON-shaped data without a JSON dependency": the optional
  `<md/json.hpp>` adapter is opt-in, and the core compiles with only the
  C++23 standard library.
- **Good for** path-style access: `m.require_path("device.channels[0].freq_hz")`.
- **Avoids** the classic `const char*` → `bool` trap that variant-based
  JSON value types suffer from. `Value{"x"}` is always a string;
  arbitrary pointer types are explicitly `= delete`'d.
- **Avoids** silent conflation of integer kinds. `int`, `unsigned`, `float`,
  and `double` are distinct alternatives in `Value` and stay that way (with
  one documented exception around the JSON adapter — see below).
- **Not ideal for** workloads that need insertion-order preservation;
  `Object` is backed by `std::unordered_map`.
- **Not ideal for** full JSONPath / RFC 6901 / JSON Pointer; the path
  syntax is dot + bracket only.
- **Not ideal for** parsing JSON text. The library never parses strings;
  if you need parsing, use nlohmann/json (or any other parser) and convert
  via the bundled adapter.

---

## Quick example

```cpp
#include <md/metadata.hpp>

#include <iostream>

int main() {
    md::Metadata m;                    // alias for md::Object
    m["name"]    = "sensor-7";         // string
    m["enabled"] = true;               // bool
    m["count"]   = 42;                 // signed int -> int64
    m["weight"]  = 3.14;               // double
    m["tags"]    = {"alpha", "beta"};  // Array (bare braced elements)

    // Nested object: a braced list of {key, value} pairs.
    m["device"] = {
        {"id",       "abc-123"},
        {"firmware", {{"major", 1}, {"minor", 4}}},
    };

    // Compact JSON-like output via operator<<.
    std::cout << m << '\n';

    // Typed retrieval — throws md::missing_key_error / md::type_error on misuse.
    std::cout << m.require_string("name") << '\n';
    std::cout << m.require_path("device.firmware.major").as_int() << '\n';
    std::cout << m.require_path("tags[0]").as_string() << '\n';
}
```

What is going on:

- `md::Metadata` is a type alias for `md::Object`. It has the surface of
  `std::unordered_map<std::string, md::Value>` plus the metadata helpers
  (`require_*`, `find_path`, `merge`, …).
- The braced list `{"alpha", "beta"}` resolves to an `Array` because every
  element is a single `Value`. The list of `{key, value}` pairs resolves to
  an `Object` because no `Value` constructor takes two arguments — only
  the pair-shaped `operator=` matches. That disambiguation is deliberate.
- `operator<<` produces a compact JSON-shaped string. Floating-point values
  use `std::to_chars` shortest round-trip, so `3.14` prints as `3.14`, not
  `3.1400000000000001`.

---

## Installation

The library is a header-only CMake `INTERFACE` target. The minimum CMake
version is 3.25.

### CMake — FetchContent

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    metadata
    URL      https://github.com/aurimasniekis/cpp-metadata/archive/refs/tags/v0.2.0.tar.gz
    URL_HASH SHA256=PLACEHOLDER
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(metadata)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE metadata::metadata)
```

`nlohmann/json` (3.12.0) is fetched the same way when JSON support is
enabled, but a `find_package`-installed copy is preferred (see
`cmake/Dependencies.cmake`).

### CMake — add_subdirectory

If you vendor the source into a subdirectory:

```cmake
add_subdirectory(third_party/metadata)
target_link_libraries(my_app PRIVATE metadata::metadata)
```

### CMake — `find_package` after install

If the library was installed with `cmake --install <build>`:

```cmake
find_package(metadata 0.2 REQUIRED)
target_link_libraries(my_app PRIVATE metadata::metadata)
```

Install rules are auto-disabled when `nlohmann_json` was fetched (a fetched
dependency cannot be re-exported by an installed package). To install,
either provide `nlohmann_json` via the system package manager / a prior
`find_package` install, or disable JSON support with
`-DMETADATA_WITH_NLOHMANN_JSON=OFF`.

### Manual drop-in

Copy the `include/md/` directory into your include path. The core requires
only the C++23 standard library. `<md/json.hpp>` `#error`s out at the top
of the file if `<nlohmann/json.hpp>` is missing — the failure is loud and
immediate, not a silent miss.

> The `<md/version.hpp>` header is generated by CMake from
> `version.hpp.in`. If you copy headers manually, you have to provide your
> own `version.hpp` or skip including it.

---

## Requirements

- **C++ standard**: C++23 — the implementation uses `<format>`, concepts,
  `std::variant`, `std::to_chars` for `double`, and heterogeneous lookup
  with a transparent hash.
- **CMake**: ≥ 3.25 when building via CMake.
- **Optional dependency**: [`nlohmann/json`](https://github.com/nlohmann/json)
  ≥ 3.12, only when `METADATA_WITH_NLOHMANN_JSON=ON` (the default).

---

## Core concepts

The public surface is small. The headers that matter day-to-day are:

| Header              | Provides                                        |
|---------------------|-------------------------------------------------|
| `<md/metadata.hpp>` | Umbrella header. Pulls in everything below.     |
| `<md/value.hpp>`    | `Value`, `Array`, the constrained constructors. |
| `<md/object.hpp>`   | `Object` (aliased as `Metadata`).               |
| `<md/path.hpp>`     | `find_path`, `require_path`, `contains_path`.   |
| `<md/helpers.hpp>`  | Free-function forwarders (`md::contains`, …).   |
| `<md/ostream.hpp>`  | `operator<<` for `Value`, `Object`, `Array`.    |
| `<md/format.hpp>`   | `std::formatter` specializations.               |
| `<md/hash.hpp>`     | `std::hash<Value>` / `<Object>` / `<Array>`.    |
| `<md/json.hpp>`     | Optional nlohmann/json adapter.                 |
| `<md/parcel.hpp>`   | Optional cpp-parcel adapter (cell wrappers).    |
| `<md/error.hpp>`    | `md::error`, `missing_key_error`, `type_error`. |

Just `#include <md/metadata.hpp>` and you get the full API. The JSON
adapter is auto-included via `__has_include(<nlohmann/json.hpp>)`, and
the parcel adapter via `__has_include(<parcel/parcel.h>)`.

### `md::Value`

`Value` is a discriminated union with these alternatives:

| Alternative      | Predicate     | Strict accessor  | Pointer accessor (noexcept) |
|------------------|---------------|------------------|-----------------------------|
| `std::nullptr_t` | `is_null()`   | —                | —                           |
| `bool`           | `is_bool()`   | `as_bool()`      | `as_bool_if()`              |
| `std::int64_t`   | `is_int()`    | `as_int()`       | `as_int_if()`               |
| `std::uint64_t`  | `is_uint()`   | `as_uint()`      | `as_uint_if()`              |
| `float`          | `is_float()`  | `as_float()`     | `as_float_if()`             |
| `double`         | `is_double()` | `as_double()` \* | `as_double_if()`            |
| `std::string`    | `is_string()` | `as_string()`    | `as_string_if()`            |
| `Array`          | `is_array()`  | `as_array()`     | `as_array_if()`             |
| `Object`         | `is_object()` | `as_object()`    | `as_object_if()`            |

`is_number()` is true if any of `int`, `uint`, `float`, or `double` holds.

\* **`as_double()` is the only widening accessor.** It accepts `int64`,
`uint64`, `float`, or `double` and returns a `double`. If the value isn't
a number at all it throws `md::type_error`. Every other strict `as_*`
accessor throws `std::bad_variant_access` on a type mismatch.

The pointer-returning `as_*_if()` family is `noexcept` and returns
`nullptr` on a mismatch — use it when exceptions are not what you want:

```cpp
md::Value v{42};
if (auto* p = v.as_int_if()) {
    // *p is a std::int64_t
}

// value_or<T>: returns the stored T if the alternative matches, otherwise
// returns the fallback.
std::int64_t n = v.value_or<std::int64_t>(0);
```

Construction is engineered against the classic JSON-value pitfalls:

- `Value{true}` stays a `bool`. The bool constructor is constrained with
  `requires std::same_as<B, bool>`, so an `int` does not slip in as `bool`.
- Signed integer types (`int`, `short`, `long`, …) route to `int64_t`.
  Unsigned types (`unsigned`, `size_t`, …) route to `uint64_t`. Character
  types (`char`, `wchar_t`, `char8_t`, `char16_t`, `char32_t`) are
  excluded from both — they are not treated as integers.
- `float` literals stay 32-bit; `double` and `long double` route to
  `double` (64-bit).
- `Value{"abc"}` is a string. The constructor template
  `template <class T> Value(T*) = delete;` shoots down arbitrary pointer
  decay into `bool`.

### `md::Object` (a.k.a. `md::Metadata`)

`Object` wraps
`std::unordered_map<std::string, Value, TransparentStringHash, std::equal_to<>>`
and exposes both the familiar map surface and a small set of metadata
helpers.

```cpp
md::Object o{{"name", "x"}};

o["count"] = 1;                            // implicit Value(int)
o.at("name");                              // throws std::out_of_range on miss
auto it = o.find("name");                  // STL-style iterator return
o.insert_or_assign("count", md::Value{2});
o.erase("count");
for (const auto& [k, v] : o) { /* ... */ }
```

The transparent hash and `equal_to<>` mean `find`, `contains`, `count`,
and `erase` accept `std::string_view` and `const char*` directly without
allocating a temporary `std::string`.

Metadata helpers on `Object`:

| Method                                          | Returns / throws                                       |
|-------------------------------------------------|--------------------------------------------------------|
| `contains(k)`                                   | `bool`                                                 |
| `find_ptr(k)`                                   | `Value*` or `nullptr`                                  |
| `require(k)`                                    | `Value&`, or throws `missing_key_error`                |
| `require_string(k)` / `_array` / `_object`      | typed reference, or `missing_key_error` / `type_error` |
| `get_string_if(k)` / `_array_if` / `_object_if` | `const T*` or `nullptr` (never throws)                 |
| `merge(src)`                                    | deep merge — source wins on conflict                   |
| `find_path("a.b[0].c")`                         | `Value*` or `nullptr`                                  |
| `require_path(...)`                             | reference, or throws                                   |
| `contains_path(...)`                            | `bool`                                                 |

Free-function forwarders in `<md/helpers.hpp>` (`md::contains(o, k)`,
`md::require_string(o, k)`, etc.) exist for code that prefers a
non-method style.

`Metadata` is a type alias for `Object` — the only reason to use the
alias is for self-documenting names in user code.

### `md::Array`

`Array` is `std::vector<Value>`. There is no wrapper class around it, so
all of the standard vector operations apply (push, emplace, iterators,
range-for, etc.). Two factory helpers exist for symmetry:

```cpp
md::Array empty = md::array();
md::Array three = md::array({md::Value{1}, md::Value{2}, md::Value{3}});
```

### Path syntax

Paths use `.` to descend into objects and `[N]` to index into arrays.
There is no escaping for `.` or `[` inside keys — keys with those
characters cannot be addressed by path; use direct `o["..."]` access
instead.

```text
a.b.c        — walk three nested objects
items[0]     — index into the array under "items"
a[1].b       — array, then object
a[1][0]      — nested arrays
""           — empty path: see below
```

Behavior:

- `find_path` returns `nullptr` for any of: missing key, out-of-range
  index, malformed syntax (`items[`, `items[abc]`, leading `..`), and
  type mismatches (descending into a non-object, indexing a non-array).
- `require_path` throws `missing_key_error` for plain misses and
  `type_error` for malformed syntax or type mismatches.
- `contains_path` is the boolean equivalent of `find_path`.
- An empty path returns `nullptr` / `false` and `require_path("")` throws
  `type_error`, because the API cannot return a `Value&` to the root
  object itself (the root is an `Object`, not a `Value`).

### `merge`

Deep merge with **source wins**:

- If both sides are `Object`, recurse.
- Otherwise, the source value overwrites the destination value. Arrays
  are **replaced**, not concatenated.

This is fine for layered configuration and overlays. If you need
array-append or any other strategy, do it explicitly with `as_array()`.

### Streaming and `std::format`

`<md/ostream.hpp>` defines `operator<<` for all three types, and
`<md/format.hpp>` defines `std::formatter` specializations for the same
three. Both go through the same compact JSON writer:

```cpp
std::cout  << v << '\n';
std::string s = std::format("{}", v);
```

- The format spec is **empty-only**. `std::format("{:p}", v)` throws
  `std::format_error`. Pretty-printing is out of scope for v1.
- Strings are escaped to JSON: `"`, `\`, `\b`, `\f`, `\n`, `\r`, `\t`,
  and any other control character as `\u00XX`.
- Floating-point values use `std::to_chars` shortest round-trip, so the
  output is the minimum number of decimal digits that reparse to the
  exact same binary value.

### Hashing

`<md/hash.hpp>` provides `std::hash` specializations for `Value`,
`Object`, and `Array`. A few things worth knowing:

- Per-alternative salt: `Value{0}`, `Value{0u}`, `Value{0.0f}`,
  `Value{0.0}`, and `Value{false}` all hash to different values.
- Arrays hash positionally (order matters).
- **Objects hash with a commutative XOR fold of per-entry hashes**, so
  two `Object`s that compare equal also hash equal regardless of
  insertion order. This makes `Object` usable as a key in an
  `unordered_*` container.
- The hash is **stable within a single process only**. Do not persist
  these hashes to disk or send them over a network expecting another
  process to reproduce them.

### Optional: nlohmann/json adapter

If `<nlohmann/json.hpp>` is on your include path, `<md/metadata.hpp>`
automatically pulls in `<md/json.hpp>`, which provides ADL hooks plus
convenience helpers:

```cpp
#include <md/metadata.hpp>
#include <nlohmann/json.hpp>          // include order doesn't matter

md::Object m{{"k", 1}};

// ADL hooks let nlohmann's converters Just Work.
nlohmann::json j = m;                            // -> to_json(json&, const Object&)
md::Value     v = j.get<md::Value>();            // -> from_json(const json&, Value&)

// Non-ADL convenience forms (slightly nicer at call sites).
nlohmann::json j2 = md::to_json(m);
md::Value      v2 = md::from_json(j2);
```

Integer routing in `from_json` follows nlohmann's parser-level
discrimination: `number_unsigned` → `uint64`, `number_integer` →
`int64`. A non-negative integer parsed from JSON text may come back as
either, depending on how nlohmann tokenized it. The test suite
accordingly accepts `is_int() || is_uint()` for that case.

**Float vs double does not survive a JSON round-trip.** nlohmann's
parser collapses every floating-point value to its `number_float`
(`double`) bucket. A `float` that goes out as JSON comes back as a
`double`. This is documented in `tests/test_json.cpp::FloatRoundTripsThroughDouble`.
There is no workaround inside this library — pick a custom wire format
if you need float fidelity.

`md::from_json(j, Object&)` throws `md::type_error` if `j` is not a JSON
object.

### Optional: Parcel adapter

If [`Parcel`](https://github.com/aurimasniekis/cpp-parcel) is on your
include path, `<md/metadata.hpp>` automatically pulls in `<md/parcel.hpp>`,
which exposes the three core types as primitive-style parcel cells:

| Cell             | Storage      | Wire kind |
|------------------|--------------|-----------|
| `md::ValueCell`  | `md::Value`  | `md:v`    |
| `md::ObjectCell` | `md::Object` | `md:o`    |
| `md::ArrayCell`  | `md::Array`  | `md:a`    |

Each cell derives from `parcel::BaseCell<…, Storage>` and reuses the
existing `<md/json.hpp>` ADL hooks for the inner JSON shape, so the
serialized cell is the standard parcel `{"k", "v"}` envelope with the
md value already inside the `"v"` slot:

```cpp
#include <md/metadata.hpp>          // also pulls in <md/parcel.hpp>
#include <parcel/parcel.h>

parcel::ParcelRegistry registry;
md::register_cells(registry);   // shorthand for the three register_kind calls

md::Object payload{
    {"name",     "sensor"},
    {"readings", md::Array{1.0, 2.5, 3.75}},
};

// Wrap and serialize.
md::ObjectCell cell{payload};
auto wire = cell.to_json();           // {"k":"md:o","v":{"name":...}}

// Round-trip back through the registry.
parcel::cell_t restored = registry.cell_from_json(wire);
auto* back = dynamic_cast<md::ObjectCell*>(restored.get());
const md::Object& restored_obj = back->value;
```

`PARCEL_DEFAULT_CELL` specializations are emitted for all three types,
so `parcel::cell(md::Value{42})` (and the equivalents for `Object` /
`Array`) automatically pick the right cell wrapper. A `FieldsBuilder`
field of type `md::Object` likewise infers `md::ObjectCell` without an
explicit `CellT` argument.

The same caveats apply as for the raw nlohmann/json adapter: floats
collapse to `double` on the way back from JSON, and `from_json` is
strict about the `"k"` tag matching the expected wire kind.

---

## Common usage patterns

### Building a tree by assignment

```cpp
md::Object o;
o["name"]   = "sensor";   // const char*  -> Value(string)
o["count"]  = 42;         // int          -> Value(int64)
o["weight"] = 3.14;       // double       -> Value(double)
o["ratio"]  = 1.5f;       // float        -> Value(float)  (stays float)
o["live"]   = true;       // bool         -> Value(bool)
```

Use braced lists for compound values. The two `operator=` overloads
disambiguate by element type — bare values pick the `Array` overload,
pair-shaped elements pick the `Object` overload:

```cpp
o["tags"] = {"alpha", "beta"};           // -> Array
o["sub"]  = {{"k", 1}, {"k2", 2}};       // -> Object
```

Same disambiguation works inside an `Object{...}` constructor list:

```cpp
md::Object root{
    {"name",    "default"},
    {"options", {{"retries", 3}, {"timeout_ms", 1000}}},  // nested Object
    {"tags",    md::Array{"a", "b"}},                     // explicit Array
};
```

### Reading values back out

There are three flavors, increasing in strictness:

```cpp
// 1) Pointer/noexcept form — best for "I'm not sure".
if (auto* s = o.get_string_if("name")) { std::cout << *s << '\n'; }

// 2) Strict typed form — throws on miss or type mismatch.
const std::string& name = o.require_string("name");

// 3) Path form — same throw/null behavior, deep access.
const md::Value& fw_major = o.require_path("device.firmware.major");
std::cout << fw_major.as_int() << '\n';
```

### Walking nested structures

```cpp
md::Object root{
    {"device",   {{"id", "abc-123"},
                  {"firmware", {{"major", 1}, {"minor", 4}}}}},
    {"channels", md::Array{
        md::Object{{"name", "left"},  {"gain_db",  0.5}},
        md::Object{{"name", "right"}, {"gain_db", -1.0}},
    }},
};

const md::Object& fw = root.require_object("device").require_object("firmware");
std::cout << fw.at("major").as_int() << '.' << fw.at("minor").as_int() << '\n';

for (const md::Value& ch : root.require_array("channels")) {
    const md::Object& c = ch.as_object();
    std::cout << c.at("name").as_string()
              << " gain=" << c.at("gain_db").as_double() << '\n';
}
```

This is also the recommended way to build an array of objects: use
`md::Array{...}` for the outer container and `md::Object{...}` for each
element. A braced list of brace-pair-shaped elements would not
disambiguate at the outer level (see *Edge cases* below).

### Layering configuration with `merge`

```cpp
md::Object base{
    {"name",    "default"},
    {"options", {{"retries", 3}, {"timeout_ms", 1000}}},
    {"tags",    md::Array{"a", "b"}},
};

const md::Object overlay{
    {"options",     {{"timeout_ms", 500}, {"strict", true}}},
    {"tags",        md::Array{"x"}},     // arrays are replaced
    {"description", "overridden"},
};

base.merge(overlay);
// base["options"] is now {retries:3, timeout_ms:500, strict:true}
// base["tags"]    is now ["x"]
// base["description"] was inserted
```

### Path-style access

```cpp
md::Object m{
    {"device",   {{"name", "acme"}, {"port", 8080}}},
    {"channels", md::Array{
        md::Object{{"freq_hz", 2.4e9}},
        md::Object{{"freq_hz", 5.8e9}},
    }},
};

m.require_path("device.name").as_string();         // "acme"
m.require_path("channels[0].freq_hz").as_double(); // 2.4e9

if (const md::Value* p = m.find_path("device.absent")) {
    // not reached
} else {
    // missing key -> nullptr, no exception
}

m.contains_path("channels[5]");   // false  (out of range)
m.contains_path("device.port");   // true
```

Mutating via path also works:

```cpp
md::Value* p = m.find_path("device.port");
if (p != nullptr) {
    *p = md::Value{9090};
}
```

### Formatted output

```cpp
md::Value v = md::Object{
    {"name",      "radio"},
    {"power_dbm", -10.5},
    {"channels",  md::Array{1, 2, 3}},
};

std::cout << std::format("{}", v) << '\n';
std::cout << v << '\n';
// Both: {"name":"radio","power_dbm":-10.5,"channels":[1,2,3]}
// (Key order is unspecified — see below.)
```

### Hashing into unordered containers

```cpp
#include <unordered_set>

std::unordered_set<md::Value> seen;
seen.insert(md::Value{42});
seen.insert(md::Value{md::Object{{"k", 1}}});
seen.contains(md::Value{42});   // true
```

The `Object` hash is intentionally commutative so insertion order doesn't
break the equality/hash contract.

### Round-tripping through nlohmann/json

```cpp
#include <nlohmann/json.hpp>
#include <md/metadata.hpp>

md::Object m{
    {"name",     "sensor"},
    {"enabled",  true},
    {"readings", md::Array{1.0, 2.5, 3.75}},
};

nlohmann::json j = md::to_json(m);
std::string text = j.dump();              // "{...}"

md::Value back = md::from_json(j);
```

---

## Error handling

The library reports errors through exceptions. There are three
project-specific exception types, all under `namespace md`:

```cpp
struct error             : std::runtime_error { /* ... */ };
struct missing_key_error : error              { /* ... */ };
struct type_error        : error              { /* ... */ };
```

| Mechanism                 | Thrown by                                                                                                                                                                  |
|---------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `missing_key_error`       | `Object::require`, `require_*`, `require_path` on a missing key                                                                                                            |
| `type_error`              | `require_*` on type mismatch; `require_path` on malformed path or type mismatch; `Value::as_double` on a non-number; `from_json(json, Object&)` on a non-object JSON value |
| `std::bad_variant_access` | Strict `as_bool` / `as_int` / `as_uint` / `as_float` / `as_string` / `as_array` / `as_object` on the wrong alternative                                                     |
| `std::out_of_range`       | `Object::at` on a missing key                                                                                                                                              |
| `std::format_error`       | A non-empty `std::format` spec on any of the supported types                                                                                                               |

If you want to avoid exceptions entirely, stay on the noexcept side of
the API: `as_*_if()`, `get_if<T>()`, `value_or<T>()`, `find_ptr()`,
`find_path()`, `contains`, `contains_path`.

```cpp
md::Value v;             // null
try {
    (void)v.as_int();    // throws std::bad_variant_access
} catch (const std::bad_variant_access&) {
    // ...
}

md::Object o;
try {
    (void)o.require("missing");
} catch (const md::missing_key_error& e) {
    std::cout << e.what() << '\n';
}
```

---

## Edge cases and pitfalls

These come straight from the implementation and tests — read them once
and most of the surprises go away.

### `Value{42}` is a scalar, never a one-element array

There is no `Value(std::initializer_list<Value>)` constructor — adding
one would silently change the meaning of every existing brace-init site.
If you want a one-element array, write `md::Array{42}`.

### Nested-array literals don't work

```cpp
m["x"] = {1, {2, 3}, 4};                     // does NOT compile cleanly
m["x"] = {1, md::Array{2, 3}, 4};            // OK
```

The inner `{2, 3}` cannot form a `Value` for the reason above. Use
`md::Array{}` explicitly for any nested array.

### Empty `{}` is ambiguous

```cpp
m["x"] = {};                  // ambiguous: Array? Object?
m["x"] = md::Array{};         // explicit empty Array
m["x"] = md::Object{};        // explicit empty Object
```

The two `operator=` overloads on `Value` both match an empty list. Be
explicit.

### Arrays of objects need an outer `md::Array{}`

```cpp
root["channels"] = md::Array{
    md::Object{{"name", "left"}},
    md::Object{{"name", "right"}},
};
```

This is the reliable form. A bare brace list of `Object`s would fall back
to the pair-shaped overload and not compile.

### `Object` keys are unordered

Insertion order is not preserved — the backing container is
`std::unordered_map`. Two `Object`s that compare equal may print their
keys in different orders, but they will hash equal and compare equal.
If you need stable text output, sort the keys yourself before printing.

### Float fidelity is lost through JSON

nlohmann's parser maps every floating-point JSON value to `double`. A
`Value{1.5f}` round-tripped through nlohmann comes back as `Value{1.5}`
(a `double`). See `tests/test_json.cpp::FloatRoundTripsThroughDouble`.

### `as_float()` is strict, `as_double()` is not

```cpp
md::Value d{2.25};      // double
(void)d.as_float();     // throws std::bad_variant_access — strict
(void)d.as_double();    // 2.25 — fine

md::Value i{42};        // int64
(void)i.as_double();    // 42.0 — widens
(void)i.as_int();       // 42  — strict, exact
```

If you want a "give me a number as a double regardless of how it's
stored", `as_double()` is the only one that does that.

### Path syntax is dot+bracket only

Keys containing `.` or `[` cannot be addressed by `find_path` / `require_path`.
Use `Object::operator[]` or `find` to reach them. The path parser also
rejects:

- a leading `.` (`.a`)
- two adjacent dots (`..a`)
- an unclosed bracket (`items[`)
- a non-digit inside brackets (`items[abc]`)
- an empty bracket pair (`items[]`)

### Path of `""` returns null, not the root

`find_path("")` is `nullptr`, `contains_path("")` is `false`,
`require_path("")` throws `type_error`. The root is an `Object`, not a
`Value`, so there is nothing for the function to hand back.

### Hashes are process-local

Do not persist values returned by `std::hash<md::Value>` (or `Object` /
`Array`). The mixing constants are fine for an in-memory hash table only.

### Format spec is empty-only

`std::format("{:p}", v)` throws `std::format_error`. There is no
pretty-printing option in v1.

### Copying `Value` is a real copy

A `Value` holding an `Object` stores it inside a `std::unique_ptr<Object>`
(to break the recursive type), but copy construction deep-copies the
contained `Object`. Move construction is `noexcept`.

---

## API overview

A compact map of the public surface a typical user touches. This is not
a full API dump — read the headers for the full list of overloads.

| Symbol                                               | Purpose                                                            |
|------------------------------------------------------|--------------------------------------------------------------------|
| `md::Value`                                          | The variant type. Predicates + strict + noexcept accessors.        |
| `md::Object` (`md::Metadata`)                        | The unordered-map-shaped container with metadata helpers.          |
| `md::Array`                                          | Type alias for `std::vector<md::Value>`.                           |
| `md::null()`, `md::boolean(b)`                       | Factory helpers for the trivial values.                            |
| `md::number<T>(x)`                                   | Factory helper constrained to numeric types.                       |
| `md::string(...)`                                    | Factory helper from `std::string` / `string_view` / `const char*`. |
| `md::array()`, `md::array({...})`                    | Array factory helpers.                                             |
| `md::object()`, `md::object({...})`                  | Object factory helpers.                                            |
| `Object::operator[]`, `at`, `find`, `contains`, …    | The `std::unordered_map` surface.                                  |
| `Object::require[...]`, `get_*_if`                   | Typed retrieval with / without exceptions.                         |
| `Object::find_path`, `require_path`, `contains_path` | Dot+bracket path access.                                           |
| `Object::merge(src)`                                 | Deep merge, source wins.                                           |
| `md::contains(o,k)`, `md::require_string(o,k)`, …    | Free-function forwarders for the helper methods.                   |
| `operator<<`, `std::formatter<...>`                  | Compact JSON output (empty spec only).                             |
| `std::hash<Value/Object/Array>`                      | Hashing for unordered containers.                                  |
| `md::to_json`, `md::from_json`                       | nlohmann/json adapter (header `<md/json.hpp>`).                    |
| `md::error`, `missing_key_error`, `type_error`       | Exception hierarchy.                                               |

---

## Examples

The `examples/` directory contains short, runnable programs:

| File                            | Demonstrates                                                 |
|---------------------------------|--------------------------------------------------------------|
| `examples/basic.cpp`            | Building a small `Metadata`, printing it, simple retrieval.  |
| `examples/nested.cpp`           | Nested objects, arrays of objects, deep `require_*` access.  |
| `examples/path_lookup.cpp`      | `require_path`, `find_path`, `contains_path` (hit and miss). |
| `examples/merge.cpp`            | Deep merge with array replacement and new-key insertion.     |
| `examples/format_output.cpp`    | `std::format` vs `operator<<` on the same value.             |
| `examples/object_helpers.cpp`   | Method form vs free-function form, optional + strict access. |
| `examples/json_integration.cpp` | Round-trip through `nlohmann::json`.                         |

All example programs are wired up in `examples/CMakeLists.txt`. When
`METADATA_BUILD_EXAMPLES=ON` (the default at the top level), CMake builds
them as `metadata_basic`, `metadata_nested`, etc.

---

## Building and testing

The repository ships a thin `Makefile` over the CMake build for the
common workflows. The CMake commands directly are equally fine:

```bash
# Configure + build + test
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Or via the wrapper:

```bash
make build          # configure + build in build/        (Debug)
make test           # ctest in build/
make examples       # build and run every metadata_* example, fail on non-zero exit
make sanitize       # Debug build + tests with ASan + UBSan in build-san/
make tidy           # Debug build with clang-tidy in build-tidy/
make release        # Release build + tests in build-release/
make no-json        # Build + test with METADATA_WITH_NLOHMANN_JSON=OFF
make coverage       # Clang source-based coverage in build-coverage/
make docs           # Doxygen HTML in build-docs/
make format         # clang-format -i over include / tests / examples
make ci             # format-check + tidy + test + sanitize + release + no-json
```

GoogleTest 1.17 is fetched automatically via `FetchContent` if no
`GTest` package is found.

### Build options

| CMake option                  | Default        | Effect                                                                |
|-------------------------------|----------------|-----------------------------------------------------------------------|
| `METADATA_BUILD_TESTS`        | top-level only | Build the GoogleTest suite.                                           |
| `METADATA_BUILD_EXAMPLES`     | top-level only | Build every example target.                                           |
| `METADATA_BUILD_DOCS`         | `OFF`          | Configure the Doxygen target (`metadata_docs`).                       |
| `METADATA_WITH_NLOHMANN_JSON` | `ON`           | Link `nlohmann/json` and define `METADATA_WITH_NLOHMANN_JSON=1`.      |
| `METADATA_ENABLE_SANITIZERS`  | `OFF`          | ASan + UBSan flags (Debug).                                           |
| `METADATA_ENABLE_CLANG_TIDY`  | `OFF`          | Run clang-tidy during the build.                                      |
| `METADATA_ENABLE_COVERAGE`    | `OFF`          | Clang source-based coverage flags.                                    |
| `METADATA_WARNINGS_AS_ERRORS` | top-level only | Promote compiler warnings to errors.                                  |
| `METADATA_INSTALL`            | top-level only | Generate install rules; auto-disabled if `nlohmann_json` was fetched. |

---

## FAQ

**Do I need to link anything?**
No. `metadata::metadata` is a CMake `INTERFACE` target — it adds the
include directories and turns on C++23. The library is header-only.

**Is it thread-safe?**
No more than `std::unordered_map<std::string, std::variant<...>>` is.
Concurrent reads of an unchanged `Object` are fine. Concurrent writes,
or a write concurrent with any read, require external synchronization.
Hash functions are pure and noexcept.

**Does `Value` own its data or borrow it?**
`Value` owns its data. `Value(std::string_view)` and `Value(const char*)`
copy into a `std::string`. `Value(Object)` moves into an internal
`std::unique_ptr<Object>` (the indirection breaks the recursive type
relationship between `Value` and `Object`).

**Why is `Object` wrapped in a `unique_ptr` inside `Value`?**
Because `std::variant<..., Object>` would need `Object` complete at the
point where `Value` is defined — and `Object` itself stores `Value`s.
The `unique_ptr<Object>` indirection breaks the cycle and keeps the
public API normal-looking (you still write `Value{Object{...}}`).

**Can I parse JSON text with this?**
No. There is no parser. Use nlohmann/json (or any other JSON parser),
then `md::from_json(json)` to import the result.

**Can I keep insertion order?**
Not in v1. The `Object` storage is `std::unordered_map`. An ordered
variant is listed as a possible future direction.

**Why does my float become a double after JSON round-trip?**
nlohmann's parser collapses all floating-point JSON to `double`. There
is nothing this library can do about it on the parser side. See the
*Edge cases* section.

**Why does `std::format("{:p}", v)` throw?**
The compact JSON writer is the only output mode in v1; the format-spec
parser rejects any non-empty spec with `std::format_error`.

**Why is `as_int()` strict but `as_double()` widening?**
`as_double()` is intentionally the one accessor that papers over the
int/uint/float/double distinction, for code that just wants "a number as
a double". Every other accessor is strict so that bugs are loud.

**Does `Object::operator[]` create missing keys?**
Yes — same as `std::unordered_map`. It default-constructs a `Value`
(which is `null`) under the missing key. If you don't want that, use
`find`, `contains`, or `find_ptr` instead.


## Contributing

Contributions to the library are welcome! If you encounter any issues or have suggestions for
improvements,
please feel free to submit a pull request or open an issue on the project's repository.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
