#pragma once

#include "Token.h"

#include <string>
#include <vector>

namespace bhasha {

// Reports issues encountered during lexical analysis.
struct LexError {
    int line{1};
    int column{1};
    std::string message;
};

// Lexer: turns UTF-8 source text into a flat stream of tokens.
class Lexer {
public:
    explicit Lexer(std::string source);

    // Returns the full token stream (always ends with END_OF_FILE).
    // Lexical errors are collected separately via getErrors().
    std::vector<Token> tokenize();

    const std::vector<LexError>& getErrors() const { return errors_; }

private:
    std::string source_;
    size_t pos_{0};     // byte offset into source_
    int line_{1};
    int col_{1};        // column in *code points*, not bytes
    std::vector<LexError> errors_;

    // Returns true if more source remains.
    bool hasMore() const { return pos_ < source_.size(); }

    // Reads the next UTF-8 code point (returns -1 at EOF) and advances.
    // Updates line/col tracking.
    int nextCodepoint();

    // Peeks the next code point without consuming it.
    int peekCodepoint() const;

    // Skips whitespace and # line comments.
    void skipWhitespaceAndComments();

    // Tries to lex a number (ASCII 0-9 or Bangla ০-৯).
    Token lexNumber(int firstCp, int startLine, int startCol);

    // Tries to lex a string literal "...".
    Token lexString(int startLine, int startCol);

    // Tries to lex an identifier or Bangla keyword.
    // Always emits IDENTIFIER; the parser pattern-matches against keyword lexemes
    // in the appropriate syntactic positions. This keeps variable names that
    // happen to share text with a keyword working naturally.
    Token lexIdentifierOrKeyword(int firstCp, int startLine, int startCol);
};

} // namespace bhasha
