#pragma once

#include "../ast/AST.h"
#include "SymbolTable.h"

#include <string>
#include <vector>

namespace bhasha {

struct SemanticError {
    int line{1};
    std::string message;
};

class SemanticAnalyzer {
public:
    // Validates a list of statements. Returns true if there were no errors.
    // If any error is reported, code generation should be skipped.
    bool analyze(const std::vector<StmtPtr>& program);

    const std::vector<SemanticError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

    const SymbolTable& getSymbolTable() const { return symbols_; }

private:
    SymbolTable symbols_;
    std::vector<SemanticError> errors_;

    void errorAt(int line, const std::string& msg);

    void analyzeStmt(const Statement& s);
    void analyzeBlock(const BlockStmt& b);

    Type analyzeExpr(const Expression& e);

    // Type compatibility for binary expressions.
    Type checkBinaryOp(BinaryExpr::Op op, Type lhs, Type rhs, int line);
};

} // namespace bhasha
