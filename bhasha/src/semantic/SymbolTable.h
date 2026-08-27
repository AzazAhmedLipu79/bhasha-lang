#pragma once

#include "../ast/Type.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace bhasha {

// One entry in the symbol table.
struct Symbol {
    std::string name;
    Type type{Type::UNKNOWN};
    int line{1};
};

// A scoped symbol table. Supports pushScope/popScope for nested blocks.
class SymbolTable {
public:
    void pushScope() { scopes_.emplace_back(); }
    void popScope()  { if (!scopes_.empty()) scopes_.pop_back(); }

    // Declares a new variable in the current scope.
    // Returns false if the name is already declared in the current scope.
    bool declare(const std::string& name, Type type, int line);

    // Looks up a variable in any visible scope. Returns nullopt if not found.
    std::optional<Symbol> lookup(const std::string& name) const;

    // For pretty-printing.
    void dump() const;

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};

} // namespace bhasha
