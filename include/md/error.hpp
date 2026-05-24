#pragma once

#include <stdexcept>

namespace md {

// NOLINTBEGIN(readability-identifier-naming)
/// Base class for all exceptions thrown by the md library.
struct error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Thrown when a required key or path is not present in an Object.
struct missing_key_error : error {
    using error::error;
};

/// Thrown when a Value holds a different alternative than the caller demanded.
struct type_error : error {
    using error::error;
};
// NOLINTEND(readability-identifier-naming)

}  // namespace md
