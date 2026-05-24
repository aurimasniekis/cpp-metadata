#pragma once

#include <md/error.hpp>
#include <md/object.hpp>
#include <md/value.hpp>

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

namespace md {

namespace detail {

enum class PathSegmentKind {
    Key,
    Index,
    End,
    Malformed,
};

struct PathSegment {
    PathSegmentKind kind = PathSegmentKind::End;
    std::string_view key;
    std::size_t index = 0;
};

// Pulls one segment from `path` starting at `pos` and updates `pos` past it.
// On success returns Key or Index. Returns End when the path is exhausted.
// Returns Malformed for unparsable input (e.g. unmatched bracket).
inline PathSegment next_segment(const std::string_view path, std::size_t& pos) {
    PathSegment out;
    if (pos >= path.size()) {
        out.kind = PathSegmentKind::End;
        return out;
    }

    const char c = path[pos];

    if (c == '[') {
        ++pos;
        const std::size_t start = pos;
        while (pos < path.size() && path[pos] != ']') {
            if (path[pos] < '0' || path[pos] > '9') {
                out.kind = PathSegmentKind::Malformed;
                return out;
            }
            ++pos;
        }
        if (pos >= path.size() || pos == start) {
            // Either missing ']' or empty []
            out.kind = PathSegmentKind::Malformed;
            return out;
        }
        std::size_t idx = 0;
        for (std::size_t i = start; i < pos; ++i) {
            idx = (idx * 10) + static_cast<std::size_t>(path[i] - '0');
        }
        ++pos;  // skip ']'
        out.kind = PathSegmentKind::Index;
        out.index = idx;
        return out;
    }

    if (c == '.') {
        // A leading '.' is malformed; a trailing '.' before next segment is also
        // unusual. We consume the dot and expect a key to follow.
        ++pos;
        if (pos >= path.size() || path[pos] == '.' || path[pos] == '[') {
            out.kind = PathSegmentKind::Malformed;
            return out;
        }
        return next_segment(path, pos);
    }

    // Read an identifier segment until '.' or '['.
    const std::size_t start = pos;
    while (pos < path.size() && path[pos] != '.' && path[pos] != '[') {
        ++pos;
    }
    out.kind = PathSegmentKind::Key;
    out.key = path.substr(start, pos - start);
    return out;
}

// Returns nullptr on miss, sets `malformed` to true on bad syntax.
inline const Value* walk_path(const Object& root, const std::string_view path, bool& malformed) {
    malformed = false;
    if (path.empty()) {
        // The root itself isn't a Value — but for symmetry, callers can
        // interpret an empty path as "the object". We return nullptr to
        // signal "no Value to return" and let callers handle root specially.
        return nullptr;
    }

    std::size_t pos = 0;
    PathSegment seg = next_segment(path, pos);
    if (seg.kind == PathSegmentKind::Malformed) {
        malformed = true;
        return nullptr;
    }
    if (seg.kind != PathSegmentKind::Key) {
        // Path must start with an identifier (root is an Object).
        malformed = true;
        return nullptr;
    }

    const Value* cur = root.find_ptr(seg.key);
    if (cur == nullptr) {
        return nullptr;
    }

    while (true) {
        seg = next_segment(path, pos);
        if (seg.kind == PathSegmentKind::End) {
            return cur;
        }
        if (seg.kind == PathSegmentKind::Malformed) {
            malformed = true;
            return nullptr;
        }
        if (seg.kind == PathSegmentKind::Key) {
            const Object* obj = cur->as_object_if();
            if (obj == nullptr) {
                malformed = true;  // type mismatch — require_path() will throw type_error
                return nullptr;
            }
            cur = obj->find_ptr(seg.key);
            if (cur == nullptr) {
                return nullptr;
            }
        } else {  // Index
            const Array* arr = cur->as_array_if();
            if (arr == nullptr) {
                malformed = true;
                return nullptr;
            }
            if (seg.index >= arr->size()) {
                return nullptr;
            }
            cur = &(*arr)[seg.index];
        }
    }
}

inline Value* walk_path_mut(Object& root, const std::string_view path, bool& malformed) {
    // Reuse the const walker by stripping const off the result — the underlying
    // storage is non-const (we have a non-const Object&), so this is safe.
    const Object& croot = root;
    const Value* p = walk_path(croot, path, malformed);
    return const_cast<Value*>(p);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

}  // namespace detail

inline const Value* Object::find_path(const std::string_view path) const {
    bool malformed = false;
    return detail::walk_path(*this, path, malformed);
}

inline Value* Object::find_path(const std::string_view path) {
    bool malformed = false;
    return detail::walk_path_mut(*this, path, malformed);
}

inline const Value& Object::require_path(const std::string_view path) const {
    bool malformed = false;
    if (const Value* p = detail::walk_path(*this, path, malformed); p != nullptr) {
        return *p;
    }
    if (malformed) {
        throw type_error("Object::require_path: malformed path or type mismatch: '" +
                         std::string(path) + "'");
    }
    throw missing_key_error("Object::require_path: not found: '" + std::string(path) + "'");
}

inline Value& Object::require_path(const std::string_view path) {
    bool malformed = false;
    if (Value* p = detail::walk_path_mut(*this, path, malformed); p != nullptr) {
        return *p;
    }
    if (malformed) {
        throw type_error("Object::require_path: malformed path or type mismatch: '" +
                         std::string(path) + "'");
    }
    throw missing_key_error("Object::require_path: not found: '" + std::string(path) + "'");
}

inline bool Object::contains_path(const std::string_view path) const {
    bool malformed = false;
    return detail::walk_path(*this, path, malformed) != nullptr;
}

}  // namespace md
