#include "PythonGenerator.h"

#include <utility>

namespace bhasha {

namespace {

// Escape a string for Python double-quoted literals.
std::string escapePythonString(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': r += "\\\\"; break;
            case '"':  r += "\\\""; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            default:   r += c;       break;
        }
    }
    return r;
}

} // namespace

void PythonGenerator::emitLine(const std::string& line) {
    out_ << ind() << line << "\n";
}

void PythonGenerator::generateTo(const std::vector<StmtPtr>& program, std::ostream& out) {
    out_ = std::ostringstream();
    indent_ = 0;
    for (const auto& s : program) {
        if (s) genStmt(*s);
    }
    out << out_.str();
}

std::string PythonGenerator::generate(const std::vector<StmtPtr>& program) {
    std::ostringstream sink;
    generateTo(program, sink);
    return sink.str();
}

void PythonGenerator::genStmt(const Statement& s) {
    if (auto* d = dynamic_cast<const VarDeclStmt*>(&s)) {
        std::string init = d->initializer ? genExpr(*d->initializer) : "0";
        emitLine(d->name + " = " + init);
        return;
    }
    if (auto* a = dynamic_cast<const AssignStmt*>(&s)) {
        std::string v = a->value ? genExpr(*a->value) : "0";
        emitLine(a->name + " = " + v);
        return;
    }
    if (auto* p = dynamic_cast<const PrintStmt*>(&s)) {
        std::string v = p->value ? genExpr(*p->value) : "0";
        emitLine("print(" + v + ")");
        return;
    }
    if (auto* i = dynamic_cast<const IfStmt*>(&s)) {
        std::string cond = i->condition ? genExpr(*i->condition) : "False";
        emitLine("if " + cond + ":");
        indent_++;
        if (i->thenBranch) genStmt(*i->thenBranch);
        indent_--;
        if (i->elseBranch) {
            // else if: IfStmt directly
            if (auto* nested = dynamic_cast<const IfStmt*>(i->elseBranch.get())) {
                // Render "elif cond:" instead of "else: if cond:"
                std::string cond2 = nested->condition ? genExpr(*nested->condition) : "False";
                emitLine("elif " + cond2 + ":");
                indent_++;
                if (nested->thenBranch) genStmt(*nested->thenBranch);
                indent_--;
                if (nested->elseBranch) {
                    emitLine("else:");
                    indent_++;
                    genStmt(*nested->elseBranch);
                    indent_--;
                }
                return;
            }
            emitLine("else:");
            indent_++;
            genStmt(*i->elseBranch);
            indent_--;
        }
        return;
    }
    if (auto* w = dynamic_cast<const WhileStmt*>(&s)) {
        std::string cond = w->condition ? genExpr(*w->condition) : "False";
        emitLine("while " + cond + ":");
        indent_++;
        if (w->body) genStmt(*w->body);
        indent_--;
        return;
    }
    if (auto* b = dynamic_cast<const BlockStmt*>(&s)) {
        genBlock(*b);
        return;
    }
}

void PythonGenerator::genBlock(const BlockStmt& b) {
    for (const auto& s : b.statements) {
        if (s) genStmt(*s);
    }
}

std::string PythonGenerator::genExpr(const Expression& e) {
    if (auto* l = dynamic_cast<const LiteralExpr*>(&e)) {
        switch (l->kind) {
            case LiteralExpr::Kind::NUMBER:
                return std::to_string(l->number);
            case LiteralExpr::Kind::STRING:
                return "\"" + escapePythonString(l->text) + "\"";
            case LiteralExpr::Kind::BOOL:
                return l->boolean ? "True" : "False";
        }
    }
    if (auto* v = dynamic_cast<const VariableExpr*>(&e)) {
        return v->name;
    }
    if (auto* u = dynamic_cast<const UnaryExpr*>(&e)) {
        std::string inner = u->operand ? genExpr(*u->operand) : "0";
        if (u->op == UnaryExpr::Op::NEG) return "-" + inner;
        return inner;
    }
    if (auto* b = dynamic_cast<const BinaryExpr*>(&e)) {
        std::string lhs = b->lhs ? genExpr(*b->lhs) : "0";
        std::string rhs = b->rhs ? genExpr(*b->rhs) : "0";
        // Bhasha's only numeric type is 'সংখ্যা' (integer), so '/' performs
        // integer division and maps to Python's '//' operator.
        const char* opStr = (b->op == BinaryExpr::Op::DIV) ? "//"
                                                           : binaryOpString(b->op);
        return "(" + lhs + " " + opStr + " " + rhs + ")";
    }
    return "0";
}

} // namespace bhasha
