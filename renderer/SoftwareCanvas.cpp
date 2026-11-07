#include "SoftwareCanvas.hpp"

#include <algorithm>
#include <cmath>

#include "Media.hpp"

namespace {

const double kPi = 3.14159265358979323846;

double clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

}

SoftwareCanvas::SoftwareCanvas(std::string ffmpegPath) : ffmpeg_(std::move(ffmpegPath)) {}

bool SoftwareCanvas::init(int width, int height, std::string& error) {
    if (width <= 0 || height <= 0) {
        error = "invalid canvas size";
        return false;
    }
    width_ = width;
    height_ = height;
    frame_.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
    return true;
}

void SoftwareCanvas::shutdown() {
    images_.clear();
    frame_.clear();
}

int SoftwareCanvas::loadImage(const std::string& path, int& width, int& height, std::string& error) {
    Bitmap bitmap;
    if (!decodeImageRgba(ffmpeg_, path, bitmap.width, bitmap.height, bitmap.pixels, error)) return -1;
    width = bitmap.width;
    height = bitmap.height;
    images_.push_back(std::move(bitmap));
    return static_cast<int>(images_.size()) - 1;
}

int SoftwareCanvas::createImage(int width, int height) {
    Bitmap bitmap;
    bitmap.width = width;
    bitmap.height = height;
    bitmap.pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
    images_.push_back(std::move(bitmap));
    return static_cast<int>(images_.size()) - 1;
}

void SoftwareCanvas::updateImage(int handle, const std::uint8_t* rgba) {
    if (handle < 0 || handle >= static_cast<int>(images_.size())) return;
    Bitmap& bitmap = images_[static_cast<std::size_t>(handle)];
    std::copy(rgba, rgba + bitmap.pixels.size(), bitmap.pixels.begin());
}

int SoftwareCanvas::loadFont(const std::string&, const std::vector<int>&) {
    return -1;
}

Pt SoftwareCanvas::measureText(int, const std::string& text, double fontSize) {
    return Pt{fontSize * 0.55 * static_cast<double>(text.size()), fontSize};
}

void SoftwareCanvas::beginFrame(const Rgba& clear) {
    for (std::size_t i = 0; i < frame_.size(); i += 4) {
        frame_[i] = static_cast<std::uint8_t>(std::lround(clear.r));
        frame_[i + 1] = static_cast<std::uint8_t>(std::lround(clear.g));
        frame_[i + 2] = static_cast<std::uint8_t>(std::lround(clear.b));
        frame_[i + 3] = static_cast<std::uint8_t>(std::lround(clear.a));
    }
    mask_ = MaskSpec();
}

void SoftwareCanvas::setMask(const MaskSpec& mask) {
    mask_ = mask;
}

double SoftwareCanvas::maskFactor(double x, double y) const {
    if (!mask_.active) return 1.0;
    const double angle = mask_.rotation * kPi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    const double px = x - mask_.center.x;
    const double py = y - mask_.center.y;
    const double qx = c * px + s * py;
    const double qy = -s * px + c * py;

    double d;
    if (mask_.is_rect) {
        d = std::max(std::fabs(qx) - mask_.half.x, std::fabs(qy) - mask_.half.y);
    } else {
        const double hx = std::max(mask_.half.x, 1e-6);
        const double hy = std::max(mask_.half.y, 1e-6);
        d = (std::sqrt((qx / hx) * (qx / hx) + (qy / hy) * (qy / hy)) - 1.0) * std::min(hx, hy);
    }
    double inside = clamp01(0.5 - d);
    if (mask_.invert) inside = 1.0 - inside;
    return 1.0 + (inside - 1.0) * clamp01(mask_.strength);
}

void SoftwareCanvas::blend(int x, int y, double r, double g, double b, double alpha) {
    if (alpha <= 0.0) return;
    std::uint8_t* px = &frame_[(static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)) * 4];
    const double da = px[3] / 255.0;
    const double outA = alpha + da * (1.0 - alpha);
    if (outA <= 0.0) return;
    const double inv = 1.0 - alpha;
    px[0] = static_cast<std::uint8_t>(std::lround(std::max(0.0, std::min(255.0, (r * alpha + px[0] * da * inv) / outA))));
    px[1] = static_cast<std::uint8_t>(std::lround(std::max(0.0, std::min(255.0, (g * alpha + px[1] * da * inv) / outA))));
    px[2] = static_cast<std::uint8_t>(std::lround(std::max(0.0, std::min(255.0, (b * alpha + px[2] * da * inv) / outA))));
    px[3] = static_cast<std::uint8_t>(std::lround(outA * 255.0));
}

void SoftwareCanvas::drawImage(const VidImageDraw& draw) {
    if (draw.handle < 0 || draw.handle >= static_cast<int>(images_.size())) return;
    const Bitmap& bitmap = images_[static_cast<std::size_t>(draw.handle)];
    if (draw.size.x <= 0.0 || draw.size.y <= 0.0) return;

    const double angle = draw.rotation * kPi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    const double extentX = std::fabs(c) * draw.size.x / 2.0 + std::fabs(s) * draw.size.y / 2.0 + 1.0;
    const double extentY = std::fabs(s) * draw.size.x / 2.0 + std::fabs(c) * draw.size.y / 2.0 + 1.0;
    const int x0 = std::max(0, static_cast<int>(std::floor(draw.center.x - extentX)));
    const int x1 = std::min(width_ - 1, static_cast<int>(std::ceil(draw.center.x + extentX)));
    const int y0 = std::max(0, static_cast<int>(std::floor(draw.center.y - extentY)));
    const int y1 = std::min(height_ - 1, static_cast<int>(std::ceil(draw.center.y + extentY)));

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const double px = x + 0.5 - draw.center.x;
            const double py = y + 0.5 - draw.center.y;
            const double lx = c * px + s * py;
            const double ly = -s * px + c * py;
            const double u = lx / draw.size.x + 0.5;
            const double v = ly / draw.size.y + 0.5;
            if (u < 0.0 || u >= 1.0 || v < 0.0 || v >= 1.0) continue;

            const double fx = u * bitmap.width - 0.5;
            const double fy = v * bitmap.height - 0.5;
            const int ix0 = std::max(0, std::min(bitmap.width - 1, static_cast<int>(std::floor(fx))));
            const int iy0 = std::max(0, std::min(bitmap.height - 1, static_cast<int>(std::floor(fy))));
            const int ix1 = std::min(bitmap.width - 1, ix0 + 1);
            const int iy1 = std::min(bitmap.height - 1, iy0 + 1);
            const double tx = clamp01(fx - ix0);
            const double ty = clamp01(fy - iy0);

            double rgba[4];
            for (int k = 0; k < 4; ++k) {
                const auto at = [&](int ax, int ay) {
                    return static_cast<double>(bitmap.pixels[(static_cast<std::size_t>(ay) * static_cast<std::size_t>(bitmap.width) + static_cast<std::size_t>(ax)) * 4 + static_cast<std::size_t>(k)]);
                };
                const double top = at(ix0, iy0) + (at(ix1, iy0) - at(ix0, iy0)) * tx;
                const double bottom = at(ix0, iy1) + (at(ix1, iy1) - at(ix0, iy1)) * tx;
                rgba[k] = top + (bottom - top) * ty;
            }

            const double alpha = rgba[3] / 255.0 * clamp01(draw.opacity) * maskFactor(x + 0.5, y + 0.5);
            blend(x, y, rgba[0], rgba[1], rgba[2], alpha);
        }
    }
}

void SoftwareCanvas::drawShape(const ShapeDraw& draw) {
    const double hx = draw.size.x / 2.0;
    const double hy = draw.size.y / 2.0;
    if (hx <= 0.0 || hy <= 0.0) return;

    const double angle = draw.rotation * kPi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    const double extentX = std::fabs(c) * hx + std::fabs(s) * hy + 2.0;
    const double extentY = std::fabs(s) * hx + std::fabs(c) * hy + 2.0;
    const int x0 = std::max(0, static_cast<int>(std::floor(draw.center.x - extentX)));
    const int x1 = std::min(width_ - 1, static_cast<int>(std::ceil(draw.center.x + extentX)));
    const int y0 = std::max(0, static_cast<int>(std::floor(draw.center.y - extentY)));
    const int y1 = std::min(height_ - 1, static_cast<int>(std::ceil(draw.center.y + extentY)));
    const double radius = std::min(draw.corner_radius, std::min(hx, hy));
    const double stroke = std::max(0.0, draw.stroke_width);

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const double px = x + 0.5 - draw.center.x;
            const double py = y + 0.5 - draw.center.y;
            const double qx = c * px + s * py;
            const double qy = -s * px + c * py;

            double d;
            if (draw.is_rect) {
                const double wx = std::fabs(qx) - hx + radius;
                const double wy = std::fabs(qy) - hy + radius;
                const double ox = std::max(wx, 0.0);
                const double oy = std::max(wy, 0.0);
                d = std::sqrt(ox * ox + oy * oy) + std::min(std::max(wx, wy), 0.0) - radius;
            } else {
                d = (std::sqrt((qx / hx) * (qx / hx) + (qy / hy) * (qy / hy)) - 1.0) * std::min(hx, hy);
            }

            const double outer = clamp01(0.5 - d);
            if (outer <= 0.0) continue;
            const double inner = stroke > 0.0 ? clamp01(0.5 - (d + stroke)) : 1.0;

            const double r = draw.stroke.r + (draw.fill.r - draw.stroke.r) * inner;
            const double g = draw.stroke.g + (draw.fill.g - draw.stroke.g) * inner;
            const double b = draw.stroke.b + (draw.fill.b - draw.stroke.b) * inner;
            const double a = (draw.stroke.a + (draw.fill.a - draw.stroke.a) * inner) / 255.0;
            blend(x, y, r, g, b, a * outer * clamp01(draw.opacity) * maskFactor(x + 0.5, y + 0.5));
        }
    }
}

void SoftwareCanvas::drawText(const TextDraw& draw) {
    const Pt size = measureText(draw.font, draw.content, draw.font_size);
    ShapeDraw box;
    box.is_rect = true;
    box.center = draw.center;
    box.size = Pt{size.x, size.y * 0.7};
    box.rotation = draw.rotation;
    box.fill = draw.fill;
    box.stroke = draw.stroke;
    box.stroke_width = 0.0;
    box.opacity = draw.opacity;
    drawShape(box);
}

void SoftwareCanvas::fillScreen(const Rgba& color) {
    const MaskSpec saved = mask_;
    mask_ = MaskSpec();
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) blend(x, y, color.r, color.g, color.b, color.a / 255.0);
    }
    mask_ = saved;
}

void SoftwareCanvas::endFrame(std::vector<std::uint8_t>& rgba) {
    rgba = frame_;
}
