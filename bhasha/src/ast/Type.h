#pragma once

#include <string>

namespace bhasha {

// Bhasha's simple type system.
enum class Type {
    NUMBER,
    TEXT,
    BOOL,
    UNKNOWN,  // used when type inference fails (error recovery)
};

// Human-readable Bangla type names.
std::string typeNameBangla(Type t);

} // namespace bhasha
