#ifndef RAYLIBCANVAS_HPP
#define RAYLIBCANVAS_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Canvas.hpp"

class RaylibCanvas : public Canvas {
public:
    RaylibCanvas();
    ~RaylibCanvas() override;

    RaylibCanvas(const RaylibCanvas&) = delete;
    RaylibCanvas& operator=(const RaylibCanvas&) = delete;

    bool init(int width, int height, std::string& error) override;
    void shutdown() override;

    int loadImage(const std::string& path, int& width, int& height, std::string& error) override;
    int createImage(int width, int height) override;
    void updateImage(int handle, const std::uint8_t* rgba) override;
    int loadFont(const std::string& path, const std::vector<int>& codepoints) override;
    Pt measureText(int font, const std::string& text, double fontSize) override;

    void beginFrame(const Rgba& clear) override;
    void setMask(const MaskSpec& mask) override;
    void drawImage(const VidImageDraw& draw) override;
    void drawShape(const ShapeDraw& draw) override;
    void drawText(const TextDraw& draw) override;
    void fillScreen(const Rgba& color) override;
    void endFrame(std::vector<std::uint8_t>& rgba) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif
