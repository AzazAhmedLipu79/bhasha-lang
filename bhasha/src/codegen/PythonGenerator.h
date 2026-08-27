#pragma once

#include "../ast/AST.h"

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace bhasha {

// Translates a validated Bhasha AST into a Python source string.
class PythonGenerator {
public:
    // Generates Python source from a list of statements and returns it.
    std::string generate(const std::vector<StmtPtr>& program);

    // For testing: generate to a specific stream.
    void generateTo(const std::vector<StmtPtr>& program, std::ostream& out);

private:
    std::ostringstream out_;
    int indent_{0};

    void emitLine(const std::string& line);
    std::string ind() const { return std::string(indent_ * 4, ' '); }

    void genStmt(const Statement& s);
    void genBlock(const BlockStmt& b);
    std::string genExpr(const Expression& e);
};

} // namespace bhasha
