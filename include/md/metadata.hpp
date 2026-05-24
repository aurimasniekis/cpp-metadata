#pragma once

/// @file
/// @brief Umbrella header. Including this pulls in the full public md
///        API. The nlohmann/json adapter is conditional on
///        `<nlohmann/json.hpp>` being on the include path at compile time.

#include <md/error.hpp>
#include <md/format.hpp>
#include <md/hash.hpp>
#include <md/helpers.hpp>
#include <md/object.hpp>
#include <md/ostream.hpp>
#include <md/path.hpp>
#include <md/value.hpp>
#include <md/version.hpp>

#if __has_include(<nlohmann/json.hpp>)
#include <md/json.hpp>
#endif

#if __has_include(<parcel/parcel.h>)
#include <md/parcel.hpp>
#endif
