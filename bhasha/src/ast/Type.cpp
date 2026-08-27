#include "Type.h"

namespace bhasha {

std::string typeNameBangla(Type t) {
    switch (t) {
        case Type::NUMBER:  return "সংখ্যা";
        case Type::TEXT:    return "লেখা";
        case Type::BOOL:    return "বুলিয়ান";
        case Type::UNKNOWN: return "অজানা";
    }
    return "?";
}

} // namespace bhasha
