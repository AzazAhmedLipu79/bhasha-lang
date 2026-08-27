#pragma once

#include "../ast/AST.h"
#include "../lexer/Token.h"

#include <string>
#include <vector>

namespace bhasha {

// Reports issues encountered during parsing. The parser attempts to recover
// from these so it can report multiple errors per compilation.
struct ParseError {
    int line{1};
    int column{1};
    std::string message;
    std::string got;    // offending token text
    std::string expected; // human-readable expectation
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parse the token stream into a list of statements.
    // Returns an empty vector if the input was empty.
    std::vector<StmtPtr> parseProgram();

    const std::vector<ParseError>& getErrors() const { return errors_; }
    bool hadErrors() const { return !errors_.empty(); }

private:
    std::vector<Token> tokens_;
    size_t pos_{0};
    std::vector<ParseError> errors_;

    // ----- Helpers -------------------------------------------------------------
    const Token& peek() const { return tokens_[pos_]; }
    const Token& peekAt(size_t offset) const { return tokens_[pos_ + offset]; }
    const Token& previous() const { return tokens_[pos_ - 1]; }
    bool check(TokenType t) const { return peek().type == t; }
    bool match(TokenType t);             // consume if matches
    const Token& advance();              // consume current and return it
    bool isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }

    // Records an error and returns a dummy token (so caller can keep parsing).
    const Token& errorHere(const std::string& msg,
                           const std::string& expected,
                           const std::string& got);

    // Skips tokens until a likely-statement boundary. Used for error recovery.
    void synchronize();

    // ----- Grammar rules -------------------------------------------------------
    StmtPtr parseStatement();
    StmtPtr parseDeclaration();
    StmtPtr parseAssignmentOrExpression();
    StmtPtr parsePrint();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseBlock();   // assumes '{' has just been consumed

    ExprPtr parseExpression();
    ExprPtr parseComparison();
    ExprPtr parseAddition();
    ExprPtr parseMultiplication();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();
};

} // namespace bhasha
