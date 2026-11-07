#include "Resolver.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <unordered_map>

namespace {

struct RawValue {
    bool is_vector = false;
    Operand operand;
    VectorValue vector;
};

struct Deref {
    bool is_vector = false;
    Operand operand;
    VectorValue vector;
};

struct PropState {
    bool have_prev = false;
    long long prev_start_cs = 0;
    long long prev_end_cs = 0;
    int prev_end_frame = -1;
    PropValue last;
};

class Resolver {
public:
    explicit Resolver(const ProgramAST& program) : program_(program) {}

    Timeline run() {
        const CreateStmt& create = program_.create_stmt;
        if (create.fps <= 0.0) fail("Create requires a positive fps");
        if (create.width <= 0 || create.height <= 0) fail("Create requires a valid resolution");

        timeline_.name = create.name;
        timeline_.width = create.width;
        timeline_.height = create.height;
        timeline_.fps = create.fps;
        timeline_.total_frames = static_cast<int>(std::llround(create.duration.seconds * create.fps));
        if (timeline_.total_frames <= 0) fail("project duration is too short");

        indexNames();
        buildAssets();
        bindMasks();
        applyMain();

        for (AssetTimeline& asset : timeline_.assets) {
            if (asset.info.placed && (asset.info.type == AssetType::Video || asset.info.type == AssetType::Audio)) {
                buildMediaTable(asset);
            }
        }
        return std::move(timeline_);
    }

private:
    const ProgramAST& program_;
    Timeline timeline_;
    std::unordered_map<std::string, const VariableDecl*> decls_;
    std::unordered_map<std::string, const FunctionDecl*> funcs_;
    std::unordered_map<std::string, std::size_t> asset_index_;

    [[noreturn]] static void fail(const std::string& message) {
        throw ResolveError(message);
    }

    static std::string formatCs(long long cs) {
        char buffer[48];
        std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld.%02lld", cs / 6000, (cs / 100) % 60, cs % 100);
        return buffer;
    }

    static std::string lower(std::string text) {
        for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    }

    static bool isVisual(AssetType type) {
        return type == AssetType::Image || type == AssetType::Video || type == AssetType::Text ||
               type == AssetType::Shape;
    }

    void indexNames() {
        for (const VariableDecl& decl : program_.declarations) {
            if (!decls_.emplace(decl.name, &decl).second) fail("'" + decl.name + "' is declared more than once");
        }
        for (const FunctionDecl& func : program_.functions) {
            if (!funcs_.emplace(func.name, &func).second) fail("function '" + func.name + "' is declared more than once");
        }
    }

    Deref deref(const Operand& start) const {
        Operand op = start;
        for (int depth = 0; depth < 32; ++depth) {
            if (op.type != OperandType::VariableRef) {
                Deref result;
                result.operand = op;
                return result;
            }
            auto it = decls_.find(op.str_val);
            if (it == decls_.end()) fail("undefined variable '$" + op.str_val + "'");
            const VariableDecl& decl = *it->second;
            if (decl.decl_type != DeclType::Constant) fail("'$" + op.str_val + "' is an asset, not a value");
            if (decl.is_vector_constant) {
                Deref result;
                result.is_vector = true;
                result.vector = decl.vector_val;
                return result;
            }
            op = decl.constant_val;
        }
        fail("variable reference is nested too deeply");
    }

    double resolveNumber(const Operand& op) const {
        Deref d = deref(op);
        if (d.is_vector || d.operand.type != OperandType::LiteralNumber) fail("expected a number");
        return d.operand.number_val;
    }

    double resolveSeconds(const Operand& op) const {
        Deref d = deref(op);
        if (d.is_vector) fail("expected a time value");
        if (d.operand.type == OperandType::LiteralTimestamp) return d.operand.timestamp_val.seconds;
        if (d.operand.type == OperandType::LiteralNumber) return d.operand.number_val;
        fail("expected a time value");
    }

    std::string resolveString(const Operand& op) const {
        Deref d = deref(op);
        if (d.is_vector || d.operand.type != OperandType::LiteralString) fail("expected a string");
        return d.operand.str_val;
    }

    bool resolveBool(const Operand& op) const {
        Deref d = deref(op);
        if (d.is_vector || d.operand.type != OperandType::LiteralBool) fail("expected true or false");
        return d.operand.bool_val;
    }

    Color resolveColor(const Operand& op) const {
        Deref d = deref(op);
        if (d.is_vector || d.operand.type != OperandType::LiteralColor) fail("expected a color");
        const std::string& hex = d.operand.str_val;
        if (hex.size() != 7 && hex.size() != 9) fail("invalid color '" + hex + "'");
        Color color;
        color.r = std::stoi(hex.substr(1, 2), nullptr, 16);
        color.g = std::stoi(hex.substr(3, 2), nullptr, 16);
        color.b = std::stoi(hex.substr(5, 2), nullptr, 16);
        color.a = hex.size() == 9 ? std::stoi(hex.substr(7, 2), nullptr, 16) : 255;
        return color;
    }

    double resolveAxis(const Operand& op, bool isX) const {
        Deref d = deref(op);
        if (d.is_vector) fail("expected a number or 'center'");
        if (d.operand.type == OperandType::Keyword && d.operand.str_val == "center") {
            return isX ? timeline_.width / 2.0 : timeline_.height / 2.0;
        }
        if (d.operand.type == OperandType::LiteralNumber) return d.operand.number_val;
        fail("expected a number or 'center'");
    }

    Vec2 resolveVec(const VectorValue& vector) const {
        if (vector.is_single_var) {
            Deref d = deref(Operand::makeVarRef(vector.var_name));
            if (!d.is_vector) fail("'$" + vector.var_name + "' is not a vector");
            return resolveVec(d.vector);
        }
        return Vec2{resolveAxis(vector.x_operand, true), resolveAxis(vector.y_operand, false)};
    }

    PropValue convert(const RawValue& raw, PropType type) const {
        switch (type) {
            case PropType::Vec2: {
                if (raw.is_vector) return resolveVec(raw.vector);
                Deref d = deref(raw.operand);
                if (!d.is_vector) fail("expected a vector value (X x Y)");
                return resolveVec(d.vector);
            }
            case PropType::Number:
                if (raw.is_vector) fail("expected a number");
                return resolveNumber(raw.operand);
            case PropType::Time:
                if (raw.is_vector) fail("expected a time value");
                return resolveSeconds(raw.operand);
            case PropType::Bool:
                if (raw.is_vector) fail("expected true or false");
                return resolveBool(raw.operand);
            case PropType::Color:
                if (raw.is_vector) fail("expected a color");
                return resolveColor(raw.operand);
            case PropType::String:
                if (raw.is_vector) fail("expected a string");
                return resolveString(raw.operand);
        }
        fail("unsupported property type");
    }

    long long timeToCs(const Operand& op) const {
        Deref d = deref(op);
        if (d.is_vector || d.operand.type != OperandType::LiteralTimestamp) fail("expected a timestamp");
        return std::llround(d.operand.timestamp_val.seconds * 100.0);
    }

    int frameFloor(long long cs) const {
        return static_cast<int>(std::floor(static_cast<double>(cs) * timeline_.fps / 100.0 + 1e-9));
    }

    int frameNearest(long long cs) const {
        return static_cast<int>(std::ceil(static_cast<double>(cs) * timeline_.fps / 100.0 - 0.5 - 1e-9));
    }

    Vec2 center() const {
        return Vec2{timeline_.width / 2.0, timeline_.height / 2.0};
    }

    static AssetType typeFromPath(const std::string& path) {
        const std::size_t dot = path.find_last_of('.');
        if (dot == std::string::npos) fail("cannot determine the asset type of '" + path + "'");
        const std::string ext = lower(path.substr(dot + 1));
        static const std::set<std::string> images = {"png", "jpg", "jpeg", "bmp", "gif", "webp", "tga"};
        static const std::set<std::string> videos = {"mp4", "mov", "mkv", "webm", "avi", "m4v"};
        static const std::set<std::string> audios = {"mp3", "wav", "ogg", "flac", "m4a", "aac", "opus"};
        if (images.count(ext)) return AssetType::Image;
        if (videos.count(ext)) return AssetType::Video;
        if (audios.count(ext)) return AssetType::Audio;
        fail("cannot determine the asset type of '" + path + "'");
    }

    static void addTrack(AssetTimeline& asset, const std::string& name, PropType type, PropValue initial,
                         bool instantOnly = false) {
        PropTrack track;
        track.type = type;
        track.instant_only = instantOnly || type == PropType::Bool || type == PropType::String || type == PropType::Time;
        track.initial = std::move(initial);
        asset.tracks.emplace(name, std::move(track));
    }

    void addVisualTracks(AssetTimeline& asset) const {
        addTrack(asset, "opacity", PropType::Number, 1.0);
        addTrack(asset, "layer", PropType::Number, 0.0, true);
        addTrack(asset, "size", PropType::Number, 1.0);
        addTrack(asset, "scale", PropType::Vec2, Vec2{1.0, 1.0});
        addTrack(asset, "rotation", PropType::Number, 0.0);
        addTrack(asset, "position", PropType::Vec2, center());
    }

    void addVideoTracks(AssetTimeline& asset) const {
        addVisualTracks(asset);
        addTrack(asset, "speed", PropType::Number, 1.0);
        addTrack(asset, "pause", PropType::Bool, false);
        addTrack(asset, "mute", PropType::Bool, false);
        addTrack(asset, "volume", PropType::Number, 1.0);
        addTrack(asset, "pitch_scale", PropType::Number, 1.0);
    }

    void addAudioTracks(AssetTimeline& asset) const {
        addTrack(asset, "volume", PropType::Number, 1.0);
        addTrack(asset, "speed", PropType::Number, 1.0);
        addTrack(asset, "pitch_scale", PropType::Number, 1.0);
        addTrack(asset, "pause", PropType::Bool, false);
        addTrack(asset, "pointer", PropType::Time, 0.0);
        addTrack(asset, "replay", PropType::Bool, false);
    }

    void resolveTake(const TakeExpr& expr, AssetInfo& info) const {
        auto it = asset_index_.find(expr.source_var);
        if (it == asset_index_.end()) fail("take source '$" + expr.source_var + "' must be a media asset declared earlier");
        const AssetInfo& source = timeline_.assets[it->second].info;
        if (source.type != AssetType::Video && source.type != AssetType::Audio) {
            fail("take source '$" + expr.source_var + "' must be a video or audio asset");
        }
        info.type = source.type;
        info.path = source.path;

        double start = 0.0;
        switch (expr.start_bound.kind) {
            case ClipBoundKind::KeywordStart:
                break;
            case ClipBoundKind::Timestamp:
                start = resolveSeconds(expr.start_bound.operand);
                break;
            default:
                fail("take start must be 'start' or a timestamp");
        }

        double end = -1.0;
        switch (expr.end_bound.kind) {
            case ClipBoundKind::KeywordEnd:
                break;
            case ClipBoundKind::Timestamp:
                end = resolveSeconds(expr.end_bound.operand);
                if (end < start) fail("take end is before its start");
                break;
            case ClipBoundKind::RelativeDuration:
                end = start + resolveSeconds(expr.end_bound.operand);
                break;
            default:
                fail("take end must be 'end', a timestamp or '+duration'");
        }

        info.take_start = source.take_start + start;
        info.take_end = end >= 0.0 ? source.take_start + end : source.take_end;
    }

    void buildAssets() {
        for (const VariableDecl& decl : program_.declarations) {
            if (decl.decl_type != DeclType::Asset) continue;

            try {
                AssetTimeline asset;
                asset.info.name = decl.name;
                const AssetExpr& expr = decl.asset_expr;

                switch (expr.kind) {
                    case AssetKind::Load: {
                        const LoadExpr& load = std::get<LoadExpr>(expr.details);
                        asset.info.path = resolveString(load.path);
                        asset.info.type = typeFromPath(asset.info.path);
                        break;
                    }
                    case AssetKind::Take:
                        resolveTake(std::get<TakeExpr>(expr.details), asset.info);
                        break;
                    case AssetKind::Text:
                        asset.info.type = AssetType::Text;
                        break;
                    case AssetKind::Shape:
                        asset.info.type = AssetType::Shape;
                        break;
                    case AssetKind::Mask:
                        asset.info.type = AssetType::Mask;
                        break;
                    case AssetKind::Camera:
                        asset.info.type = AssetType::Camera;
                        break;
                }

                switch (asset.info.type) {
                    case AssetType::Image:
                        addVisualTracks(asset);
                        break;
                    case AssetType::Video:
                        addVideoTracks(asset);
                        break;
                    case AssetType::Audio:
                        addAudioTracks(asset);
                        break;
                    case AssetType::Text:
                        initText(asset, std::get<TextExpr>(expr.details));
                        break;
                    case AssetType::Shape:
                        initShape(asset, std::get<ShapeExpr>(expr.details));
                        break;
                    case AssetType::Mask:
                        initMask(asset, std::get<MaskExpr>(expr.details));
                        break;
                    case AssetType::Camera:
                        initCamera(asset, std::get<CameraExpr>(expr.details));
                        break;
                }

                asset_index_[decl.name] = timeline_.assets.size();
                timeline_.assets.push_back(std::move(asset));
            } catch (const ResolveError& error) {
                fail("$" + decl.name + ": " + error.what());
            }
        }
    }

    void initText(AssetTimeline& asset, const TextExpr& expr) {
        addVisualTracks(asset);
        asset.info.base_size = resolveNumber(expr.size);
        addTrack(asset, "color", PropType::Color, resolveColor(expr.color));
        addTrack(asset, "font", PropType::String, resolveString(expr.font_path));
        addTrack(asset, "stroke_width", PropType::Number, resolveNumber(expr.stroke_width));
        addTrack(asset, "stroke_color", PropType::Color, resolveColor(expr.stroke_color));
        addTrack(asset, "content", PropType::String, resolveString(expr.content));
    }

    void initShape(AssetTimeline& asset, const ShapeExpr& expr) {
        addVisualTracks(asset);
        asset.info.is_rect = expr.is_rect;
        if (expr.is_rect) {
            asset.info.dimension = resolveVec(expr.rect_dimension);
        } else {
            asset.info.radius = resolveNumber(expr.circle_radius);
        }
        addTrack(asset, "color", PropType::Color, resolveColor(expr.fill_color));
        addTrack(asset, "border_radius", PropType::Number, resolveNumber(expr.border_radius));
        addTrack(asset, "stroke_width", PropType::Number, resolveNumber(expr.stroke_width));
        addTrack(asset, "stroke_color", PropType::Color, resolveColor(expr.stroke_color));
    }

    void initMask(AssetTimeline& asset, const MaskExpr& expr) {
        asset.info.is_rect = expr.is_rect;
        if (expr.is_rect) {
            asset.info.dimension = resolveVec(expr.rect_dimension);
        } else {
            asset.info.radius = resolveNumber(expr.circle_radius);
        }
        addTrack(asset, "size", PropType::Number, 1.0);
        addTrack(asset, "scale", PropType::Vec2, Vec2{1.0, 1.0});
        addTrack(asset, "position", PropType::Vec2, center());
        addTrack(asset, "opacity", PropType::Number, 1.0);
        addTrack(asset, "invert", PropType::Bool, false);
    }

    void initCamera(AssetTimeline& asset, const CameraExpr& expr) {
        resolveVec(expr.resolution);
        if (timeline_.camera_index >= 0) fail("only one camera is allowed per project");
        timeline_.camera_index = static_cast<int>(timeline_.assets.size());
        asset.info.placed = true;
        addTrack(asset, "position", PropType::Vec2, center());
        addTrack(asset, "zoom", PropType::Number, 1.0);
        addTrack(asset, "opacity", PropType::Number, 1.0);
        addTrack(asset, "rotation", PropType::Number, 0.0);
    }

    void bindMasks() {
        for (const MaskBinding& binding : program_.mask_bindings) {
            auto targetIt = asset_index_.find(binding.target_var);
            auto maskIt = asset_index_.find(binding.mask_var);
            if (targetIt == asset_index_.end()) fail("mask target '$" + binding.target_var + "' is not a declared asset");
            if (maskIt == asset_index_.end()) fail("mask '$" + binding.mask_var + "' is not a declared asset");

            AssetTimeline& target = timeline_.assets[targetIt->second];
            AssetTimeline& mask = timeline_.assets[maskIt->second];
            if (!isVisual(target.info.type)) fail("'$" + binding.target_var + "' cannot be masked");
            if (mask.info.type != AssetType::Mask) fail("'$" + binding.mask_var + "' is not a mask");
            if (!target.info.mask.empty()) fail("'$" + binding.target_var + "' already has a mask");

            target.info.mask = binding.mask_var;
            mask.info.placed = true;
        }
    }

    void expand(const std::vector<ActionItem>& actions, std::vector<const ActionItem*>& out, int depth) const {
        if (depth > 16) fail("functions are nested too deeply");
        for (const ActionItem& item : actions) {
            if (std::holds_alternative<FunctionCallAction>(item.action)) {
                const std::string& name = std::get<FunctionCallAction>(item.action).func_name;
                auto it = funcs_.find(name);
                if (it == funcs_.end()) fail("undefined function '" + name + "'");
                expand(it->second->body, out, depth + 1);
            } else {
                out.push_back(&item);
            }
        }
    }

    void applyMain() {
        std::set<std::string> seen;
        for (const AssetBlock& block : program_.main_block.asset_blocks) {
            auto it = asset_index_.find(block.target_var);
            if (it == asset_index_.end()) fail("'$" + block.target_var + "' is not a declared asset");
            if (!seen.insert(block.target_var).second) fail("'$" + block.target_var + "' has more than one block");

            AssetTimeline& asset = timeline_.assets[it->second];
            asset.info.placed = true;

            std::vector<const ActionItem*> flat;
            expand(block.actions, flat, 0);
            applyActions(asset, flat);
        }
    }

    void applyActions(AssetTimeline& asset, const std::vector<const ActionItem*>& actions) {
        std::map<std::string, PropState> states;
        const Operand one = Operand::makeNumber(1.0);

        for (const ActionItem* item : actions) {
            if (std::holds_alternative<InstantAction>(item->action)) {
                const InstantAction& action = std::get<InstantAction>(item->action);
                RawValue value;
                value.is_vector = action.is_vector;
                value.operand = action.val;
                value.vector = action.vec_val;
                addAction(asset, states, action.property_name, action.time, action.time, true, false, nullptr, value,
                          EasingType::Linear, one);
            } else {
                const TransitionAction& action = std::get<TransitionAction>(item->action);
                const Ramp& ramp = action.ramp;
                RawValue from;
                from.is_vector = ramp.start_is_vec;
                from.operand = ramp.start_val;
                from.vector = ramp.start_vec;
                RawValue to;
                to.is_vector = ramp.end_is_vec;
                to.operand = ramp.end_val;
                to.vector = ramp.end_vec;
                addAction(asset, states, action.property_name, action.start_time, action.end_time, false, ramp.is_pass,
                          &from, to, ramp.easing, ramp.strength);
            }
        }
    }

    void addAction(AssetTimeline& asset, std::map<std::string, PropState>& states, const std::string& property,
                   const Operand& startTime, const Operand& endTime, bool instant, bool pass, const RawValue* from,
                   const RawValue& to, EasingType easing, const Operand& strength) {
        try {
            auto trackIt = asset.tracks.find(property);
            if (trackIt == asset.tracks.end()) fail("property is not valid for this asset");
            PropTrack& track = trackIt->second;

            const long long startCs = timeToCs(startTime);
            const long long endCs = instant ? startCs : timeToCs(endTime);
            if (!instant && endCs < startCs) fail("end time is before start time");
            if (!instant && track.instant_only) fail("this property only supports instant actions");

            auto stateIt = states.find(property);
            if (stateIt == states.end()) {
                PropState fresh;
                fresh.last = track.initial;
                stateIt = states.emplace(property, std::move(fresh)).first;
            }
            PropState& state = stateIt->second;

            if (state.have_prev && startCs < state.prev_start_cs) fail("actions are not in chronological order");
            if (state.have_prev && startCs == state.prev_start_cs) fail("two actions change this property at the same time");
            if (state.have_prev && startCs < state.prev_end_cs) fail("overlapping actions at " + formatCs(startCs));

            const PropValue toValue = convert(to, track.type);
            PropValue fromValue = toValue;
            if (!instant) fromValue = pass ? state.last : convert(*from, track.type);

            double strengthValue = 1.0;
            if (!instant && easing != EasingType::Linear) {
                strengthValue = resolveNumber(strength);
                if (strengthValue <= 0.0) fail("easing strength must be positive");
            }

            int startFrame = instant ? frameFloor(startCs) : frameNearest(startCs);
            int endFrame = instant ? startFrame : frameNearest(endCs);
            if (endFrame < startFrame) endFrame = startFrame;

            if (startFrame <= state.prev_end_frame) {
                const int moved = state.prev_end_frame + 1;
                timeline_.warnings.push_back("$" + asset.info.name + "." + property + ": action at " + formatCs(startCs) +
                                             " moved from frame " + std::to_string(startFrame) + " to frame " +
                                             std::to_string(moved));
                startFrame = moved;
                if (endFrame < startFrame) endFrame = startFrame;
            }

            Segment segment;
            segment.start_frame = startFrame;
            segment.end_frame = endFrame;
            segment.to = toValue;
            if (instant || endFrame == startFrame) {
                segment.instant = true;
                segment.end_frame = startFrame;
                segment.from = toValue;
            } else {
                segment.instant = false;
                segment.from = fromValue;
                segment.easing = easing;
                segment.strength = strengthValue;
            }
            track.segments.push_back(std::move(segment));

            state.have_prev = true;
            state.prev_start_cs = startCs;
            state.prev_end_cs = endCs;
            state.prev_end_frame = track.segments.back().end_frame;
            state.last = toValue;
        } catch (const ResolveError& error) {
            fail("$" + asset.info.name + "." + property + ": " + error.what());
        }
    }

    void buildMediaTable(AssetTimeline& asset) {
        const int total = timeline_.total_frames;
        asset.media_time.assign(static_cast<std::size_t>(total), 0.0);
        asset.media_seek.assign(static_cast<std::size_t>(total), 0);

        const PropTrack& speed = asset.tracks.at("speed");
        const PropTrack& pause = asset.tracks.at("pause");
        auto pointerIt = asset.tracks.find("pointer");
        auto replayIt = asset.tracks.find("replay");
        const PropTrack* pointer = pointerIt == asset.tracks.end() ? nullptr : &pointerIt->second;
        const PropTrack* replay = replayIt == asset.tracks.end() ? nullptr : &replayIt->second;

        double position = 0.0;
        for (int frame = 0; frame < total; ++frame) {
            bool seeked = false;
            if (pointer && pointer->startingAt(frame)) {
                position = pointer->number(frame);
                seeked = true;
            }
            if (replay && replay->startingAt(frame) && replay->boolean(frame)) {
                position = pointer ? pointer->number(frame) : 0.0;
                seeked = true;
            }
            asset.media_time[static_cast<std::size_t>(frame)] = position;
            asset.media_seek[static_cast<std::size_t>(frame)] = seeked ? 1 : 0;
            if (!pause.boolean(frame)) position += speed.number(frame) / timeline_.fps;
            if (position < 0.0) position = 0.0;
        }
    }
};

}

Timeline resolve(const ProgramAST& program) {
    return Resolver(program).run();
}
