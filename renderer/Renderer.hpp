#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Canvas.hpp"
#include "Evaluator.hpp"
#include "Media.hpp"

struct RenderOptions {
    std::string project_dir;
    std::string ffmpeg_path = "ffmpeg";
    std::string output_path = "output.mp4";
    std::string temp_dir;
    int crf = 18;
    std::string preset = "medium";
    bool render_audio = true;
    double decode_fps = 60.0;
    std::function<void(int, int)> on_progress;
};

class Renderer {
public:
    Renderer(const Evaluator& evaluator, Canvas& canvas, RenderOptions options);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void prepare();
    void renderFrame(int frame, std::vector<std::uint8_t>& rgba);
    void renderToFile();

    const std::vector<std::string>& warnings() const { return warnings_; }

private:
    struct ImageResource {
        int handle = -1;
        int width = 0;
        int height = 0;
    };

    struct VideoResource {
        int handle = -1;
        int width = 0;
        int height = 0;
        long long uploaded = -1;
        std::unique_ptr<VideoDecoder> decoder;
    };

    struct View {
        double cx = 0.0;
        double cy = 0.0;
        Pt camera;
        double zoom = 1.0;
        double rotation = 0.0;
        double cos_r = 1.0;
        double sin_r = 0.0;
    };

    const Evaluator& evaluator_;
    Canvas& canvas_;
    RenderOptions options_;
    bool prepared_ = false;
    std::vector<std::string> warnings_;
    std::unordered_map<std::string, ImageResource> images_;
    std::unordered_map<std::string, VideoResource> videos_;
    std::unordered_map<std::string, int> fonts_;

    View makeView(const FrameState& state) const;
    Pt project(const View& view, double x, double y) const;
    MaskSpec maskFor(const AssetState& asset, const FrameState& state, const View& view) const;
    void drawAsset(const AssetState& asset, const FrameState& state, const View& view);
    void drawImageAsset(const AssetState& asset, const View& view, double opacity);
    void drawVideoAsset(const AssetState& asset, const View& view, double opacity);
    void drawShapeAsset(const AssetState& asset, const View& view, double opacity);
    void drawTextAsset(const AssetState& asset, const View& view, double opacity);
    void loadFonts();
};

#endif
