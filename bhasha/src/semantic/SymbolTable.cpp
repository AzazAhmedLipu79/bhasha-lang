#include "SymbolTable.h"

#include <cstdio>

namespace bhasha {

bool SymbolTable::declare(const std::string& name, Type type, int line) {
    if (scopes_.empty()) pushScope();
    auto& cur = scopes_.back();
    if (cur.count(name)) return false;
    cur.emplace(name, Symbol{name, type, line});
    return true;
}

std::optional<Symbol> SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return std::nullopt;
}

void SymbolTable::dump() const {
    std::printf("Symbol table (%zu scopes)\n", scopes_.size());
    for (size_t i = 0; i < scopes_.size(); ++i) {
        std::printf("  [scope %zu]\n", i);
        for (const auto& [name, sym] : scopes_[i]) {
            std::printf("    %s : %s  (line %d)\n",
                        name.c_str(), typeNameBangla(sym.type).c_str(), sym.line);
        }
    }
}

} // namespace bhasha
