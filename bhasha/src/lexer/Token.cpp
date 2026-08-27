#include "Token.h"

#include <unordered_map>

namespace bhasha {

std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::STRING:      return "STRING";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::KW_SONGKHA:  return "'সংখ্যা'";
        case TokenType::KW_LEKHA:    return "'লেখা'";
        case TokenType::KW_YADI:     return "'যদি'";
        case TokenType::KW_NAHOLE:   return "'নাহলে'";
        case TokenType::KW_ZOTOKKHON:return "'যতক্ষণ'";
        case TokenType::KW_DEKHAO:   return "'দেখাও'";
        case TokenType::PLUS:        return "'+'";
        case TokenType::MINUS:       return "'-'";
        case TokenType::STAR:        return "'*'";
        case TokenType::SLASH:       return "'/'";
        case TokenType::GT:          return "'>'";
        case TokenType::LT:          return "'<'";
        case TokenType::GTE:         return "'>='";
        case TokenType::LTE:         return "'<='";
        case TokenType::EQEQ:        return "'=='";
        case TokenType::NEQ:         return "'!='";
        case TokenType::ASSIGN:      return "'='";
        case TokenType::LPAREN:      return "'('";
        case TokenType::RPAREN:      return "')'";
        case TokenType::LBRACE:      return "'{'";
        case TokenType::RBRACE:      return "'}'";
        case TokenType::SEMI:        return "';'";
        case TokenType::COMMA:       return "','";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR:       return "ERROR";
    }
    return "?";
}

} // namespace bhasha
