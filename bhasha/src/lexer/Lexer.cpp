#include "Lexer.h"

#include <cstdint>
#include <unordered_map>

namespace bhasha {

namespace {

// Bangla digit code points U+09E6 .. U+09EF map to ASCII '0'..'9'.
bool isBanglaDigit(int cp) {
    return cp >= 0x09E6 && cp <= 0x09EF;
}
int banglaDigitValue(int cp) { return cp - 0x09E6; }

// A "letter" for identifiers: ASCII letters or any Bangla letter
// (U+0980..U+09FF excluding digits 09E6..09EF which we treat as digits).
bool isIdentStart(int cp) {
    if (cp == '_') return true;
    if (cp >= 'A' && cp <= 'Z') return true;
    if (cp >= 'a' && cp <= 'z') return true;
    // Bangla block letters (exclude digits range and vowel signs etc.)
    if (cp >= 0x0981 && cp <= 0x09CD) return true; // various Bangla signs/letters
    if (cp >= 0x09F0 && cp <= 0x09FF) return true; // rarer Bangla signs
    return false;
}

bool isIdentPart(int cp) {
    if (isIdentStart(cp)) return true;
    if (cp >= '0' && cp <= '9') return true;
    if (isBanglaDigit(cp)) return true;
    // Allow Bangla vowel signs / virama as parts of identifiers.
    if (cp >= 0x09BC && cp <= 0x09CD) return true;
    return false;
}

bool isWhitespace(int cp) {
    return cp == ' ' || cp == '\t' || cp == '\r' || cp == '\n' || cp == 0x0B || cp == 0x0C;
}

} // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

int Lexer::nextCodepoint() {
    if (pos_ >= source_.size()) return -1;
    unsigned char c = static_cast<unsigned char>(source_[pos_]);

    int cp;
    int bytes;
    if      ((c & 0x80) == 0x00) { cp = c;            bytes = 1; }
    else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F;     bytes = 2; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F;     bytes = 3; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07;     bytes = 4; }
    else {
        // Invalid leading byte; treat as 1-byte and advance to avoid loop.
        ++pos_;
        ++col_;
        return 0xFFFD;
    }
    for (int i = 1; i < bytes; ++i) {
        if (pos_ + i >= source_.size()) { cp = 0xFFFD; bytes = i + 1; break; }
        cp = (cp << 6) | (static_cast<unsigned char>(source_[pos_ + i]) & 0x3F);
    }

    pos_ += bytes;
    if (cp == '\n') { ++line_; col_ = 1; }
    else            { ++col_; }
    return cp;
}

int Lexer::peekCodepoint() const {
    if (pos_ >= source_.size()) return -1;
    unsigned char c = static_cast<unsigned char>(source_[pos_]);
    int cp;
    int bytes;
    if      ((c & 0x80) == 0x00) { cp = c;        bytes = 1; }
    else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; bytes = 2; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; bytes = 3; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; bytes = 4; }
    else return 0xFFFD;
    for (int i = 1; i < bytes; ++i) {
        if (pos_ + i >= source_.size()) return 0xFFFD;
        cp = (cp << 6) | (static_cast<unsigned char>(source_[pos_ + i]) & 0x3F);
    }
    return cp;
}

void Lexer::skipWhitespaceAndComments() {
    while (hasMore()) {
        int cp = peekCodepoint();
        if (cp == -1) return;
        if (isWhitespace(cp)) {
            nextCodepoint();
            continue;
        }
        if (cp == '#') {
            // Comment to end of line.
            nextCodepoint();
            while (hasMore()) {
                int c = peekCodepoint();
                if (c == -1 || c == '\n') break;
                nextCodepoint();
            }
            continue;
        }
        return;
    }
}

Token Lexer::lexNumber(int firstCp, int startLine, int startCol) {
    std::string raw;
    long long value = 0;
    auto appendDigit = [&](int digit) {
        raw.push_back('0' + digit);
        value = value * 10 + digit;
    };
    if (firstCp >= '0' && firstCp <= '9')      appendDigit(firstCp - '0');
    else if (isBanglaDigit(firstCp))           appendDigit(banglaDigitValue(firstCp));

    while (hasMore()) {
        int cp = peekCodepoint();
        if (cp >= '0' && cp <= '9') { nextCodepoint(); appendDigit(cp - '0'); }
        else if (isBanglaDigit(cp)) { nextCodepoint(); appendDigit(banglaDigitValue(cp)); }
        else break;
    }
    Token t(TokenType::NUMBER, raw, startLine, startCol);
    t.numValue = value;
    t.strValue = raw;
    return t;
}

Token Lexer::lexString(int startLine, int startCol) {
    // Consume opening quote (already peeked).
    nextCodepoint();
    std::string out;
    bool closed = false;
    while (hasMore()) {
        int cp = peekCodepoint();
        if (cp == -1) break;
        if (cp == '"') { nextCodepoint(); closed = true; break; }
        if (cp == '\n') {
            // Strings cannot span lines in this toy language.
            break;
        }
        if (cp == '\\') {
            nextCodepoint();
            int esc = peekCodepoint();
            if (esc == -1) break;
            nextCodepoint();
            switch (esc) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case 'r': out.push_back('\r'); break;
                case '\\': out.push_back('\\'); break;
                case '"': out.push_back('"'); break;
                default:  out.push_back('\\'); out.push_back(static_cast<char>(esc)); break;
            }
            continue;
        }
        // Append the raw UTF-8 bytes for this codepoint.
        size_t startByte = pos_;
        nextCodepoint();
        out.append(source_, startByte, pos_ - startByte);
    }
    if (!closed) {
        errors_.push_back({startLine, startCol,
            "অসমাপ্ত স্ট্রিং লিটারেল: বন্ধ করতে '\"' প্রয়োজন।"});
    }
    Token t(TokenType::STRING, out, startLine, startCol);
    t.strValue = out;
    return t;
}

Token Lexer::lexIdentifierOrKeyword(int firstCp, int startLine, int startCol) {
    std::string raw;
    auto appendBytes = [&](int cp) {
        // Reconstruct UTF-8 bytes from a code point.
        if (cp < 0x80) {
            raw.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            raw.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            raw.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            raw.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            raw.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            raw.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            raw.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            raw.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            raw.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            raw.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    };
    appendBytes(firstCp);

    while (hasMore()) {
        int cp = peekCodepoint();
        if (cp == -1 || !isIdentPart(cp)) break;
        nextCodepoint();
        appendBytes(cp);
    }

    // Emit IDENTIFIER for every name. The parser matches keyword lexemes in
    // statement-initial contexts (e.g. "সংখ্যা <ident> = ...").
    (void)0;
    return Token(TokenType::IDENTIFIER, raw, startLine, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        if (!hasMore()) {
            tokens.emplace_back(TokenType::END_OF_FILE, "", line_, col_);
            break;
        }
        int startLine = line_;
        int startCol = col_;
        int cp = peekCodepoint();

        // Number literal.
        if ((cp >= '0' && cp <= '9') || isBanglaDigit(cp)) {
            nextCodepoint();
            tokens.push_back(lexNumber(cp, startLine, startCol));
            continue;
        }
        // String literal.
        if (cp == '"') {
            tokens.push_back(lexString(startLine, startCol));
            continue;
        }
        // Identifier or Bangla keyword.
        if (isIdentStart(cp)) {
            nextCodepoint();
            tokens.push_back(lexIdentifierOrKeyword(cp, startLine, startCol));
            continue;
        }
        // Operators and punctuation.
        nextCodepoint();
        switch (cp) {
            case '+': tokens.emplace_back(TokenType::PLUS, "+", startLine, startCol); break;
            case '-': tokens.emplace_back(TokenType::MINUS, "-", startLine, startCol); break;
            case '*': tokens.emplace_back(TokenType::STAR, "*", startLine, startCol); break;
            case '/': tokens.emplace_back(TokenType::SLASH, "/", startLine, startCol); break;
            case '(': tokens.emplace_back(TokenType::LPAREN, "(", startLine, startCol); break;
            case ')': tokens.emplace_back(TokenType::RPAREN, ")", startLine, startCol); break;
            case '{': tokens.emplace_back(TokenType::LBRACE, "{", startLine, startCol); break;
            case '}': tokens.emplace_back(TokenType::RBRACE, "}", startLine, startCol); break;
            case ';': tokens.emplace_back(TokenType::SEMI, ";", startLine, startCol); break;
            case ',': tokens.emplace_back(TokenType::COMMA, ",", startLine, startCol); break;
            case '>': {
                int nxt = peekCodepoint();
                if (nxt == '=') { nextCodepoint(); tokens.emplace_back(TokenType::GTE, ">=", startLine, startCol); }
                else            { tokens.emplace_back(TokenType::GT,  ">",  startLine, startCol); }
                break;
            }
            case '<': {
                int nxt = peekCodepoint();
                if (nxt == '=') { nextCodepoint(); tokens.emplace_back(TokenType::LTE, "<=", startLine, startCol); }
                else            { tokens.emplace_back(TokenType::LT,  "<",  startLine, startCol); }
                break;
            }
            case '!': {
                int nxt = peekCodepoint();
                if (nxt == '=') { nextCodepoint(); tokens.emplace_back(TokenType::NEQ, "!=", startLine, startCol); }
                else {
                    errors_.push_back({startLine, startCol,
                        std::string("অপ্রত্যাশিত অক্ষর '!')। হয়তো আপনি '!=' বোঝাতে চেয়েছিলেন?")});
                    tokens.emplace_back(TokenType::ERROR, "!", startLine, startCol);
                }
                break;
            }
            case '=': {
                int nxt = peekCodepoint();
                if (nxt == '=') { nextCodepoint(); tokens.emplace_back(TokenType::EQEQ, "==", startLine, startCol); }
                else            { tokens.emplace_back(TokenType::ASSIGN, "=", startLine, startCol); }
                break;
            }
            default: {
                std::string msg = "অপরিচিত অক্ষর '";
                // Append a printable form (ASCII char if printable).
                if (cp >= 0x20 && cp < 0x7F) msg.push_back(static_cast<char>(cp));
                else msg.push_back('?');
                msg += "'।";
                errors_.push_back({startLine, startCol, msg});
                tokens.emplace_back(TokenType::ERROR, "?", startLine, startCol);
                break;
            }
        }
    }
    return tokens;
}

} // namespace bhasha
