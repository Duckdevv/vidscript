#include "Parser.hpp"

#include <utility>

ParseError::ParseError(const std::string& message, int line, int column)
    : std::runtime_error("line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + message),
      line_(line),
      column_(column) {}

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {
    if (tokens_.empty() || tokens_.back().type != TokenType::EndOfFile) {
        Token eof;
        eof.type = TokenType::EndOfFile;
        if (!tokens_.empty()) {
            eof.line = tokens_.back().line;
            eof.column = tokens_.back().column;
        }
        tokens_.push_back(eof);
    }
}

const Token& Parser::peek(std::size_t offset) const {
    std::size_t index = pos_ + offset;
    if (index >= tokens_.size()) return tokens_.back();
    return tokens_[index];
}

const Token& Parser::advance() {
    const Token& token = tokens_[pos_];
    if (pos_ + 1 < tokens_.size()) ++pos_;
    return token;
}

bool Parser::check(TokenType type, std::size_t offset) const {
    return peek(offset).type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

const Token& Parser::expect(TokenType type, const std::string& what) {
    if (!check(type)) fail("expected " + what + " but found " + describe(peek()), peek());
    return advance();
}

void Parser::skipBangs() {
    while (check(TokenType::Bang)) advance();
}

void Parser::fail(const std::string& message, const Token& at) const {
    throw ParseError(message, at.line, at.column);
}

std::string Parser::describe(const Token& token) {
    switch (token.type) {
        case TokenType::EndOfFile: return "end of file";
        case TokenType::VarRef: return "'$" + token.text + "'";
        case TokenType::StringLit: return "\"" + token.text + "\"";
        default: return "'" + token.text + "'";
    }
}

Timestamp Parser::timestampFrom(const Token& token) {
    Timestamp timestamp;
    timestamp.raw = token.text;
    timestamp.seconds = static_cast<double>(token.centiseconds) / 100.0;
    return timestamp;
}

ProgramAST Parser::parse() {
    ProgramAST program;
    skipBangs();
    program.create_stmt = parseCreate();

    bool hasMain = false;
    for (;;) {
        skipBangs();
        const Token& token = peek();
        if (token.type == TokenType::EndOfFile) break;

        if (token.type == TokenType::KwMain) {
            if (hasMain) fail("duplicate main block", token);
            program.main_block = parseMain();
            hasMain = true;
        } else if (token.type == TokenType::KwFunc) {
            program.functions.push_back(parseFunction());
        } else if (token.type == TokenType::Identifier && check(TokenType::Equals, 1)) {
            program.declarations.push_back(parseDeclaration());
        } else if (token.type == TokenType::VarRef && check(TokenType::Dot, 1)) {
            program.mask_bindings.push_back(parseMaskBinding());
        } else {
            fail("unexpected " + describe(token), token);
        }
    }

    if (!hasMain) fail("missing main block", peek());
    return program;
}

CreateStmt Parser::parseCreate() {
    expect(TokenType::KwCreate, "'Create'");
    expect(TokenType::LParen, "'('");

    CreateStmt stmt;
    stmt.name = expect(TokenType::StringLit, "a project name string").text;
    expect(TokenType::Comma, "','");
    stmt.duration = timestampFrom(expect(TokenType::TimeLit, "a duration timestamp"));
    expect(TokenType::Comma, "','");

    const Token& at = peek();
    VectorValue resolution = parseVectorArg();
    if (resolution.is_single_var || resolution.x_operand.type != OperandType::LiteralNumber ||
        resolution.y_operand.type != OperandType::LiteralNumber) {
        fail("Create requires a literal resolution", at);
    }
    stmt.width = static_cast<int>(resolution.x_operand.number_val);
    stmt.height = static_cast<int>(resolution.y_operand.number_val);

    if (match(TokenType::Comma)) {
        stmt.fps = expect(TokenType::NumberLit, "an fps number").number;
    }

    expect(TokenType::RParen, "')'");
    expect(TokenType::Bang, "'!'");
    return stmt;
}

VariableDecl Parser::parseDeclaration() {
    VariableDecl decl;
    decl.name = advance().text;
    expect(TokenType::Equals, "'='");

    switch (peek().type) {
        case TokenType::KwLoad:
        case TokenType::KwTake:
        case TokenType::AtText:
        case TokenType::AtShape:
        case TokenType::AtMask:
        case TokenType::AtCamera:
            decl.decl_type = DeclType::Asset;
            decl.asset_expr = parseAssetExpr();
            break;
        default: {
            Value value = parseValue();
            decl.decl_type = DeclType::Constant;
            if (value.is_vector) {
                decl.is_vector_constant = true;
                decl.vector_val = value.vector;
            } else {
                decl.constant_val = value.operand;
            }
            break;
        }
    }

    expect(TokenType::Bang, "'!'");
    return decl;
}

MaskBinding Parser::parseMaskBinding() {
    MaskBinding binding;
    binding.target_var = advance().text;
    expect(TokenType::Dot, "'.'");
    const Token& use = expect(TokenType::Identifier, "'use'");
    if (use.text != "use") fail("expected 'use' but found " + describe(use), use);
    expect(TokenType::LParen, "'('");
    binding.mask_var = expect(TokenType::VarRef, "a mask reference").text;
    expect(TokenType::RParen, "')'");
    expect(TokenType::Bang, "'!'");
    return binding;
}

MainBlock Parser::parseMain() {
    expect(TokenType::KwMain, "'main'");
    expect(TokenType::LParen, "'('");
    expect(TokenType::RParen, "')'");
    expect(TokenType::LBrace, "'{'");

    MainBlock block;
    for (;;) {
        skipBangs();
        if (check(TokenType::RBrace)) break;
        if (check(TokenType::EndOfFile)) fail("unterminated main block", peek());
        block.asset_blocks.push_back(parseAssetBlock());
    }

    expect(TokenType::RBrace, "'}'");
    match(TokenType::Bang);
    return block;
}

FunctionDecl Parser::parseFunction() {
    expect(TokenType::KwFunc, "'func'");
    FunctionDecl func;
    func.name = expect(TokenType::Identifier, "a function name").text;
    expect(TokenType::LParen, "'('");
    expect(TokenType::RParen, "')'");
    expect(TokenType::LBrace, "'{'");
    func.body = parseActionList();
    expect(TokenType::RBrace, "'}'");
    expect(TokenType::Bang, "'!'");
    return func;
}

AssetBlock Parser::parseAssetBlock() {
    AssetBlock block;
    block.target_var = expect(TokenType::VarRef, "an asset reference").text;
    expect(TokenType::LBrace, "'{'");
    block.actions = parseActionList();
    expect(TokenType::RBrace, "'}'");
    expect(TokenType::Bang, "'!'");
    return block;
}

std::vector<ActionItem> Parser::parseActionList() {
    std::vector<ActionItem> actions;
    if (check(TokenType::RBrace)) return actions;

    for (;;) {
        actions.push_back(parseActionItem());
        if (!match(TokenType::Semicolon)) break;
        if (check(TokenType::RBrace)) break;
    }
    return actions;
}

ActionItem Parser::parseActionItem() {
    ActionItem item;

    if (check(TokenType::Identifier) && check(TokenType::LParen, 1)) {
        FunctionCallAction call;
        call.func_name = advance().text;
        advance();
        expect(TokenType::RParen, "')'");
        item.kind = ActionKind::FunctionCall;
        item.action = std::move(call);
        return item;
    }

    expect(TokenType::LParen, "'(' to start an action");
    Operand start = parseTimeOperand();

    if (match(TokenType::Slash)) {
        TransitionAction transition;
        transition.start_time = start;
        transition.end_time = parseTimeOperand();
        expect(TokenType::Comma, "','");
        transition.property_name = expect(TokenType::Identifier, "a property name").text;
        expect(TokenType::Comma, "','");
        transition.ramp = parseRamp();
        expect(TokenType::RParen, "')'");
        item.kind = ActionKind::Transition;
        item.action = std::move(transition);
    } else {
        InstantAction instant;
        instant.time = start;
        expect(TokenType::Comma, "','");
        instant.property_name = expect(TokenType::Identifier, "a property name").text;
        expect(TokenType::Comma, "','");
        Value value = parseValue();
        instant.is_vector = value.is_vector;
        if (value.is_vector) {
            instant.vec_val = value.vector;
        } else {
            instant.val = value.operand;
        }
        expect(TokenType::RParen, "')'");
        item.kind = ActionKind::Instant;
        item.action = std::move(instant);
    }

    return item;
}

Ramp Parser::parseRamp() {
    Ramp ramp;

    if (match(TokenType::KwPass)) {
        ramp.is_pass = true;
    } else {
        Value start = parseValue();
        ramp.start_is_vec = start.is_vector;
        if (start.is_vector) {
            ramp.start_vec = start.vector;
        } else {
            ramp.start_val = start.operand;
        }
    }

    if (match(TokenType::Pipe)) {
        ramp.easing = EasingType::Linear;
    } else if (check(TokenType::Slash) || check(TokenType::Backslash)) {
        const TokenType first = advance().type;
        ramp.strength = parseOperand();
        if (!check(TokenType::Slash) && !check(TokenType::Backslash)) {
            fail("expected '/' or '\\' after the easing strength but found " + describe(peek()), peek());
        }
        const TokenType second = advance().type;
        if (first == TokenType::Slash && second == TokenType::Slash) {
            ramp.easing = EasingType::EaseIn;
        } else if (first == TokenType::Backslash && second == TokenType::Backslash) {
            ramp.easing = EasingType::EaseOut;
        } else if (first == TokenType::Slash) {
            ramp.easing = EasingType::EaseInOut;
        } else {
            ramp.easing = EasingType::EaseOutIn;
        }
    } else {
        fail("expected '|', '/' or '\\' in a transition but found " + describe(peek()), peek());
    }

    Value end = parseValue();
    ramp.end_is_vec = end.is_vector;
    if (end.is_vector) {
        ramp.end_vec = end.vector;
    } else {
        ramp.end_val = end.operand;
    }
    return ramp;
}

AssetExpr Parser::parseAssetExpr() {
    AssetExpr expr;
    const Token& token = peek();

    switch (token.type) {
        case TokenType::KwLoad:
            expr.kind = AssetKind::Load;
            expr.details = parseLoad();
            break;
        case TokenType::KwTake:
            expr.kind = AssetKind::Take;
            expr.details = parseTake();
            break;
        case TokenType::AtText:
            expr.kind = AssetKind::Text;
            expr.details = parseText();
            break;
        case TokenType::AtShape:
            expr.kind = AssetKind::Shape;
            expr.details = parseShape();
            break;
        case TokenType::AtMask:
            expr.kind = AssetKind::Mask;
            expr.details = parseMask();
            break;
        case TokenType::AtCamera:
            expr.kind = AssetKind::Camera;
            expr.details = parseCamera();
            break;
        default:
            fail("expected an asset expression but found " + describe(token), token);
    }

    return expr;
}

LoadExpr Parser::parseLoad() {
    expect(TokenType::KwLoad, "'load'");
    expect(TokenType::LParen, "'('");
    const Token& at = peek();
    LoadExpr load;
    load.path = parseOperand();
    if (load.path.type != OperandType::LiteralString && load.path.type != OperandType::VariableRef) {
        fail("load expects a path string", at);
    }
    expect(TokenType::RParen, "')'");
    return load;
}

TakeExpr Parser::parseTake() {
    expect(TokenType::KwTake, "'take'");
    expect(TokenType::LParen, "'('");
    TakeExpr take;
    take.source_var = expect(TokenType::VarRef, "a source asset reference").text;
    expect(TokenType::Comma, "','");
    take.start_bound = parseClipBound();
    expect(TokenType::Comma, "','");
    take.end_bound = parseClipBound();
    expect(TokenType::RParen, "')'");
    return take;
}

ClipBound Parser::parseClipBound() {
    ClipBound bound;
    const Token& token = peek();

    if (token.type == TokenType::KwStart) {
        advance();
        bound.kind = ClipBoundKind::KeywordStart;
    } else if (token.type == TokenType::KwEnd) {
        advance();
        bound.kind = ClipBoundKind::KeywordEnd;
    } else if (token.type == TokenType::Plus) {
        advance();
        const Token& next = peek();
        if (next.type != TokenType::NumberLit && next.type != TokenType::TimeLit && next.type != TokenType::VarRef) {
            fail("expected a duration after '+' but found " + describe(next), next);
        }
        bound.kind = ClipBoundKind::RelativeDuration;
        bound.operand = parseOperand();
    } else if (token.type == TokenType::TimeLit || token.type == TokenType::VarRef) {
        bound.kind = ClipBoundKind::Timestamp;
        bound.operand = parseOperand();
    } else {
        fail("expected 'start', 'end', a timestamp or '+duration' but found " + describe(token), token);
    }

    return bound;
}

TextExpr Parser::parseText() {
    expect(TokenType::AtText, "'@text'");
    expect(TokenType::LParen, "'('");
    TextExpr text;
    text.size = parseOperand();
    expect(TokenType::Comma, "','");
    text.color = parseOperand();
    expect(TokenType::Comma, "','");
    text.font_path = parseOperand();
    expect(TokenType::Comma, "','");
    text.stroke_width = parseOperand();
    expect(TokenType::Comma, "','");
    text.stroke_color = parseOperand();
    expect(TokenType::Comma, "','");
    text.content = parseOperand();
    expect(TokenType::RParen, "')'");
    return text;
}

ShapeExpr Parser::parseShape() {
    expect(TokenType::AtShape, "'@shape'");
    expect(TokenType::LParen, "'('");

    ShapeExpr shape;
    const Token& at = peek();
    Operand type = parseOperand();
    if (type.type != OperandType::LiteralString || (type.str_val != "rect" && type.str_val != "circle")) {
        fail("shape type must be \"rect\" or \"circle\"", at);
    }
    shape.shape_type = type.str_val;
    shape.is_rect = type.str_val == "rect";
    expect(TokenType::Comma, "','");

    if (shape.is_rect) {
        shape.rect_dimension = parseVectorArg();
    } else {
        shape.circle_radius = parseOperand();
    }
    expect(TokenType::Comma, "','");
    shape.fill_color = parseOperand();
    expect(TokenType::Comma, "','");
    shape.stroke_width = parseOperand();
    expect(TokenType::Comma, "','");
    shape.stroke_color = parseOperand();
    expect(TokenType::Comma, "','");
    shape.border_radius = parseOperand();
    expect(TokenType::RParen, "')'");
    return shape;
}

MaskExpr Parser::parseMask() {
    expect(TokenType::AtMask, "'@mask'");
    expect(TokenType::LParen, "'('");

    MaskExpr mask;
    const Token& at = peek();
    Operand type = parseOperand();
    if (type.type != OperandType::LiteralString || (type.str_val != "rect" && type.str_val != "circle")) {
        fail("mask type must be \"rect\" or \"circle\"", at);
    }
    mask.mask_type = type.str_val;
    mask.is_rect = type.str_val == "rect";
    expect(TokenType::Comma, "','");

    if (mask.is_rect) {
        mask.rect_dimension = parseVectorArg();
    } else {
        mask.circle_radius = parseOperand();
    }
    expect(TokenType::RParen, "')'");
    return mask;
}

CameraExpr Parser::parseCamera() {
    expect(TokenType::AtCamera, "'@camera'");
    expect(TokenType::LParen, "'('");
    CameraExpr camera;
    camera.resolution = parseVectorArg();
    expect(TokenType::RParen, "')'");
    return camera;
}

Operand Parser::parseOperand() {
    const Token& token = peek();

    switch (token.type) {
        case TokenType::NumberLit:
            advance();
            return Operand::makeNumber(token.number);
        case TokenType::StringLit:
            advance();
            return Operand::makeString(token.text);
        case TokenType::ColorLit:
            advance();
            return Operand::makeColor(token.text);
        case TokenType::TimeLit:
            advance();
            return Operand::makeTimestamp(timestampFrom(token));
        case TokenType::KwTrue:
            advance();
            return Operand::makeBool(true);
        case TokenType::KwFalse:
            advance();
            return Operand::makeBool(false);
        case TokenType::KwCenter:
            advance();
            return Operand::makeKeyword("center");
        case TokenType::VarRef:
            advance();
            return Operand::makeVarRef(token.text);
        default:
            fail("expected a value but found " + describe(token), token);
    }
}

Operand Parser::parseTimeOperand() {
    const Token& token = peek();
    if (token.type != TokenType::TimeLit && token.type != TokenType::VarRef) {
        fail("expected a timestamp but found " + describe(token), token);
    }
    return parseOperand();
}

Parser::Value Parser::parseValue() {
    Value value;

    if (check(TokenType::ResolutionLit)) {
        value.is_vector = true;
        value.vector = parseVectorArg();
        return value;
    }

    value.operand = parseOperand();
    if (match(TokenType::KwCross)) {
        value.is_vector = true;
        value.vector.x_operand = value.operand;
        value.vector.y_operand = parseOperand();
        value.operand = Operand();
    }
    return value;
}

VectorValue Parser::parseVectorArg() {
    VectorValue vector;

    if (check(TokenType::ResolutionLit)) {
        const Token& token = advance();
        vector.x_operand = Operand::makeNumber(token.width);
        vector.y_operand = Operand::makeNumber(token.height);
        return vector;
    }

    if (check(TokenType::VarRef) && !check(TokenType::KwCross, 1)) {
        vector.is_single_var = true;
        vector.var_name = advance().text;
        return vector;
    }

    vector.x_operand = parseOperand();
    expect(TokenType::KwCross, "'x'");
    vector.y_operand = parseOperand();
    return vector;
}
