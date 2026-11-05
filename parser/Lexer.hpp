#ifndef LEXER_HPP
#define LEXER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

enum class TokenType {
    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,
    Semicolon,
    Bang,
    Slash,
    Backslash,
    Pipe,
    Dot,
    Plus,
    Equals,
    NumberLit,
    StringLit,
    ColorLit,
    TimeLit,
    ResolutionLit,
    VarRef,
    Identifier,
    KwCreate,
    KwMain,
    KwFunc,
    KwLoad,
    KwTake,
    KwTrue,
    KwFalse,
    KwCenter,
    KwPass,
    KwStart,
    KwEnd,
    KwCross,
    AtText,
    AtShape,
    AtMask,
    AtCamera,
    EndOfFile
};

inline const char* tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::Comma: return "Comma";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Bang: return "Bang";
        case TokenType::Slash: return "Slash";
        case TokenType::Backslash: return "Backslash";
        case TokenType::Pipe: return "Pipe";
        case TokenType::Dot: return "Dot";
        case TokenType::Plus: return "Plus";
        case TokenType::Equals: return "Equals";
        case TokenType::NumberLit: return "NumberLit";
        case TokenType::StringLit: return "StringLit";
        case TokenType::ColorLit: return "ColorLit";
        case TokenType::TimeLit: return "TimeLit";
        case TokenType::ResolutionLit: return "ResolutionLit";
        case TokenType::VarRef: return "VarRef";
        case TokenType::Identifier: return "Identifier";
        case TokenType::KwCreate: return "KwCreate";
        case TokenType::KwMain: return "KwMain";
        case TokenType::KwFunc: return "KwFunc";
        case TokenType::KwLoad: return "KwLoad";
        case TokenType::KwTake: return "KwTake";
        case TokenType::KwTrue: return "KwTrue";
        case TokenType::KwFalse: return "KwFalse";
        case TokenType::KwCenter: return "KwCenter";
        case TokenType::KwPass: return "KwPass";
        case TokenType::KwStart: return "KwStart";
        case TokenType::KwEnd: return "KwEnd";
        case TokenType::KwCross: return "KwCross";
        case TokenType::AtText: return "AtText";
        case TokenType::AtShape: return "AtShape";
        case TokenType::AtMask: return "AtMask";
        case TokenType::AtCamera: return "AtCamera";
        case TokenType::EndOfFile: return "EndOfFile";
    }
    return "Unknown";
}

struct Token {
    TokenType type = TokenType::EndOfFile;
    std::string text;
    int line = 1;
    int column = 1;
    double number = 0.0;
    std::int64_t centiseconds = 0;
    int width = 0;
    int height = 0;
};

class LexError : public std::runtime_error {
public:
    LexError(const std::string& message, int line, int column)
        : std::runtime_error("line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + message),
          line_(line),
          column_(column) {}

    int line() const { return line_; }
    int column() const { return column_; }

private:
    int line_;
    int column_;
};

class Lexer {
public:
    explicit Lexer(std::string source) : src_(std::move(source)) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        for (;;) {
            skipWhitespace();
            if (atEnd()) break;
            tokens.push_back(nextToken());
        }
        Token eof;
        eof.type = TokenType::EndOfFile;
        eof.line = line_;
        eof.column = col_;
        tokens.push_back(eof);
        return tokens;
    }

private:
    std::string src_;
    std::size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    static bool isDigit(char c) { return c >= '0' && c <= '9'; }
    static bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
    static bool isAlnum(char c) { return isAlpha(c) || isDigit(c); }
    static bool isHex(char c) { return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

    static const std::unordered_map<std::string, TokenType>& keywords() {
        static const std::unordered_map<std::string, TokenType> table = {
            {"Create", TokenType::KwCreate},
            {"main", TokenType::KwMain},
            {"func", TokenType::KwFunc},
            {"load", TokenType::KwLoad},
            {"take", TokenType::KwTake},
            {"true", TokenType::KwTrue},
            {"false", TokenType::KwFalse},
            {"center", TokenType::KwCenter},
            {"pass", TokenType::KwPass},
            {"start", TokenType::KwStart},
            {"end", TokenType::KwEnd},
            {"x", TokenType::KwCross}};
        return table;
    }

    static const std::unordered_map<std::string, TokenType>& generators() {
        static const std::unordered_map<std::string, TokenType> table = {
            {"text", TokenType::AtText},
            {"shape", TokenType::AtShape},
            {"mask", TokenType::AtMask},
            {"camera", TokenType::AtCamera}};
        return table;
    }

    bool atEnd() const { return pos_ >= src_.size(); }

    char peek(std::size_t offset = 0) const {
        return pos_ + offset < src_.size() ? src_[pos_ + offset] : '\0';
    }

    char advance() {
        char c = src_[pos_++];
        if (c == '\n') {
            ++line_;
            col_ = 1;
        } else {
            ++col_;
        }
        return c;
    }

    [[noreturn]] void fail(const std::string& message, int line, int column) const {
        throw LexError(message, line, column);
    }

    static Token make(TokenType type, std::string text, int line, int column) {
        Token token;
        token.type = type;
        token.text = std::move(text);
        token.line = line;
        token.column = column;
        return token;
    }

    Token single(TokenType type, int line, int column) {
        char c = advance();
        return make(type, std::string(1, c), line, column);
    }

    void skipWhitespace() {
        while (!atEnd()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else {
                break;
            }
        }
    }

    void skipRestOfLine() {
        while (!atEnd() && peek() != '\n') advance();
    }

    std::int64_t parseInteger(std::size_t begin, std::size_t end, int line, int column) const {
        if (end - begin > 9) fail("number is too large", line, column);
        return std::stoll(src_.substr(begin, end - begin));
    }

    Token nextToken() {
        const int line = line_;
        const int column = col_;
        const char c = peek();

        if (isDigit(c) || (c == '-' && isDigit(peek(1)))) return lexNumber(line, column);
        if (isAlpha(c)) return lexWord(line, column);

        switch (c) {
            case '$': return lexVarRef(line, column);
            case '@': return lexGenerator(line, column);
            case '#': return lexColor(line, column);
            case '"': return lexString(line, column);
            case '!': {
                advance();
                skipRestOfLine();
                return make(TokenType::Bang, "!", line, column);
            }
            case '(': return single(TokenType::LParen, line, column);
            case ')': return single(TokenType::RParen, line, column);
            case '{': return single(TokenType::LBrace, line, column);
            case '}': return single(TokenType::RBrace, line, column);
            case ',': return single(TokenType::Comma, line, column);
            case ';': return single(TokenType::Semicolon, line, column);
            case '/': return single(TokenType::Slash, line, column);
            case '\\': return single(TokenType::Backslash, line, column);
            case '|': return single(TokenType::Pipe, line, column);
            case '.': return single(TokenType::Dot, line, column);
            case '+': return single(TokenType::Plus, line, column);
            case '=': return single(TokenType::Equals, line, column);
            default: break;
        }

        fail(std::string("unexpected character '") + c + "'", line, column);
    }

    Token lexWord(int line, int column) {
        const std::size_t start = pos_;
        while (isAlnum(peek())) advance();
        std::string word = src_.substr(start, pos_ - start);
        auto it = keywords().find(word);
        TokenType type = it != keywords().end() ? it->second : TokenType::Identifier;
        return make(type, std::move(word), line, column);
    }

    Token lexVarRef(int line, int column) {
        advance();
        if (!isAlpha(peek())) fail("expected a name after '$'", line, column);
        const std::size_t start = pos_;
        while (isAlnum(peek())) advance();
        return make(TokenType::VarRef, src_.substr(start, pos_ - start), line, column);
    }

    Token lexGenerator(int line, int column) {
        advance();
        const std::size_t start = pos_;
        while (isAlnum(peek())) advance();
        std::string name = src_.substr(start, pos_ - start);
        auto it = generators().find(name);
        if (it == generators().end()) fail("unknown generator '@" + name + "'", line, column);
        return make(it->second, "@" + name, line, column);
    }

    Token lexColor(int line, int column) {
        advance();
        const std::size_t start = pos_;
        while (isHex(peek())) advance();
        const std::size_t length = pos_ - start;
        if (length != 6 && length != 8) fail("color must have 6 or 8 hex digits", line, column);
        if (isAlnum(peek())) fail("invalid color", line, column);
        return make(TokenType::ColorLit, "#" + src_.substr(start, length), line, column);
    }

    Token lexString(int line, int column) {
        advance();
        std::string value;
        for (;;) {
            if (atEnd() || peek() == '\n') fail("unterminated string", line, column);
            char c = advance();
            if (c == '"') break;
            if (c != '\\') {
                value += c;
                continue;
            }
            if (atEnd()) fail("unterminated string", line, column);
            const int escapeLine = line_;
            const int escapeColumn = col_ - 1;
            char e = advance();
            switch (e) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default: fail("invalid escape sequence", escapeLine, escapeColumn);
            }
        }
        return make(TokenType::StringLit, std::move(value), line, column);
    }

    Token lexNumber(int line, int column) {
        const std::size_t start = pos_;
        const bool negative = peek() == '-';
        if (negative) advance();
        while (isDigit(peek())) advance();

        if (peek() == ':') {
            if (negative) fail("timestamp cannot be negative", line, column);
            return lexTimestamp(start, line, column);
        }
        if (!negative && peek() == 'x' && isDigit(peek(1))) return lexResolution(start, line, column);

        if (peek() == '.' && isDigit(peek(1))) {
            advance();
            while (isDigit(peek())) advance();
        }
        if (isAlpha(peek())) fail("invalid number", line, column);

        std::string text = src_.substr(start, pos_ - start);
        Token token = make(TokenType::NumberLit, text, line, column);
        token.number = std::stod(text);
        return token;
    }

    std::int64_t readTwoDigits(int line, int column) {
        if (!isDigit(peek()) || !isDigit(peek(1))) fail("expected two digits in timestamp", line, column);
        std::int64_t value = (peek() - '0') * 10 + (peek(1) - '0');
        advance();
        advance();
        return value;
    }

    Token lexTimestamp(std::size_t start, int line, int column) {
        const std::int64_t minutes = parseInteger(start, pos_, line, column);
        advance();
        const std::int64_t seconds = readTwoDigits(line, column);
        std::int64_t fraction = 0;
        if (peek() == '.') {
            advance();
            fraction = readTwoDigits(line, column);
        }
        if (isAlnum(peek())) fail("invalid timestamp", line, column);
        if (seconds >= 60) fail("seconds must be below 60", line, column);
        Token token = make(TokenType::TimeLit, src_.substr(start, pos_ - start), line, column);
        token.centiseconds = (minutes * 60 + seconds) * 100 + fraction;
        return token;
    }

    Token lexResolution(std::size_t start, int line, int column) {
        const std::int64_t width = parseInteger(start, pos_, line, column);
        advance();
        const std::size_t heightStart = pos_;
        while (isDigit(peek())) advance();
        const std::int64_t height = parseInteger(heightStart, pos_, line, column);
        if (isAlnum(peek())) fail("invalid resolution", line, column);
        Token token = make(TokenType::ResolutionLit, src_.substr(start, pos_ - start), line, column);
        token.width = static_cast<int>(width);
        token.height = static_cast<int>(height);
        return token;
    }
};

#endif
