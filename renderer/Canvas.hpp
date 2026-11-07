#ifndef CANVAS_HPP
#define CANVAS_HPP

#include <cstdint>
#include <string>
#include <vector>

struct Pt {
    double x = 0.0;
    double y = 0.0;
};

struct Rgba {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 255.0;
};

struct MaskSpec {
    bool active = false;
    bool is_rect = true;
    Pt center;
    Pt half;
    double rotation = 0.0;
    bool invert = false;
    double strength = 1.0;
};

struct ImageDraw {
    int handle = -1;
    Pt center;
    Pt size;
    double rotation = 0.0;
    double opacity = 1.0;
};

struct ShapeDraw {
    bool is_rect = true;
    Pt center;
    Pt size;
    double rotation = 0.0;
    double corner_radius = 0.0;
    Rgba fill;
    Rgba stroke;
    double stroke_width = 0.0;
    double opacity = 1.0;
};

struct TextDraw {
    int font = -1;
    std::string content;
    Pt center;
    double font_size = 0.0;
    double rotation = 0.0;
    Rgba fill;
    Rgba stroke;
    double stroke_width = 0.0;
    double opacity = 1.0;
};

class Canvas {
public:
    virtual ~Canvas() = default;

    virtual bool init(int width, int height, std::string& error) = 0;
    virtual void shutdown() = 0;

    virtual int loadImage(const std::string& path, int& width, int& height, std::string& error) = 0;
    virtual int createImage(int width, int height) = 0;
    virtual void updateImage(int handle, const std::uint8_t* rgba) = 0;
    virtual int loadFont(const std::string& path, const std::vector<int>& codepoints) = 0;
    virtual Pt measureText(int font, const std::string& text, double fontSize) = 0;

    virtual void beginFrame(const Rgba& clear) = 0;
    virtual void setMask(const MaskSpec& mask) = 0;
    virtual void drawImage(const ImageDraw& draw) = 0;
    virtual void drawShape(const ShapeDraw& draw) = 0;
    virtual void drawText(const TextDraw& draw) = 0;
    virtual void fillScreen(const Rgba& color) = 0;
    virtual void endFrame(std::vector<std::uint8_t>& rgba) = 0;
};

#endif
