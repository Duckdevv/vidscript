#ifndef INTERPOLATOR_HPP
#define INTERPOLATOR_HPP

#include <cmath>
#include <variant>

#include "AST.hpp"
#include "FrameState.hpp"

namespace Interpolator {

inline double clamp01(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

inline double ease(EasingType type, double p, double strength) {
    p = clamp01(p);
    const double s = strength > 0.0 ? strength : 1.0;

    switch (type) {
        case EasingType::Linear:
            return p;
        case EasingType::EaseIn:
            return std::pow(p, s);
        case EasingType::EaseOut:
            return 1.0 - std::pow(1.0 - p, s);
        case EasingType::EaseInOut:
            if (p < 0.5) return 0.5 * std::pow(2.0 * p, s);
            return 1.0 - 0.5 * std::pow(2.0 * (1.0 - p), s);
        case EasingType::EaseOutIn:
            if (p < 0.5) return 0.5 * (1.0 - std::pow(1.0 - 2.0 * p, s));
            return 0.5 + 0.5 * std::pow(2.0 * p - 1.0, s);
    }
    return p;
}

inline double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

inline Vec2 lerp(const Vec2& a, const Vec2& b, double t) {
    return Vec2{lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
}

inline Color lerp(const Color& a, const Color& b, double t) {
    return Color{lerp(a.r, b.r, t), lerp(a.g, b.g, t), lerp(a.b, b.b, t), lerp(a.a, b.a, t)};
}

inline PropValue interpolate(const PropValue& from, const PropValue& to, double t) {
    if (std::holds_alternative<double>(from) && std::holds_alternative<double>(to)) {
        return lerp(std::get<double>(from), std::get<double>(to), t);
    }
    if (std::holds_alternative<Vec2>(from) && std::holds_alternative<Vec2>(to)) {
        return lerp(std::get<Vec2>(from), std::get<Vec2>(to), t);
    }
    if (std::holds_alternative<Color>(from) && std::holds_alternative<Color>(to)) {
        return lerp(std::get<Color>(from), std::get<Color>(to), t);
    }
    return to;
}

}

#endif
