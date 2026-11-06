#include "Evaluator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace {

const PropTrack* findTrack(const AssetTimeline& asset, const char* name) {
    auto it = asset.tracks.find(name);
    return it == asset.tracks.end() ? nullptr : &it->second;
}

bool isVisual(AssetType type) {
    return type == AssetType::Image || type == AssetType::Video || type == AssetType::Text ||
           type == AssetType::Shape;
}

}

Evaluator::Evaluator(Timeline timeline) : timeline_(std::move(timeline)) {}

Evaluator::Evaluator(const ProgramAST& program) : timeline_(resolve(program)) {}

FrameState Evaluator::evaluate(int frame) const {
    if (frame < 0 || frame >= timeline_.total_frames) throw std::out_of_range("frame is outside the project");

    FrameState state;
    state.frame = frame;
    state.time = static_cast<double>(frame) / timeline_.fps;
    state.camera.position = Vec2{timeline_.width / 2.0, timeline_.height / 2.0};

    for (const AssetTimeline& asset : timeline_.assets) {
        if (!asset.info.placed) continue;
        if (asset.info.type == AssetType::Camera) {
            state.camera = cameraOf(asset, frame);
            continue;
        }
        state.assets.push_back(stateOf(asset, frame));
    }

    for (std::size_t i = 0; i < state.assets.size(); ++i) {
        if (isVisual(state.assets[i].type)) state.draw_order.push_back(i);
    }
    std::stable_sort(state.draw_order.begin(), state.draw_order.end(), [&state](std::size_t a, std::size_t b) {
        return state.assets[a].layer < state.assets[b].layer;
    });

    return state;
}

CameraState Evaluator::cameraOf(const AssetTimeline& asset, int frame) const {
    CameraState camera;
    camera.present = true;
    if (const PropTrack* track = findTrack(asset, "position")) camera.position = track->vec(frame);
    if (const PropTrack* track = findTrack(asset, "zoom")) camera.zoom = track->number(frame);
    if (const PropTrack* track = findTrack(asset, "opacity")) camera.opacity = track->number(frame);
    if (const PropTrack* track = findTrack(asset, "rotation")) camera.rotation = track->number(frame);
    return camera;
}

AssetState Evaluator::stateOf(const AssetTimeline& asset, int frame) const {
    AssetState state;
    state.name = asset.info.name;
    state.type = asset.info.type;
    state.info = &asset.info;
    state.mask = asset.info.mask;

    if (const PropTrack* track = findTrack(asset, "position")) state.position = track->vec(frame);
    if (const PropTrack* track = findTrack(asset, "scale")) state.scale = track->vec(frame);
    if (const PropTrack* track = findTrack(asset, "size")) state.size = track->number(frame);
    if (const PropTrack* track = findTrack(asset, "rotation")) state.rotation = track->number(frame);
    if (const PropTrack* track = findTrack(asset, "opacity")) state.opacity = track->number(frame);
    if (const PropTrack* track = findTrack(asset, "layer")) state.layer = static_cast<int>(std::llround(track->number(frame)));
    if (const PropTrack* track = findTrack(asset, "color")) state.color = track->color(frame);
    if (const PropTrack* track = findTrack(asset, "stroke_color")) state.stroke_color = track->color(frame);
    if (const PropTrack* track = findTrack(asset, "stroke_width")) state.stroke_width = track->number(frame);
    if (const PropTrack* track = findTrack(asset, "border_radius")) state.border_radius = track->number(frame);
    if (const PropTrack* track = findTrack(asset, "content")) state.content = track->text(frame);
    if (const PropTrack* track = findTrack(asset, "font")) state.font = track->text(frame);
    if (const PropTrack* track = findTrack(asset, "invert")) state.invert = track->boolean(frame);

    if (asset.info.type == AssetType::Video || asset.info.type == AssetType::Audio) {
        const std::size_t index = static_cast<std::size_t>(frame);
        MediaState& media = state.media;
        media.source_time = asset.info.take_start + asset.media_time[index];
        media.clip_end = asset.info.take_end;
        media.seeked = asset.media_seek[index] != 0;
        if (const PropTrack* track = findTrack(asset, "pause")) media.playing = !track->boolean(frame);
        if (const PropTrack* track = findTrack(asset, "speed")) media.speed = track->number(frame);
        if (const PropTrack* track = findTrack(asset, "volume")) media.volume = track->number(frame);
        if (const PropTrack* track = findTrack(asset, "pitch_scale")) media.pitch_scale = track->number(frame);
        if (const PropTrack* track = findTrack(asset, "mute")) media.mute = track->boolean(frame);
    }

    return state;
}
