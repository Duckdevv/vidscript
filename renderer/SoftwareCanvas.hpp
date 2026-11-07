#ifndef SOFTWARECANVAS_HPP
#define SOFTWARECANVAS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "Canvas.hpp"

class SoftwareCanvas : public Canvas {
public:
    explicit SoftwareCanvas(std::string ffmpegPath = "ffmpeg");

    bool init(int width, int height, std::string& error) override;
    void shutdown() override;

    int loadImage(const std::string& path, int& width, int& height, std::string& error) override;
    int createImage(int width, int height) override;
    void updateImage(int handle, const std::uint8_t* rgba) override;
    int loadFont(const std::string& path, const std::vector<int>& codepoints) override;
    Pt measureText(int font, const std::string& text, double fontSize) override;

    void beginFrame(const Rgba& clear) override;
    void setMask(const MaskSpec& mask) override;
    void drawImage(const ImageDraw& draw) override;
    void drawShape(const ShapeDraw& draw) override;
    void drawText(const TextDraw& draw) override;
    void fillScreen(const Rgba& color) override;
    void endFrame(std::vector<std::uint8_t>& rgba) override;

private:
    struct Bitmap {
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> pixels;
    };

    std::string ffmpeg_;
    int width_ = 0;
    int height_ = 0;
    std::vector<std::uint8_t> frame_;
    std::vector<Bitmap> images_;
    MaskSpec mask_;

    double maskFactor(double x, double y) const;
    void blend(int x, int y, double r, double g, double b, double alpha);
};

#endif
