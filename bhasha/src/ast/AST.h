#pragma once

#include "Type.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bhasha {

// Forward declarations.
struct Expression;
struct Statement;

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr  = std::unique_ptr<Statement>;

// ----- Base classes -----------------------------------------------------------

struct Expression {
    virtual ~Expression() = default;
};

struct Statement {
    virtual ~Statement() = default;
};

// ----- Expressions ------------------------------------------------------------

// A literal number, string, or boolean (booleans only come from comparisons).
struct LiteralExpr : Expression {
    enum class Kind { NUMBER, STRING, BOOL };
    Kind kind;
    long long number{0};
    std::string text;     // used for strings (decoded UTF-8)
    bool boolean{false};

    static ExprPtr makeNumber(long long v) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::NUMBER; e->number = v; return e;
    }
    static ExprPtr makeString(std::string s) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::STRING; e->text = std::move(s); return e;
    }
    static ExprPtr makeBool(bool b) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::BOOL; e->boolean = b; return e;
    }
};

// Reference to a previously declared variable.
struct VariableExpr : Expression {
    std::string name;
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
};

// Arithmetic or comparison binary expression.
struct BinaryExpr : Expression {
    enum class Op {
        ADD, SUB, MUL, DIV,
        GT, LT, GTE, LTE, EQEQ, NEQ,
    };
    Op op;
    ExprPtr lhs, rhs;

    BinaryExpr(Op o, ExprPtr l, ExprPtr r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

// Unary minus: -expr.
struct UnaryExpr : Expression {
    enum class Op { NEG };
    Op op;
    ExprPtr operand;
    UnaryExpr(Op o, ExprPtr e) : op(o), operand(std::move(e)) {}
};

// Convenience constructor helpers.
inline ExprPtr makeBinary(BinaryExpr::Op op, ExprPtr l, ExprPtr r) {
    return std::make_unique<BinaryExpr>(op, std::move(l), std::move(r));
}
inline ExprPtr makeUnary(UnaryExpr::Op op, ExprPtr e) {
    return std::make_unique<UnaryExpr>(op, std::move(e));
}
inline ExprPtr makeVariable(std::string name) {
    return std::make_unique<VariableExpr>(std::move(name));
}

// ----- Statements -------------------------------------------------------------

// Variable declaration with initializer:  সংখ্যা x = 10;
struct VarDeclStmt : Statement {
    Type type{Type::UNKNOWN};
    std::string name;
    ExprPtr initializer;
    int line{1};
};

// Assignment: x = 20;
struct AssignStmt : Statement {
    std::string name;
    ExprPtr value;
    int line{1};
};

// Output: দেখাও(expr);
struct PrintStmt : Statement {
    ExprPtr value;
    int line{1};
};

// Block: { stmts }
struct BlockStmt : Statement {
    std::vector<StmtPtr> statements;
};

// If statement with optional else.
struct IfStmt : Statement {
    ExprPtr condition;
    StmtPtr thenBranch;     // typically a BlockStmt
    StmtPtr elseBranch;     // may be nullptr, BlockStmt, or another IfStmt
    int line{1};
};

// While loop.
struct WhileStmt : Statement {
    ExprPtr condition;
    StmtPtr body;           // typically a BlockStmt
    int line{1};
};

// ----- Convenience constructors ----------------------------------------------

inline StmtPtr makeBlock(std::vector<StmtPtr> stmts) {
    auto b = std::make_unique<BlockStmt>();
    b->statements = std::move(stmts);
    return b;
}

inline const char* binaryOpString(BinaryExpr::Op op) {
    switch (op) {
        case BinaryExpr::Op::ADD:  return "+";
        case BinaryExpr::Op::SUB:  return "-";
        case BinaryExpr::Op::MUL:  return "*";
        case BinaryExpr::Op::DIV:  return "/";
        case BinaryExpr::Op::GT:   return ">";
        case BinaryExpr::Op::LT:   return "<";
        case BinaryExpr::Op::GTE:  return ">=";
        case BinaryExpr::Op::LTE:  return "<=";
        case BinaryExpr::Op::EQEQ: return "==";
        case BinaryExpr::Op::NEQ:  return "!=";
    }
    return "?";
}

} // namespace bhasha
