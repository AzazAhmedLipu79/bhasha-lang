#pragma once

#include <string>

namespace bhasha {

// Token kinds for the Bhasha (Bangla-first) educational language.
enum class TokenType {
    // Literals
    NUMBER,        // 10  or  ১০
    STRING,        // "..."
    IDENTIFIER,    // বয়স, x

    // Keywords (Bangla)
    KW_SONGKHA,    // সংখ্যা   (number)
    KW_LEKHA,      // লেখা     (text)
    KW_YADI,       // যদি      (if)
    KW_NAHOLE,     // নাহলে    (else)
    KW_ZOTOKKHON,  // যতক্ষণ    (while / as long as)
    KW_DEKHAO,     // দেখাও    (show / print)

    // Operators
    PLUS,   // +
    MINUS,  // -
    STAR,   // *
    SLASH,  // /
    GT,     // >
    LT,     // <
    GTE,    // >=
    LTE,    // <=
    EQEQ,   // ==
    NEQ,    // !=

    // Punctuation
    ASSIGN,   // =
    LPAREN,   // (
    RPAREN,   // )
    LBRACE,   // {
    RBRACE,   // }
    SEMI,     // ;
    COMMA,    // ,

    // Special
    END_OF_FILE,
    ERROR,
};

// Returns a human-friendly name for the token kind (used in error messages).
std::string tokenTypeName(TokenType t);

} // namespace bhasha
