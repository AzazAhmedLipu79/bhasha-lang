#include "Parser.h"

#include <utility>

namespace bhasha {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

const Token& Parser::advance() {
    if (!isAtEnd()) ++pos_;
    return previous();
}

const Token& Parser::errorHere(const std::string& msg,
                               const std::string& expected,
                               const std::string& got) {
    ParseError e;
    e.line    = peek().line;
    e.column  = peek().column;
    e.message = msg;
    e.got     = got;
    e.expected = expected;
    errors_.push_back(std::move(e));
    return peek();
}

void Parser::synchronize() {
    // Skip tokens until we find something that looks like the start of a
    // new statement: ';' or '}' or any statement-leading identifier.
    while (!isAtEnd()) {
        if (check(TokenType::SEMI)) { advance(); return; }
        if (check(TokenType::RBRACE)) { advance(); return; }
        if (check(TokenType::IDENTIFIER)) return;
        advance();
    }
}

// ----- Program ----------------------------------------------------------------

std::vector<StmtPtr> Parser::parseProgram() {
    std::vector<StmtPtr> stmts;
    while (!isAtEnd()) {
        // Discard stray closing braces at the top level (unmatched '}' from a
        // truncated/malformed program). Without this we'd loop forever since
        // parseStatement cannot make progress on a lone '}'.
        if (check(TokenType::RBRACE)) {
            errorHere("অপ্রত্যাশিত '}' — এটি কোনো খোলা '{' এর সাথে মেলে না।",
                      "'সংখ্যা', 'লেখা', 'যদি', 'যতক্ষণ', 'দেখাও' অথবা ';'",
                      peek().lexeme);
            advance();
            continue;
        }
        if (auto s = parseStatement()) {
            stmts.push_back(std::move(s));
        } else if (hadErrors()) {
            // parseStatement failed but didn't already synchronize; do it now.
            synchronize();
        }
    }
    return stmts;
}

// ----- Statements -------------------------------------------------------------

namespace {

// Map of lexeme text -> keyword semantic role. The lexer intentionally
// returns IDENTIFIER for these names so that a user can name a variable
// "সংখ্যা" if they wish; the parser pattern-matches here.
bool isTypeLexeme(const std::string& s, Type& out) {
    if (s == "সংখ্যা") { out = Type::NUMBER; return true; }
    if (s == "লেখা")   { out = Type::TEXT;   return true; }
    return false;
}
bool isPrintLexeme(const std::string& s)   { return s == "দেখাও"; }
bool isIfLexeme(const std::string& s)      { return s == "যদি"; }
bool isWhileLexeme(const std::string& s)   { return s == "যতক্ষণ"; }
bool isElseLexeme(const std::string& s)    { return s == "নাহলে"; }

} // namespace

StmtPtr Parser::parseStatement() {
    if (check(TokenType::IDENTIFIER)) {
        Type t;
        if (isTypeLexeme(peek().lexeme, t))   return parseDeclaration();
        if (isPrintLexeme(peek().lexeme))     return parsePrint();
        if (isIfLexeme(peek().lexeme))        return parseIf();
        if (isWhileLexeme(peek().lexeme))     return parseWhile();
        if (isElseLexeme(peek().lexeme)) {
            errorHere("'নাহলে' এখানে আশা করা হয়নি — এটি একটি 'যদি' বিবৃতির পরে আসে।",
                      "'সংখ্যা', 'লেখা', 'যদি', 'যতক্ষণ', 'দেখাও', ';' অথবা '}'",
                      peek().lexeme);
            synchronize();
            return nullptr;
        }
        return parseAssignmentOrExpression();
    }
    switch (peek().type) {
        case TokenType::LBRACE:
            // Bare block (useful for nested scopes in the future).
            return parseBlock();
        default:
            return parseAssignmentOrExpression();
    }
}

StmtPtr Parser::parseDeclaration() {
    Type declaredType = Type::UNKNOWN;
    {
        Type t;
        if (isTypeLexeme(peek().lexeme, t)) {
            declaredType = t;
            advance();
        }
    }

    if (!check(TokenType::IDENTIFIER)) {
        errorHere("ঘোষণার পরে একটি ভেরিয়েবলের নাম প্রয়োজন।",
                  "একটি শনাক্তকারী (identifier)", peek().lexeme);
        synchronize();
        return nullptr;
    }
    Token nameTok = advance();

    if (!match(TokenType::ASSIGN)) {
        errorHere("'=' প্রত্যাশিত ছিল। ঘোষণায় '=' ব্যবহার করুন।",
                  "'='", peek().lexeme);
        // Try to continue: skip until ';' or '}'.
        synchronize();
        return nullptr;
    }

    ExprPtr init = parseExpression();

    if (!match(TokenType::SEMI)) {
        errorHere("প্রতিটি ঘোষণা ';' দিয়ে শেষ করুন।",
                  "';'", peek().lexeme);
        // Don't sync here: ';' may legitimately be missing. Try to recover
        // by skipping to the next semicolon or closing brace at outer level.
        synchronize();
    }

    auto s = std::make_unique<VarDeclStmt>();
    s->type = declaredType;
    s->name = nameTok.lexeme;
    s->initializer = std::move(init);
    s->line = nameTok.line;
    return s;
}

StmtPtr Parser::parseAssignmentOrExpression() {
    // Statement-context expression: must start with identifier, '(', number,
    // string, or unary '-'.
    if (!(check(TokenType::IDENTIFIER) ||
          check(TokenType::NUMBER) ||
          check(TokenType::STRING) ||
          check(TokenType::LPAREN) ||
          check(TokenType::MINUS))) {
        errorHere("একটি বিবৃতি প্রত্যাশিত ছিল।",
                  "'সংখ্যা', 'লেখা', 'যদি', 'যতক্ষণ', 'দেখাও', ';' অথবা '}'",
                  peek().lexeme);
        synchronize();
        return nullptr;
    }
    // Save position in case this is actually not an assignment.
    size_t saved = pos_;
    const Token& first = peek();

    ExprPtr expr = parseExpression();

    if (check(TokenType::ASSIGN)) {
        // Must be a bare identifier on the LHS.
        if (first.type != TokenType::IDENTIFIER) {
            // Re-run from scratch: parseExpression consumed extra tokens.
            // This is a rare corner case; produce a clean error.
            errorHere("বাম পাশে শুধু একটি ভেরিয়েবলের নাম থাকতে পারে।",
                      "ভেরিয়েবলের নাম", first.lexeme);
            advance(); // consume '='
            parseExpression();
            if (!match(TokenType::SEMI)) synchronize();
            return nullptr;
        }
        advance(); // consume '='
        ExprPtr value = parseExpression();
        if (!match(TokenType::SEMI)) {
            errorHere("প্রতিটি বিবৃতি ';' দিয়ে শেষ করুন।",
                      "';'", peek().lexeme);
            synchronize();
        }
        auto s = std::make_unique<AssignStmt>();
        s->name = first.lexeme;
        s->value = std::move(value);
        s->line = first.line;
        return s;
    }

    // Not an assignment: must be a bare expression statement, which we don't
    // otherwise support. Report error but be tolerant.
    (void)saved;
    errorHere("'" + first.lexeme + "' এখানে একটি বিবৃতি হিসেবে ব্যবহার করা যায় না। " +
              "হয়তো আপনি 'দেখাও(...)' অথবা একটি নতুন ঘোষণা বা বরাদ্দ বোঝাতে চেয়েছিলেন?",
              "একটি সম্পূর্ণ বিবৃতি", first.lexeme);
    if (!match(TokenType::SEMI)) synchronize();
    return nullptr;
}

StmtPtr Parser::parsePrint() {
    const Token& kw = advance(); // consume 'দেখাও'
    if (!match(TokenType::LPAREN)) {
        errorHere("'দেখাও' এর পরে '(' প্রয়োজন।",
                  "'('", peek().lexeme);
        synchronize();
        return nullptr;
    }
    ExprPtr value = parseExpression();
    if (!match(TokenType::RPAREN)) {
        errorHere("')' প্রয়োজন।",
                  "')'", peek().lexeme);
        synchronize();
        return nullptr;
    }
    if (!match(TokenType::SEMI)) {
        errorHere("প্রতিটি বিবৃতি ';' দিয়ে শেষ করুন।",
                  "';'", peek().lexeme);
        synchronize();
    }
    auto s = std::make_unique<PrintStmt>();
    s->value = std::move(value);
    s->line = kw.line;
    return s;
}

StmtPtr Parser::parseIf() {
    const Token& kw = advance(); // 'যদি'
    if (!match(TokenType::LPAREN)) {
        errorHere("'যদি' এর পরে '(' প্রয়োজন।",
                  "'('", peek().lexeme);
        synchronize();
        return nullptr;
    }
    ExprPtr cond = parseExpression();
    if (!match(TokenType::RPAREN)) {
        errorHere("')' প্রয়োজন।",
                  "')'", peek().lexeme);
        synchronize();
        return nullptr;
    }
    StmtPtr thenBody;
    if (check(TokenType::LBRACE)) {
        advance();
        thenBody = parseBlock();
    } else {
        errorHere("'যদি' এর পরে একটি '{' ব্লক প্রয়োজন।",
                  "'{'", peek().lexeme);
        synchronize();
        return nullptr;
    }

    StmtPtr elseBody = nullptr;
    if (check(TokenType::IDENTIFIER) && isElseLexeme(peek().lexeme)) {
        advance();
        if (check(TokenType::LBRACE)) {
            advance();
            elseBody = parseBlock();
        } else if (check(TokenType::IDENTIFIER) && isIfLexeme(peek().lexeme)) {
            elseBody = parseIf(); // else if
        } else {
            errorHere("'নাহলে' এর পরে '{' অথবা 'যদি' প্রয়োজন।",
                      "'{' অথবা 'যদি'", peek().lexeme);
            synchronize();
            return nullptr;
        }
    }
    auto s = std::make_unique<IfStmt>();
    s->condition = std::move(cond);
    s->thenBranch = std::move(thenBody);
    s->elseBranch = std::move(elseBody);
    s->line = kw.line;
    return s;
}

StmtPtr Parser::parseWhile() {
    const Token& kw = advance(); // 'যতক্ষণ'
    if (!match(TokenType::LPAREN)) {
        errorHere("'যতক্ষণ' এর পরে '(' প্রয়োজন।",
                  "'('", peek().lexeme);
        synchronize();
        return nullptr;
    }
    ExprPtr cond = parseExpression();
    if (!match(TokenType::RPAREN)) {
        errorHere("')' প্রয়োজন।",
                  "')'", peek().lexeme);
        synchronize();
        return nullptr;
    }
    StmtPtr body;
    if (check(TokenType::LBRACE)) {
        advance();
        body = parseBlock();
    } else {
        errorHere("'যতক্ষণ' এর পরে একটি '{' ব্লক প্রয়োজন।",
                  "'{'", peek().lexeme);
        synchronize();
        return nullptr;
    }
    auto s = std::make_unique<WhileStmt>();
    s->condition = std::move(cond);
    s->body = std::move(body);
    s->line = kw.line;
    return s;
}

StmtPtr Parser::parseBlock() {
    auto block = std::make_unique<BlockStmt>();
    while (!isAtEnd() && !check(TokenType::RBRACE)) {
        if (auto s = parseStatement()) {
            block->statements.push_back(std::move(s));
        }
    }
    if (!match(TokenType::RBRACE)) {
        errorHere("'}' প্রয়োজন — ব্লক এখানো শেষ হয়নি।",
                  "'}'", peek().lexeme);
        // Don't recurse synchronize() — caller will continue at outer level.
    }
    return block;
}

// ----- Expressions ------------------------------------------------------------

ExprPtr Parser::parseExpression() { return parseComparison(); }

ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddition();
    while (true) {
        BinaryExpr::Op op;
        if      (check(TokenType::GT))  op = BinaryExpr::Op::GT;
        else if (check(TokenType::LT))  op = BinaryExpr::Op::LT;
        else if (check(TokenType::GTE)) op = BinaryExpr::Op::GTE;
        else if (check(TokenType::LTE)) op = BinaryExpr::Op::LTE;
        else if (check(TokenType::EQEQ))op = BinaryExpr::Op::EQEQ;
        else if (check(TokenType::NEQ)) op = BinaryExpr::Op::NEQ;
        else break;
        advance();
        ExprPtr right = parseAddition();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseAddition() {
    ExprPtr left = parseMultiplication();
    while (true) {
        BinaryExpr::Op op;
        if      (check(TokenType::PLUS))  op = BinaryExpr::Op::ADD;
        else if (check(TokenType::MINUS)) op = BinaryExpr::Op::SUB;
        else break;
        advance();
        ExprPtr right = parseMultiplication();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseMultiplication() {
    ExprPtr left = parseUnary();
    while (true) {
        BinaryExpr::Op op;
        if      (check(TokenType::STAR))  op = BinaryExpr::Op::MUL;
        else if (check(TokenType::SLASH)) op = BinaryExpr::Op::DIV;
        else break;
        advance();
        ExprPtr right = parseUnary();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::MINUS)) {
        advance();
        ExprPtr operand = parseUnary();
        return makeUnary(UnaryExpr::Op::NEG, std::move(operand));
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    const Token& t = peek();
    if (match(TokenType::NUMBER)) {
        return LiteralExpr::makeNumber(t.numValue);
    }
    if (match(TokenType::STRING)) {
        return LiteralExpr::makeString(t.strValue);
    }
    if (match(TokenType::IDENTIFIER)) {
        return makeVariable(t.lexeme);
    }
    if (match(TokenType::LPAREN)) {
        ExprPtr inner = parseExpression();
        if (!match(TokenType::RPAREN)) {
            errorHere("')' প্রয়োজন।",
                      "')'", peek().lexeme);
        }
        return inner;
    }
    errorHere("একটি মান (সংখ্যা, লেখা অথবা ভেরিয়েবল) প্রত্যাশিত ছিল।",
              "একটি মান অথবা '('", peek().lexeme);
    return LiteralExpr::makeNumber(0);
}

} // namespace bhasha
