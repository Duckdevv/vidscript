#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "AST.hpp"
#include "Lexer.hpp"

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, int line, int column);

    int line() const { return line_; }
    int column() const { return column_; }

private:
    int line_;
    int column_;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    ProgramAST parse();

private:
    struct Value {
        bool is_vector = false;
        Operand operand;
        VectorValue vector;
    };

    std::vector<Token> tokens_;
    std::size_t pos_ = 0;

    const Token& peek(std::size_t offset = 0) const;
    const Token& advance();
    bool check(TokenType type, std::size_t offset = 0) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& what);
    void skipBangs();
    [[noreturn]] void fail(const std::string& message, const Token& at) const;
    static std::string describe(const Token& token);
    static Timestamp timestampFrom(const Token& token);

    CreateStmt parseCreate();
    VariableDecl parseDeclaration();
    MaskBinding parseMaskBinding();
    MainBlock parseMain();
    FunctionDecl parseFunction();
    AssetBlock parseAssetBlock();
    std::vector<ActionItem> parseActionList();
    ActionItem parseActionItem();
    Ramp parseRamp();

    AssetExpr parseAssetExpr();
    LoadExpr parseLoad();
    TakeExpr parseTake();
    ClipBound parseClipBound();
    TextExpr parseText();
    ShapeExpr parseShape();
    MaskExpr parseMask();
    CameraExpr parseCamera();

    Operand parseOperand();
    Operand parseTimeOperand();
    Value parseValue();
    VectorValue parseVectorArg();
};

#endif
