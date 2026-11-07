#include "RaylibCanvas.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "raylib.h"
#include "rlgl.h"

namespace {

const double kPi = 3.14159265358979323846;

const char* kFragmentShader = R"GLSL(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform int uMode;
uniform vec2 uResolution;
uniform int uMaskType;
uniform vec2 uMaskCenter;
uniform vec2 uMaskHalf;
uniform vec2 uMaskCs;
uniform int uMaskInvert;
uniform float uMaskStrength;
uniform int uShapeKind;
uniform vec2 uShapeHalf;
uniform float uShapeRadius;
uniform float uShapeStroke;
uniform vec4 uShapeFill;
uniform vec4 uShapeStrokeColor;
uniform float uShapePad;
uniform float uShapeOpacity;
out vec4 finalColor;

float maskFactor()
{
    if (uMaskType == 0) return 1.0;
    vec2 p = vec2(gl_FragCoord.x, uResolution.y - gl_FragCoord.y) - uMaskCenter;
    vec2 q = vec2(uMaskCs.x * p.x + uMaskCs.y * p.y, -uMaskCs.y * p.x + uMaskCs.x * p.y);
    float d;
    if (uMaskType == 1) {
        vec2 e = abs(q) - uMaskHalf;
        d = max(e.x, e.y);
    } else {
        vec2 h = max(uMaskHalf, vec2(0.000001));
        d = (length(q / h) - 1.0) * min(h.x, h.y);
    }
    float inside = clamp(0.5 - d, 0.0, 1.0);
    if (uMaskInvert != 0) inside = 1.0 - inside;
    return mix(1.0, inside, uMaskStrength);
}

void main()
{
    vec4 color;
    if (uMode == 0) {
        color = texture(texture0, fragTexCoord) * fragColor;
    } else {
        vec2 q = (fragTexCoord - vec2(0.5)) * (2.0 * (uShapeHalf + vec2(uShapePad)));
        float d;
        if (uShapeKind == 1) {
            float r = min(uShapeRadius, min(uShapeHalf.x, uShapeHalf.y));
            vec2 w = abs(q) - uShapeHalf + vec2(r);
            d = length(max(w, vec2(0.0))) + min(max(w.x, w.y), 0.0) - r;
        } else {
            vec2 h = max(uShapeHalf, vec2(0.000001));
            d = (length(q / h) - 1.0) * min(h.x, h.y);
        }
        float outer = clamp(0.5 - d, 0.0, 1.0);
        float inner = 1.0;
        if (uShapeStroke > 0.0) inner = clamp(0.5 - (d + uShapeStroke), 0.0, 1.0);
        vec4 base = mix(uShapeStrokeColor, uShapeFill, inner);
        color = vec4(base.rgb, base.a * outer * uShapeOpacity);
    }
    color.a *= maskFactor();
    finalColor = color;
}
)GLSL";

unsigned char toByte(double value) {
    return static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, value))));
}

Color makeColor(const Rgba& color, double opacity) {
    Color result;
    result.r = toByte(color.r);
    result.g = toByte(color.g);
    result.b = toByte(color.b);
    result.a = toByte(color.a * std::max(0.0, std::min(1.0, opacity)));
    return result;
}

}

struct RaylibCanvas::Impl {
    int width = 0;
    int height = 0;
    bool window = false;
    RenderTexture2D target{};
    Shader shader{};
    Texture2D white{};
    std::vector<Texture2D> textures;
    std::vector<Font> fonts;
    MaskSpec mask;

    int locMode = -1;
    int locResolution = -1;
    int locMaskType = -1;
    int locMaskCenter = -1;
    int locMaskHalf = -1;
    int locMaskCs = -1;
    int locMaskInvert = -1;
    int locMaskStrength = -1;
    int locShapeKind = -1;
    int locShapeHalf = -1;
    int locShapeRadius = -1;
    int locShapeStroke = -1;
    int locShapeFill = -1;
    int locShapeStrokeColor = -1;
    int locShapePad = -1;
    int locShapeOpacity = -1;

    void setInt(int location, int value) { SetShaderValue(shader, location, &value, SHADER_UNIFORM_INT); }
    void setFloat(int location, float value) { SetShaderValue(shader, location, &value, SHADER_UNIFORM_FLOAT); }
    void setVec2(int location, float x, float y) {
        const float value[2] = {x, y};
        SetShaderValue(shader, location, value, SHADER_UNIFORM_VEC2);
    }
    void setVec4(int location, float x, float y, float z, float w) {
        const float value[4] = {x, y, z, w};
        SetShaderValue(shader, location, value, SHADER_UNIFORM_VEC4);
    }

    void applyMask() {
        setVec2(locResolution, static_cast<float>(width), static_cast<float>(height));
        if (!mask.active) {
            setInt(locMaskType, 0);
            return;
        }
        const double angle = mask.rotation * kPi / 180.0;
        setInt(locMaskType, mask.is_rect ? 1 : 2);
        setVec2(locMaskCenter, static_cast<float>(mask.center.x), static_cast<float>(mask.center.y));
        setVec2(locMaskHalf, static_cast<float>(mask.half.x), static_cast<float>(mask.half.y));
        setVec2(locMaskCs, static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)));
        setInt(locMaskInvert, mask.invert ? 1 : 0);
        setFloat(locMaskStrength, static_cast<float>(mask.strength));
    }

    void unload() {
        for (Texture2D& texture : textures) {
            if (texture.id != 0) UnloadTexture(texture);
        }
        textures.clear();
        for (Font& font : fonts) {
            if (font.texture.id != 0) UnloadFont(font);
        }
        fonts.clear();
        if (white.id != 0) UnloadTexture(white);
        white = Texture2D{};
        if (shader.id != 0) UnloadShader(shader);
        shader = Shader{};
        if (target.id != 0) UnloadRenderTexture(target);
        target = RenderTexture2D{};
        if (window) CloseWindow();
        window = false;
    }
};

RaylibCanvas::RaylibCanvas() : impl_(new Impl()) {}

RaylibCanvas::~RaylibCanvas() {
    shutdown();
}

bool RaylibCanvas::init(int width, int height, std::string& error) {
    if (width <= 0 || height <= 0) {
        error = "invalid canvas size";
        return false;
    }
    if (impl_->window) shutdown();

    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(64, 64, "VidScript");
    if (!IsWindowReady()) {
        error = "raylib could not create an OpenGL context";
        return false;
    }
    impl_->window = true;
    impl_->width = width;
    impl_->height = height;

    impl_->target = LoadRenderTexture(width, height);
    if (impl_->target.id == 0) {
        error = "raylib could not create a " + std::to_string(width) + "x" + std::to_string(height) + " render texture";
        impl_->unload();
        return false;
    }
    SetTextureFilter(impl_->target.texture, TEXTURE_FILTER_BILINEAR);

    impl_->shader = LoadShaderFromMemory(nullptr, kFragmentShader);
    if (impl_->shader.id == 0 || impl_->shader.id == rlGetShaderIdDefault()) {
        error = "the VidScript shader failed to compile (see the raylib log above)";
        impl_->shader = Shader{};
        impl_->unload();
        return false;
    }

    Image image = GenImageColor(2, 2, WHITE);
    impl_->white = LoadTextureFromImage(image);
    UnloadImage(image);

    Impl& s = *impl_;
    s.locMode = GetShaderLocation(s.shader, "uMode");
    s.locResolution = GetShaderLocation(s.shader, "uResolution");
    s.locMaskType = GetShaderLocation(s.shader, "uMaskType");
    s.locMaskCenter = GetShaderLocation(s.shader, "uMaskCenter");
    s.locMaskHalf = GetShaderLocation(s.shader, "uMaskHalf");
    s.locMaskCs = GetShaderLocation(s.shader, "uMaskCs");
    s.locMaskInvert = GetShaderLocation(s.shader, "uMaskInvert");
    s.locMaskStrength = GetShaderLocation(s.shader, "uMaskStrength");
    s.locShapeKind = GetShaderLocation(s.shader, "uShapeKind");
    s.locShapeHalf = GetShaderLocation(s.shader, "uShapeHalf");
    s.locShapeRadius = GetShaderLocation(s.shader, "uShapeRadius");
    s.locShapeStroke = GetShaderLocation(s.shader, "uShapeStroke");
    s.locShapeFill = GetShaderLocation(s.shader, "uShapeFill");
    s.locShapeStrokeColor = GetShaderLocation(s.shader, "uShapeStrokeColor");
    s.locShapePad = GetShaderLocation(s.shader, "uShapePad");
    s.locShapeOpacity = GetShaderLocation(s.shader, "uShapeOpacity");
    return true;
}

void RaylibCanvas::shutdown() {
    if (impl_) impl_->unload();
}

int RaylibCanvas::loadImage(const std::string& path, int& width, int& height, std::string& error) {
    Image image = LoadImage(path.c_str());
    if (image.data == nullptr) {
        error = "cannot load image '" + path + "'";
        return -1;
    }
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    width = image.width;
    height = image.height;
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id == 0) {
        error = "cannot upload image '" + path + "' to the GPU";
        return -1;
    }
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    impl_->textures.push_back(texture);
    return static_cast<int>(impl_->textures.size()) - 1;
}

int RaylibCanvas::createImage(int width, int height) {
    Image image = GenImageColor(width, height, BLANK);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id == 0) return -1;
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    impl_->textures.push_back(texture);
    return static_cast<int>(impl_->textures.size()) - 1;
}

void RaylibCanvas::updateImage(int handle, const std::uint8_t* rgba) {
    if (handle < 0 || handle >= static_cast<int>(impl_->textures.size())) return;
    UpdateTexture(impl_->textures[static_cast<std::size_t>(handle)], rgba);
}

int RaylibCanvas::loadFont(const std::string& path, const std::vector<int>& codepoints) {
    std::vector<int> list = codepoints;
    Font font = LoadFontEx(path.c_str(), 128, list.empty() ? nullptr : list.data(), static_cast<int>(list.size()));
    if (font.texture.id == 0 || font.glyphCount == 0) return -1;
    if (font.texture.id == GetFontDefault().texture.id) return -1;
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    impl_->fonts.push_back(font);
    return static_cast<int>(impl_->fonts.size()) - 1;
}

Pt RaylibCanvas::measureText(int font, const std::string& text, double fontSize) {
    const bool custom = font >= 0 && font < static_cast<int>(impl_->fonts.size());
    const Font face = custom ? impl_->fonts[static_cast<std::size_t>(font)] : GetFontDefault();
    const float size = static_cast<float>(fontSize);
    const float spacing = custom ? 0.0f : size / 10.0f;
    const Vector2 measured = MeasureTextEx(face, text.c_str(), size, spacing);
    return Pt{measured.x, measured.y};
}

void RaylibCanvas::beginFrame(const Rgba& clear) {
    BeginTextureMode(impl_->target);
    ClearBackground(makeColor(clear, 1.0));
    impl_->mask = MaskSpec();
}

void RaylibCanvas::setMask(const MaskSpec& mask) {
    impl_->mask = mask;
}

void RaylibCanvas::drawImage(const ImageDraw& draw) {
    if (draw.handle < 0 || draw.handle >= static_cast<int>(impl_->textures.size())) return;
    if (draw.size.x <= 0.0 || draw.size.y <= 0.0) return;
    const Texture2D texture = impl_->textures[static_cast<std::size_t>(draw.handle)];

    const float w = static_cast<float>(draw.size.x);
    const float h = static_cast<float>(draw.size.y);
    const Rectangle source = {0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    const Rectangle dest = {static_cast<float>(draw.center.x), static_cast<float>(draw.center.y), w, h};
    const Vector2 origin = {w / 2.0f, h / 2.0f};
    Color tint = WHITE;
    tint.a = toByte(255.0 * std::max(0.0, std::min(1.0, draw.opacity)));

    BeginShaderMode(impl_->shader);
    impl_->setInt(impl_->locMode, 0);
    impl_->applyMask();
    DrawTexturePro(texture, source, dest, origin, static_cast<float>(draw.rotation), tint);
    EndShaderMode();
}

void RaylibCanvas::drawShape(const ShapeDraw& draw) {
    if (draw.size.x <= 0.0 || draw.size.y <= 0.0) return;

    const float pad = 2.0f;
    const float w = static_cast<float>(draw.size.x) + 2.0f * pad;
    const float h = static_cast<float>(draw.size.y) + 2.0f * pad;
    const Rectangle source = {0.0f, 0.0f, static_cast<float>(impl_->white.width), static_cast<float>(impl_->white.height)};
    const Rectangle dest = {static_cast<float>(draw.center.x), static_cast<float>(draw.center.y), w, h};
    const Vector2 origin = {w / 2.0f, h / 2.0f};

    BeginShaderMode(impl_->shader);
    Impl& s = *impl_;
    s.setInt(s.locMode, 1);
    s.applyMask();
    s.setInt(s.locShapeKind, draw.is_rect ? 1 : 2);
    s.setVec2(s.locShapeHalf, static_cast<float>(draw.size.x / 2.0), static_cast<float>(draw.size.y / 2.0));
    s.setFloat(s.locShapeRadius, static_cast<float>(std::max(0.0, draw.corner_radius)));
    s.setFloat(s.locShapeStroke, static_cast<float>(std::max(0.0, draw.stroke_width)));
    s.setVec4(s.locShapeFill, static_cast<float>(draw.fill.r / 255.0), static_cast<float>(draw.fill.g / 255.0),
              static_cast<float>(draw.fill.b / 255.0), static_cast<float>(draw.fill.a / 255.0));
    s.setVec4(s.locShapeStrokeColor, static_cast<float>(draw.stroke.r / 255.0), static_cast<float>(draw.stroke.g / 255.0),
              static_cast<float>(draw.stroke.b / 255.0), static_cast<float>(draw.stroke.a / 255.0));
    s.setFloat(s.locShapePad, pad);
    s.setFloat(s.locShapeOpacity, static_cast<float>(std::max(0.0, std::min(1.0, draw.opacity))));
    DrawTexturePro(impl_->white, source, dest, origin, static_cast<float>(draw.rotation), WHITE);
    EndShaderMode();
}

void RaylibCanvas::drawText(const TextDraw& draw) {
    if (draw.content.empty() || draw.font_size <= 0.0) return;

    const bool custom = draw.font >= 0 && draw.font < static_cast<int>(impl_->fonts.size());
    const Font face = custom ? impl_->fonts[static_cast<std::size_t>(draw.font)] : GetFontDefault();
    const float size = static_cast<float>(draw.font_size);
    const float spacing = custom ? 0.0f : size / 10.0f;
    const Vector2 measured = MeasureTextEx(face, draw.content.c_str(), size, spacing);
    const Vector2 origin = {measured.x / 2.0f, measured.y / 2.0f};
    const float rotation = static_cast<float>(draw.rotation);

    BeginShaderMode(impl_->shader);
    impl_->setInt(impl_->locMode, 0);
    impl_->applyMask();

    if (draw.stroke_width > 0.0) {
        const Color stroke = makeColor(draw.stroke, draw.opacity);
        const double reach = draw.stroke_width;
        for (int k = 0; k < 16; ++k) {
            const double angle = 2.0 * kPi * k / 16.0;
            const Vector2 position = {static_cast<float>(draw.center.x + std::cos(angle) * reach),
                                      static_cast<float>(draw.center.y + std::sin(angle) * reach)};
            DrawTextPro(face, draw.content.c_str(), position, origin, rotation, size, spacing, stroke);
        }
    }

    const Vector2 position = {static_cast<float>(draw.center.x), static_cast<float>(draw.center.y)};
    DrawTextPro(face, draw.content.c_str(), position, origin, rotation, size, spacing, makeColor(draw.fill, draw.opacity));
    EndShaderMode();
}

void RaylibCanvas::fillScreen(const Rgba& color) {
    DrawRectangle(0, 0, impl_->width, impl_->height, makeColor(color, 1.0));
}

void RaylibCanvas::endFrame(std::vector<std::uint8_t>& rgba) {
    EndTextureMode();

    Image image = LoadImageFromTexture(impl_->target.texture);
    ImageFlipVertical(&image);
    if (image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    const std::size_t bytes = static_cast<std::size_t>(impl_->width) * static_cast<std::size_t>(impl_->height) * 4;
    rgba.resize(bytes);
    std::memcpy(rgba.data(), image.data, bytes);
    UnloadImage(image);
}
