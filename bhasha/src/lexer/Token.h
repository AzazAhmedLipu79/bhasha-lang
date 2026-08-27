#pragma once

#include "TokenType.h"

#include <string>

namespace bhasha {

// A single lexical token.
// Tracks its kind, lexeme, source position, and (for literals) the parsed value.
struct Token {
    TokenType type{TokenType::ERROR};
    std::string lexeme;
    int line{1};
    int column{1};

    // For literals. value holds:
    //   NUMBER -> int64_t value (stored in numValue)
    //   STRING -> decoded UTF-8 string (stored in strValue)
    long long numValue{0};
    std::string strValue;

    Token() = default;
    Token(TokenType t, std::string lex, int ln, int col)
        : type(t), lexeme(std::move(lex)), line(ln), column(col) {}
};

} // namespace bhasha
