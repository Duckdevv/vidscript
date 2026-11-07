#include "Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

#include "AudioMixer.hpp"
#include "Encoder.hpp"

namespace {

const double kPi = 3.14159265358979323846;

Rgba toRgba(const Color& color) {
    return Rgba{color.r, color.g, color.b, color.a};
}

double clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

void appendCodepoints(const std::string& text, std::set<int>& out) {
    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        int length = 1;
        int code = c;
        if (c >= 0xF0) {
            length = 4;
            code = c & 0x07;
        } else if (c >= 0xE0) {
            length = 3;
            code = c & 0x0F;
        } else if (c >= 0xC0) {
            length = 2;
            code = c & 0x1F;
        }
        if (i + static_cast<std::size_t>(length) > text.size()) break;
        for (int k = 1; k < length; ++k) code = (code << 6) | (static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]) & 0x3F);
        out.insert(code);
        i += static_cast<std::size_t>(length);
    }
}

std::vector<std::string> stringValues(const PropTrack& track) {
    std::vector<std::string> values;
    values.push_back(std::get<std::string>(track.initial));
    for (const Segment& segment : track.segments) values.push_back(std::get<std::string>(segment.to));
    return values;
}

}

Renderer::Renderer(const Evaluator& evaluator, Canvas& canvas, RenderOptions options)
    : evaluator_(evaluator), canvas_(canvas), options_(std::move(options)) {}

Renderer::~Renderer() {
    if (prepared_) canvas_.shutdown();
}

void Renderer::loadFonts() {
    const Timeline& timeline = evaluator_.timeline();

    std::set<int> codepoints;
    for (int c = 32; c < 127; ++c) codepoints.insert(c);
    std::set<std::string> paths;

    for (const AssetTimeline& asset : timeline.assets) {
        if (!asset.info.placed || asset.info.type != AssetType::Text) continue;
        auto contentIt = asset.tracks.find("content");
        if (contentIt != asset.tracks.end()) {
            for (const std::string& text : stringValues(contentIt->second)) appendCodepoints(text, codepoints);
        }
        auto fontIt = asset.tracks.find("font");
        if (fontIt != asset.tracks.end()) {
            for (const std::string& path : stringValues(fontIt->second)) paths.insert(path);
        }
    }

    const std::vector<int> list(codepoints.begin(), codepoints.end());
    for (const std::string& path : paths) {
        if (path.empty()) {
            fonts_[path] = -1;
            continue;
        }
        const int handle = canvas_.loadFont(resolveProjectPath(options_.project_dir, path), list);
        if (handle < 0) warnings_.push_back("cannot load font '" + path + "', the default font is used instead");
        fonts_[path] = handle;
    }
}

void Renderer::prepare() {
    if (prepared_) return;

    std::string error;
    if (!canvas_.init(evaluator_.width(), evaluator_.height(), error)) {
        throw RenderError("cannot start the graphics backend: " + error);
    }
    prepared_ = true;

    const Timeline& timeline = evaluator_.timeline();
    loadFonts();

    for (const AssetTimeline& asset : timeline.assets) {
        if (!asset.info.placed) continue;
        const std::string path = resolveProjectPath(options_.project_dir, asset.info.path);

        if (asset.info.type == AssetType::Image) {
            ImageResource resource;
            resource.handle = canvas_.loadImage(path, resource.width, resource.height, error);
            if (resource.handle < 0) throw RenderError("$" + asset.info.name + ": " + error);
            images_[asset.info.name] = resource;
        } else if (asset.info.type == AssetType::Video) {
            const MediaInfo media = probeMedia(options_.ffmpeg_path, path, error);
            if (!media.valid) throw RenderError("$" + asset.info.name + ": " + error);
            if (!media.has_video || media.width <= 0 || media.height <= 0) {
                throw RenderError("$" + asset.info.name + ": '" + path + "' has no video stream");
            }
            VideoResource resource;
            resource.width = media.width;
            resource.height = media.height;
            resource.handle = canvas_.createImage(media.width, media.height);
            resource.decoder.reset(new VideoDecoder(options_.ffmpeg_path, path, asset.info.take_start, asset.info.take_end,
                                                    media.width, media.height, options_.decode_fps));
            videos_[asset.info.name] = std::move(resource);
        }
    }
}

Renderer::View Renderer::makeView(const FrameState& state) const {
    View view;
    view.cx = evaluator_.width() / 2.0;
    view.cy = evaluator_.height() / 2.0;
    view.camera = Pt{state.camera.position.x, state.camera.position.y};
    view.zoom = state.camera.zoom;
    view.rotation = state.camera.rotation;
    view.cos_r = std::cos(view.rotation * kPi / 180.0);
    view.sin_r = std::sin(view.rotation * kPi / 180.0);
    return view;
}

Pt Renderer::project(const View& view, double x, double y) const {
    const double dx = x - view.camera.x;
    const double dy = y - view.camera.y;
    return Pt{view.cx + view.zoom * (dx * view.cos_r - dy * view.sin_r), view.cy + view.zoom * (dx * view.sin_r + dy * view.cos_r)};
}

MaskSpec Renderer::maskFor(const AssetState& asset, const FrameState& state, const View& view) const {
    MaskSpec spec;
    if (asset.mask.empty()) return spec;

    const AssetState* mask = nullptr;
    for (const AssetState& candidate : state.assets) {
        if (candidate.name == asset.mask && candidate.type == AssetType::Mask) {
            mask = &candidate;
            break;
        }
    }
    if (!mask || !mask->info) return spec;

    const Vec2 scale = mask->finalScale();
    spec.active = true;
    spec.is_rect = mask->info->is_rect;
    spec.center = project(view, mask->position.x, mask->position.y);
    if (spec.is_rect) {
        spec.half = Pt{mask->info->dimension.x * scale.x * view.zoom / 2.0, mask->info->dimension.y * scale.y * view.zoom / 2.0};
    } else {
        spec.half = Pt{mask->info->radius * scale.x * view.zoom, mask->info->radius * scale.y * view.zoom};
    }
    spec.rotation = view.rotation;
    spec.invert = mask->invert;
    spec.strength = clamp01(mask->opacity);
    return spec;
}

void Renderer::drawImageAsset(const AssetState& asset, const View& view, double opacity) {
    auto it = images_.find(asset.name);
    if (it == images_.end()) return;
    const Vec2 scale = asset.finalScale();

    ImageDraw draw;
    draw.handle = it->second.handle;
    draw.center = project(view, asset.position.x, asset.position.y);
    draw.size = Pt{it->second.width * scale.x * view.zoom, it->second.height * scale.y * view.zoom};
    draw.rotation = asset.rotation + view.rotation;
    draw.opacity = opacity;
    canvas_.drawImage(draw);
}

void Renderer::drawVideoAsset(const AssetState& asset, const View& view, double opacity) {
    auto it = videos_.find(asset.name);
    if (it == videos_.end() || !asset.info) return;
    VideoResource& video = it->second;

    const std::uint8_t* data = nullptr;
    long long index = 0;
    const double time = std::max(0.0, asset.media.source_time - asset.info->take_start);
    if (!video.decoder->frameAt(time, &data, &index)) return;
    if (index != video.uploaded) {
        canvas_.updateImage(video.handle, data);
        video.uploaded = index;
    }

    const Vec2 scale = asset.finalScale();
    ImageDraw draw;
    draw.handle = video.handle;
    draw.center = project(view, asset.position.x, asset.position.y);
    draw.size = Pt{video.width * scale.x * view.zoom, video.height * scale.y * view.zoom};
    draw.rotation = asset.rotation + view.rotation;
    draw.opacity = opacity;
    canvas_.drawImage(draw);
}

void Renderer::drawShapeAsset(const AssetState& asset, const View& view, double opacity) {
    if (!asset.info) return;
    const Vec2 scale = asset.finalScale();
    const double mean = (scale.x + scale.y) / 2.0;

    ShapeDraw draw;
    draw.is_rect = asset.info->is_rect;
    draw.center = project(view, asset.position.x, asset.position.y);
    if (draw.is_rect) {
        draw.size = Pt{asset.info->dimension.x * scale.x * view.zoom, asset.info->dimension.y * scale.y * view.zoom};
    } else {
        draw.size = Pt{asset.info->radius * 2.0 * scale.x * view.zoom, asset.info->radius * 2.0 * scale.y * view.zoom};
    }
    draw.rotation = asset.rotation + view.rotation;
    draw.corner_radius = asset.border_radius * mean * view.zoom;
    draw.fill = toRgba(asset.color);
    draw.stroke = toRgba(asset.stroke_color);
    draw.stroke_width = asset.stroke_width * mean * view.zoom;
    draw.opacity = opacity;
    canvas_.drawShape(draw);
}

void Renderer::drawTextAsset(const AssetState& asset, const View& view, double opacity) {
    if (!asset.info || asset.content.empty()) return;
    const Vec2 scale = asset.finalScale();
    const double mean = (scale.x + scale.y) / 2.0;

    TextDraw draw;
    auto fontIt = fonts_.find(asset.font);
    draw.font = fontIt == fonts_.end() ? -1 : fontIt->second;
    draw.content = asset.content;
    draw.center = project(view, asset.position.x, asset.position.y);
    draw.font_size = asset.info->base_size * mean * view.zoom;
    draw.rotation = asset.rotation + view.rotation;
    draw.fill = toRgba(asset.color);
    draw.stroke = toRgba(asset.stroke_color);
    draw.stroke_width = asset.stroke_width * mean * view.zoom;
    draw.opacity = opacity;
    if (draw.font_size <= 0.0) return;
    canvas_.drawText(draw);
}

void Renderer::drawAsset(const AssetState& asset, const FrameState& state, const View& view) {
    const double opacity = clamp01(asset.opacity);
    if (opacity <= 0.0) return;

    canvas_.setMask(maskFor(asset, state, view));

    switch (asset.type) {
        case AssetType::Image:
            drawImageAsset(asset, view, opacity);
            break;
        case AssetType::Video:
            drawVideoAsset(asset, view, opacity);
            break;
        case AssetType::Shape:
            drawShapeAsset(asset, view, opacity);
            break;
        case AssetType::Text:
            drawTextAsset(asset, view, opacity);
            break;
        default:
            break;
    }
}

void Renderer::renderFrame(int frame, std::vector<std::uint8_t>& rgba) {
    prepare();

    const FrameState state = evaluator_.evaluate(frame);
    const View view = makeView(state);

    canvas_.beginFrame(Rgba{0.0, 0.0, 0.0, 255.0});
    for (std::size_t index : state.draw_order) drawAsset(state.assets[index], state, view);

    canvas_.setMask(MaskSpec());
    const double camera = clamp01(state.camera.opacity);
    if (camera < 1.0) canvas_.fillScreen(Rgba{0.0, 0.0, 0.0, 255.0 * (1.0 - camera)});

    canvas_.endFrame(rgba);
}

void Renderer::renderToFile() {
    prepare();

    const Timeline& timeline = evaluator_.timeline();
    std::string audioPath;
    std::string wavPath;

    if (options_.render_audio) {
        AudioMixOptions mixOptions;
        mixOptions.ffmpeg_path = options_.ffmpeg_path;
        mixOptions.project_dir = options_.project_dir;
        mixOptions.wav_path = (options_.temp_dir.empty() ? options_.output_path : options_.temp_dir + "/vidscript") + ".audio.wav";
        const AudioMixResult mixed = mixAudio(evaluator_, mixOptions);
        for (const std::string& warning : mixed.warnings) warnings_.push_back(warning);
        if (mixed.has_audio) {
            audioPath = mixOptions.wav_path;
            wavPath = audioPath;
        }
    }

    EncoderOptions encoderOptions;
    encoderOptions.ffmpeg_path = options_.ffmpeg_path;
    encoderOptions.output_path = options_.output_path;
    encoderOptions.audio_path = audioPath;
    encoderOptions.width = timeline.width;
    encoderOptions.height = timeline.height;
    encoderOptions.fps = timeline.fps;
    encoderOptions.crf = options_.crf;
    encoderOptions.preset = options_.preset;

    Encoder encoder;
    try {
        encoder.start(encoderOptions);
        std::vector<std::uint8_t> rgba;
        for (int frame = 0; frame < timeline.total_frames; ++frame) {
            renderFrame(frame, rgba);
            encoder.writeFrame(rgba.data(), rgba.size());
            if (options_.on_progress) options_.on_progress(frame + 1, timeline.total_frames);
        }
        encoder.finish();
    } catch (...) {
        if (!wavPath.empty()) std::remove(wavPath.c_str());
        throw;
    }

    if (!wavPath.empty()) std::remove(wavPath.c_str());
}
