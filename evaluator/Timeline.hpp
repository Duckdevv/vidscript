#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#include <algorithm>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "AST.hpp"
#include "FrameState.hpp"
#include "Interpolator.hpp"

enum class PropType {
    Number,
    Bool,
    Vec2,
    Color,
    String,
    Time
};

struct Segment {
    int start_frame = 0;
    int end_frame = 0;
    bool instant = true;
    PropValue from;
    PropValue to;
    EasingType easing = EasingType::Linear;
    double strength = 1.0;
};

class PropTrack {
public:
    PropType type = PropType::Number;
    bool instant_only = false;
    PropValue initial;
    std::vector<Segment> segments;

    const Segment* segmentAt(int frame) const {
        auto it = std::upper_bound(segments.begin(), segments.end(), frame,
                                   [](int value, const Segment& segment) { return value < segment.start_frame; });
        if (it == segments.begin()) return nullptr;
        return &*(it - 1);
    }

    const Segment* startingAt(int frame) const {
        const Segment* segment = segmentAt(frame);
        if (segment && segment->start_frame == frame) return segment;
        return nullptr;
    }

    PropValue valueAt(int frame) const {
        const Segment* segment = segmentAt(frame);
        if (!segment) return initial;
        if (segment->instant || frame >= segment->end_frame) return segment->to;
        const double p = static_cast<double>(frame - segment->start_frame) /
                         static_cast<double>(segment->end_frame - segment->start_frame);
        return Interpolator::interpolate(segment->from, segment->to,
                                         Interpolator::ease(segment->easing, p, segment->strength));
    }

    double number(int frame) const { return std::get<double>(valueAt(frame)); }
    bool boolean(int frame) const { return std::get<bool>(valueAt(frame)); }
    Vec2 vec(int frame) const { return std::get<Vec2>(valueAt(frame)); }
    Color color(int frame) const { return std::get<Color>(valueAt(frame)); }
    std::string text(int frame) const { return std::get<std::string>(valueAt(frame)); }
};

struct AssetInfo {
    std::string name;
    AssetType type = AssetType::Image;
    bool placed = false;
    std::string path;
    double take_start = 0.0;
    double take_end = -1.0;
    double base_size = 0.0;
    bool is_rect = false;
    Vec2 dimension;
    double radius = 0.0;
    std::string mask;
};

struct AssetTimeline {
    AssetInfo info;
    std::map<std::string, PropTrack> tracks;
    std::vector<double> media_time;
    std::vector<char> media_seek;
};

struct Timeline {
    std::string name;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    int total_frames = 0;
    int camera_index = -1;
    std::vector<AssetTimeline> assets;
    std::vector<std::string> warnings;
};

#endif
