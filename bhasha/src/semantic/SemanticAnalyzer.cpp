#include "SemanticAnalyzer.h"

#include <utility>

namespace bhasha {

void SemanticAnalyzer::errorAt(int line, const std::string& msg) {
    errors_.push_back({line, msg});
}

bool SemanticAnalyzer::analyze(const std::vector<StmtPtr>& program) {
    symbols_.pushScope();
    for (const auto& s : program) {
        if (s) analyzeStmt(*s);
    }
    symbols_.popScope();
    return errors_.empty();
}

void SemanticAnalyzer::analyzeBlock(const BlockStmt& b) {
    symbols_.pushScope();
    for (const auto& s : b.statements) {
        if (s) analyzeStmt(*s);
    }
    symbols_.popScope();
}

void SemanticAnalyzer::analyzeStmt(const Statement& s) {
    if (auto* d = dynamic_cast<const VarDeclStmt*>(&s)) {
        Type rhsT = d->initializer ? analyzeExpr(*d->initializer) : Type::UNKNOWN;
        if (rhsT == Type::UNKNOWN) {
            // Already reported.
        } else if (d->type == Type::NUMBER && rhsT != Type::NUMBER) {
            errorAt(d->line,
                "'" + typeNameBangla(d->type) + "' ধরনের ভেরিয়েবলে '" +
                typeNameBangla(rhsT) + "' মান রাখা যাবে না।\n\nআপনি লিখেছেন:\n  " +
                typeNameBangla(d->type) + " " + d->name + " = ...");
        } else if (d->type == Type::TEXT && rhsT != Type::TEXT) {
            errorAt(d->line,
                "'" + typeNameBangla(d->type) + "' ধরনের ভেরিয়েবলে '" +
                typeNameBangla(rhsT) + "' মান রাখা যাবে না।\n\nআপনি লিখেছেন:\n  " +
                typeNameBangla(d->type) + " " + d->name + " = ...");
        }
        if (!symbols_.declare(d->name, d->type, d->line)) {
            auto existing = symbols_.lookup(d->name);
            std::string prev = existing ? typeNameBangla(existing->type) : "?";
            errorAt(d->line,
                "'" + d->name + "' ভেরিয়েবলটি ইতিমধ্যে ঘোষণা করা হয়েছে (" + prev + " ধরনের)।\n\nপরামর্শ:\n" +
                "  একই নামে দ্বিতীয়বার ঘোষণা করবেন না; পুরোনো মান বদলাতে শুধু বরাদ্দ ব্যবহার করুন:\n" +
                "    " + d->name + " = নতুন_মান;");
        }
    } else if (auto* a = dynamic_cast<const AssignStmt*>(&s)) {
        auto sym = symbols_.lookup(a->name);
        if (!sym) {
            errorAt(a->line,
                "'" + a->name + "' ভেরিয়েবলটি আগে ঘোষণা করা হয়নি।\n\nপরামর্শ:\n" +
                "  প্রথমে লিখুন:\n\n    সংখ্যা " + a->name + " = 0;\n  অথবা\n    লেখা " + a->name + " = \"\";");
        } else {
            Type rhsT = a->value ? analyzeExpr(*a->value) : Type::UNKNOWN;
            if (rhsT != Type::UNKNOWN && sym->type != rhsT) {
                errorAt(a->line,
                    "'" + a->name + "' হলো '" + typeNameBangla(sym->type) +
                    "' ধরনের, কিন্তু আপনি '" + typeNameBangla(rhsT) + "' মান দিচ্ছেন।");
            }
        }
    } else if (auto* p = dynamic_cast<const PrintStmt*>(&s)) {
        if (p->value) analyzeExpr(*p->value);
    } else if (auto* i = dynamic_cast<const IfStmt*>(&s)) {
        Type t = i->condition ? analyzeExpr(*i->condition) : Type::UNKNOWN;
        if (t != Type::UNKNOWN && t != Type::BOOL) {
            errorAt(i->line, "'যদি' এর শর্তটি অবশ্যই একটি তুলনা (হ্যাঁ/না) হতে হবে, যেমন: x >= 10");
        }
        if (i->thenBranch) analyzeStmt(*i->thenBranch);
        if (i->elseBranch) analyzeStmt(*i->elseBranch);
    } else if (auto* w = dynamic_cast<const WhileStmt*>(&s)) {
        Type t = w->condition ? analyzeExpr(*w->condition) : Type::UNKNOWN;
        if (t != Type::UNKNOWN && t != Type::BOOL) {
            errorAt(w->line, "'যতক্ষণ' এর শর্তটি অবশ্যই একটি তুলনা হতে হবে, যেমন: i <= 5");
        }
        if (w->body) analyzeStmt(*w->body);
    } else if (auto* b = dynamic_cast<const BlockStmt*>(&s)) {
        analyzeBlock(*b);
    }
}

Type SemanticAnalyzer::analyzeExpr(const Expression& e) {
    if (auto* l = dynamic_cast<const LiteralExpr*>(&e)) {
        switch (l->kind) {
            case LiteralExpr::Kind::NUMBER: return Type::NUMBER;
            case LiteralExpr::Kind::STRING: return Type::TEXT;
            case LiteralExpr::Kind::BOOL:   return Type::BOOL;
        }
    }
    if (auto* v = dynamic_cast<const VariableExpr*>(&e)) {
        auto sym = symbols_.lookup(v->name);
        if (!sym) {
            // We don't know the line on the variable expression node;
            // report with line 0 and the message will still be helpful.
            errorAt(0,
                "'" + v->name + "' ভেরিয়েবলটি আগে ঘোষণা করা হয়নি।\n\nপরামর্শ:\n" +
                "  প্রথমে লিখুন:\n\n    সংখ্যা " + v->name + " = 0;\n  অথবা\n    লেখা " + v->name + " = \"\";");
            return Type::UNKNOWN;
        }
        return sym->type;
    }
    if (auto* u = dynamic_cast<const UnaryExpr*>(&e)) {
        Type t = u->operand ? analyzeExpr(*u->operand) : Type::UNKNOWN;
        if (u->op == UnaryExpr::Op::NEG && t != Type::NUMBER && t != Type::UNKNOWN) {
            errorAt(0, "একমাত্র সংখ্যামূলক মানের উপর '-' ব্যবহার করা যায়।");
            return Type::UNKNOWN;
        }
        return Type::NUMBER;
    }
    if (auto* b = dynamic_cast<const BinaryExpr*>(&e)) {
        Type lhsT = b->lhs ? analyzeExpr(*b->lhs) : Type::UNKNOWN;
        Type rhsT = b->rhs ? analyzeExpr(*b->rhs) : Type::UNKNOWN;
        return checkBinaryOp(b->op, lhsT, rhsT, 0);
    }
    return Type::UNKNOWN;
}

Type SemanticAnalyzer::checkBinaryOp(BinaryExpr::Op op, Type lhs, Type rhs, int line) {
    using Op = BinaryExpr::Op;
    auto bad = [&](const std::string& why) {
        errorAt(line, why);
    };
    bool isComparison = (op == Op::GT || op == Op::LT || op == Op::GTE ||
                         op == Op::LTE || op == Op::EQEQ || op == Op::NEQ);
    bool isArithmetic = (op == Op::ADD || op == Op::SUB ||
                         op == Op::MUL || op == Op::DIV);

    if (isArithmetic) {
        if (lhs == Type::NUMBER && rhs == Type::NUMBER) return Type::NUMBER;
        if (lhs != Type::UNKNOWN && rhs != Type::UNKNOWN) {
            bad("'" + std::string(binaryOpString(op)) +
                "' শুধু সংখ্যার উপর ব্যবহার করা যায়।");
        }
        return Type::UNKNOWN;
    }
    if (isComparison) {
        if (op == Op::EQEQ || op == Op::NEQ) {
            if (lhs == Type::UNKNOWN || rhs == Type::UNKNOWN) return Type::BOOL;
            if (lhs == rhs) return Type::BOOL;
            bad("'" + std::string(binaryOpString(op)) +
                "' একই ধরনের মানের উপর ব্যবহার করা যায়।");
            return Type::BOOL;
        }
        // Ordering comparisons only make sense for numbers.
        if (lhs == Type::NUMBER && rhs == Type::NUMBER) return Type::BOOL;
        if (lhs != Type::UNKNOWN && rhs != Type::UNKNOWN) {
            bad("'" + std::string(binaryOpString(op)) +
                "' শুধু সংখ্যার উপর ব্যবহার করা যায়।");
        }
        return Type::BOOL;
    }
    return Type::UNKNOWN;
}

} // namespace bhasha
