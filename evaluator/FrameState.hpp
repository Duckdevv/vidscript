#ifndef FRAMESTATE_HPP
#define FRAMESTATE_HPP

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

struct Color {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 255.0;
};

using PropValue = std::variant<double, bool, Vec2, Color, std::string>;

enum class AssetType {
    Image,
    Video,
    Audio,
    Text,
    Shape,
    Mask,
    Camera
};

struct AssetInfo;

struct MediaState {
    double source_time = 0.0;
    double clip_end = -1.0;
    bool playing = true;
    bool seeked = false;
    double speed = 1.0;
    double volume = 1.0;
    double pitch_scale = 1.0;
    bool mute = false;
};

struct AssetState {
    std::string name;
    AssetType type = AssetType::Image;
    const AssetInfo* info = nullptr;
    Vec2 position;
    Vec2 scale{1.0, 1.0};
    double size = 1.0;
    double rotation = 0.0;
    double opacity = 1.0;
    int layer = 0;
    Color color;
    Color stroke_color;
    double stroke_width = 0.0;
    double border_radius = 0.0;
    std::string content;
    std::string font;
    bool invert = false;
    std::string mask;
    MediaState media;

    Vec2 finalScale() const { return Vec2{scale.x * size, scale.y * size}; }
};

struct CameraState {
    bool present = false;
    Vec2 position;
    double zoom = 1.0;
    double rotation = 0.0;
    double opacity = 1.0;
};

struct FrameState {
    int frame = 0;
    double time = 0.0;
    std::vector<AssetState> assets;
    std::vector<std::size_t> draw_order;
    CameraState camera;
};

#endif
