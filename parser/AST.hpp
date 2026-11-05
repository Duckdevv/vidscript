#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

struct Timestamp {
    std::string raw;
    double seconds = 0.0;

    static Timestamp parse(const std::string& str) {
        Timestamp ts;
        ts.raw = str;
        size_t colon_pos = str.find(':');
        if (colon_pos != std::string::npos) {
            double mins = std::stod(str.substr(0, colon_pos));
            double secs = std::stod(str.substr(colon_pos + 1));
            ts.seconds = mins * 60.0 + secs;
        } else {
            ts.seconds = std::stod(str);
        }
        return ts;
    }
};

enum class OperandType {
    LiteralNumber,
    LiteralString,
    LiteralColor,
    LiteralTimestamp,
    LiteralBool,
    Keyword,
    VariableRef
};

struct Operand {
    OperandType type = OperandType::LiteralNumber;
    double number_val = 0.0;
    std::string str_val;
    bool bool_val = false;
    Timestamp timestamp_val;

    static Operand makeNumber(double n) {
        Operand op;
        op.type = OperandType::LiteralNumber;
        op.number_val = n;
        return op;
    }

    static Operand makeString(const std::string& s) {
        Operand op;
        op.type = OperandType::LiteralString;
        op.str_val = s;
        return op;
    }

    static Operand makeColor(const std::string& c) {
        Operand op;
        op.type = OperandType::LiteralColor;
        op.str_val = c;
        return op;
    }

    static Operand makeTimestamp(const Timestamp& ts) {
        Operand op;
        op.type = OperandType::LiteralTimestamp;
        op.timestamp_val = ts;
        op.str_val = ts.raw;
        return op;
    }

    static Operand makeBool(bool b) {
        Operand op;
        op.type = OperandType::LiteralBool;
        op.bool_val = b;
        return op;
    }

    static Operand makeKeyword(const std::string& kw) {
        Operand op;
        op.type = OperandType::Keyword;
        op.str_val = kw;
        return op;
    }

    static Operand makeVarRef(const std::string& var) {
        Operand op;
        op.type = OperandType::VariableRef;
        op.str_val = var;
        return op;
    }
};

struct VectorValue {
    bool is_single_var = false;
    std::string var_name;
    Operand x_operand;
    Operand y_operand;
};

struct CreateStmt {
    std::string name;
    Timestamp duration;
    int width = 0;
    int height = 0;
    double fps = 0.0;
};

enum class AssetKind {
    Load,
    Take,
    Text,
    Shape,
    Mask,
    Camera
};

struct LoadExpr {
    Operand path;
};

enum class ClipBoundKind {
    KeywordStart,
    KeywordEnd,
    Timestamp,
    RelativeDuration
};

struct ClipBound {
    ClipBoundKind kind = ClipBoundKind::KeywordStart;
    Operand operand;
};

struct TakeExpr {
    std::string source_var;
    ClipBound start_bound;
    ClipBound end_bound;
};

struct TextExpr {
    Operand size;
    Operand color;
    Operand font_path;
    Operand stroke_width;
    Operand stroke_color;
    Operand content;
};

struct ShapeExpr {
    std::string shape_type;
    bool is_rect = false;
    Operand circle_radius;
    VectorValue rect_dimension;
    Operand fill_color;
    Operand stroke_width;
    Operand stroke_color;
    Operand border_radius;
};

struct MaskExpr {
    std::string mask_type;
    bool is_rect = false;
    Operand circle_radius;
    VectorValue rect_dimension;
};

struct CameraExpr {
    VectorValue resolution;
};

struct AssetExpr {
    AssetKind kind = AssetKind::Load;
    std::variant<LoadExpr, TakeExpr, TextExpr, ShapeExpr, MaskExpr, CameraExpr> details;
};

enum class DeclType {
    Constant,
    Asset
};

struct VariableDecl {
    std::string name;
    DeclType decl_type = DeclType::Constant;
    Operand constant_val;
    bool is_vector_constant = false;
    VectorValue vector_val;
    AssetExpr asset_expr;
};

struct MaskBinding {
    std::string target_var;
    std::string mask_var;
};

enum class EasingType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseOutIn
};

struct Ramp {
    bool is_pass = false;
    Operand start_val;
    VectorValue start_vec;
    bool start_is_vec = false;

    EasingType easing = EasingType::Linear;
    Operand strength;

    Operand end_val;
    VectorValue end_vec;
    bool end_is_vec = false;
};

struct InstantAction {
    Operand time;
    std::string property_name;
    bool is_vector = false;
    Operand val;
    VectorValue vec_val;
};

struct TransitionAction {
    Operand start_time;
    Operand end_time;
    std::string property_name;
    Ramp ramp;
};

struct FunctionCallAction {
    std::string func_name;
};

enum class ActionKind {
    Instant,
    Transition,
    FunctionCall
};

struct ActionItem {
    ActionKind kind = ActionKind::Instant;
    std::variant<InstantAction, TransitionAction, FunctionCallAction> action;
};

struct AssetBlock {
    std::string target_var;
    std::vector<ActionItem> actions;
};

struct MainBlock {
    std::vector<AssetBlock> asset_blocks;
};

struct FunctionDecl {
    std::string name;
    std::vector<ActionItem> body;
};

struct ProgramAST {
    CreateStmt create_stmt;
    std::vector<VariableDecl> declarations;
    std::vector<MaskBinding> mask_bindings;
    MainBlock main_block;
    std::vector<FunctionDecl> functions;
};

#endif
