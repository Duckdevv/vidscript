#include "Media.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
#include <regex>
#include <sstream>

#ifdef _WIN32
#define VS_POPEN _popen
#define VS_PCLOSE _pclose
#define VS_MODE_READ "rb"
#define VS_MODE_WRITE "wb"
#else
#define VS_POPEN popen
#define VS_PCLOSE pclose
#define VS_MODE_READ "r"
#define VS_MODE_WRITE "w"
#endif

namespace {

std::string wrapCommand(const std::string& command) {
#ifdef _WIN32
    return "\"" + command + "\"";
#else
    return command;
#endif
}

bool isAbsolutePath(const std::string& path) {
    if (path.empty()) return false;
    if (path[0] == '/' || path[0] == '\\') return true;
    return path.size() > 1 && path[1] == ':';
}

}

std::string resolveProjectPath(const std::string& projectDir, const std::string& path) {
    if (projectDir.empty() || isAbsolutePath(path)) return path;
    const char last = projectDir.back();
    if (last == '/' || last == '\\') return projectDir + path;
    return projectDir + "/" + path;
}

std::string quoteArg(const std::string& value) {
    return "\"" + value + "\"";
}

std::string formatNumber(double value) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream.setf(std::ios::fixed);
    stream.precision(6);
    stream << value;
    return stream.str();
}

Pipe::~Pipe() {
    close();
}

bool Pipe::openRead(const std::string& command) {
    close();
    file_ = VS_POPEN(wrapCommand(command).c_str(), VS_MODE_READ);
    return file_ != nullptr;
}

bool Pipe::openWrite(const std::string& command) {
    close();
    file_ = VS_POPEN(wrapCommand(command).c_str(), VS_MODE_WRITE);
    return file_ != nullptr;
}

std::size_t Pipe::read(void* buffer, std::size_t bytes) {
    if (!file_) return 0;
    return std::fread(buffer, 1, bytes, file_);
}

bool Pipe::write(const void* buffer, std::size_t bytes) {
    if (!file_) return false;
    return std::fwrite(buffer, 1, bytes, file_) == bytes;
}

int Pipe::close() {
    if (!file_) return 0;
    std::FILE* file = file_;
    file_ = nullptr;
    return VS_PCLOSE(file);
}

MediaInfo probeMedia(const std::string& ffmpeg, const std::string& path, std::string& error) {
    MediaInfo info;
    Pipe pipe;
    const std::string command = quoteArg(ffmpeg) + " -hide_banner -i " + quoteArg(path) + " 2>&1";
    if (!pipe.openRead(command)) {
        error = "cannot start ffmpeg ('" + ffmpeg + "')";
        return info;
    }

    std::string text;
    char buffer[4096];
    std::size_t count = 0;
    while ((count = pipe.read(buffer, sizeof(buffer))) > 0) text.append(buffer, count);
    pipe.close();

    if (text.find("Input #0") == std::string::npos) {
        const bool noSuchFile = text.find("No such file or directory") != std::string::npos && text.find(ffmpeg) != std::string::npos;
        const bool missingTool = text.find("not found") != std::string::npos || text.find("not recognized") != std::string::npos || noSuchFile;
        error = missingTool ? "cannot start ffmpeg ('" + ffmpeg + "')" : "cannot read media file '" + path + "'";
        return info;
    }

    info.valid = true;
    info.has_audio = text.find("Audio:") != std::string::npos;

    std::smatch match;
    const std::regex durationPattern("Duration: (\\d+):(\\d+):(\\d+\\.?\\d*)");
    if (std::regex_search(text, match, durationPattern)) {
        info.duration = std::stod(match[1]) * 3600.0 + std::stod(match[2]) * 60.0 + std::stod(match[3]);
    }

    const std::size_t videoPos = text.find("Video:");
    if (videoPos != std::string::npos) {
        info.has_video = true;
        std::size_t lineEnd = text.find('\n', videoPos);
        if (lineEnd == std::string::npos) lineEnd = text.size();
        const std::string line = text.substr(videoPos, lineEnd - videoPos);
        const std::regex sizePattern("[ ,](\\d{2,5})x(\\d{2,5})[ ,\\[]");
        if (std::regex_search(line, match, sizePattern)) {
            info.width = std::stoi(match[1]);
            info.height = std::stoi(match[2]);
        }
        const std::regex fpsPattern("(\\d+(?:\\.\\d+)?) fps");
        if (std::regex_search(line, match, fpsPattern)) info.fps = std::stod(match[1]);
    }

    return info;
}

bool decodeAudio(const std::string& ffmpeg, const std::string& path, double start, double end, int sampleRate,
                 std::vector<float>& stereo, std::string& error) {
    stereo.clear();
    std::string command = quoteArg(ffmpeg) + " -hide_banner -loglevel error";
    if (start > 0.0) command += " -ss " + formatNumber(start);
    if (end >= 0.0) command += " -t " + formatNumber(std::max(0.0, end - std::max(0.0, start)));
    command += " -i " + quoteArg(path) + " -vn -f f32le -ac 2 -ar " + std::to_string(sampleRate) + " -";

    Pipe pipe;
    if (!pipe.openRead(command)) {
        error = "cannot start ffmpeg ('" + ffmpeg + "')";
        return false;
    }

    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> chunk(1 << 20);
    std::size_t count = 0;
    while ((count = pipe.read(chunk.data(), chunk.size())) > 0) {
        bytes.insert(bytes.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(count));
    }
    pipe.close();

    const std::size_t samples = bytes.size() / sizeof(float);
    stereo.resize(samples - samples % 2);
    if (!stereo.empty()) std::copy(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(stereo.size() * sizeof(float)),
                                   reinterpret_cast<std::uint8_t*>(stereo.data()));
    return true;
}

bool decodeImageRgba(const std::string& ffmpeg, const std::string& path, int& width, int& height,
                     std::vector<std::uint8_t>& rgba, std::string& error) {
    const MediaInfo info = probeMedia(ffmpeg, path, error);
    if (!info.valid) return false;
    if (!info.has_video || info.width <= 0 || info.height <= 0) {
        error = "'" + path + "' is not an image";
        return false;
    }

    width = info.width;
    height = info.height;
    rgba.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);

    const std::string command = quoteArg(ffmpeg) + " -hide_banner -loglevel error -i " + quoteArg(path) +
                                " -frames:v 1 -f rawvideo -pix_fmt rgba -";
    Pipe pipe;
    if (!pipe.openRead(command)) {
        error = "cannot start ffmpeg ('" + ffmpeg + "')";
        return false;
    }
    const std::size_t got = pipe.read(rgba.data(), rgba.size());
    pipe.close();
    if (got != rgba.size()) {
        error = "cannot decode image '" + path + "'";
        return false;
    }
    return true;
}

VideoDecoder::VideoDecoder(std::string ffmpeg, std::string path, double start, double end, int width, int height,
                           double decodeFps)
    : ffmpeg_(std::move(ffmpeg)),
      path_(std::move(path)),
      start_(start),
      end_(end),
      width_(width),
      height_(height),
      fps_(decodeFps),
      frame_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0) {}

VideoDecoder::~VideoDecoder() {
    pipe_.reset();
}

bool VideoDecoder::restart() {
    pipe_.reset(new Pipe());
    std::string command = quoteArg(ffmpeg_) + " -hide_banner -loglevel fatal";
    if (start_ > 0.0) command += " -ss " + formatNumber(start_);
    if (end_ >= 0.0) command += " -t " + formatNumber(std::max(0.0, end_ - std::max(0.0, start_)));
    command += " -i " + quoteArg(path_) + " -an -vf fps=" + formatNumber(fps_) + " -f rawvideo -pix_fmt rgba -";
    index_ = -1;
    eof_ = false;
    return pipe_->openRead(command);
}

bool VideoDecoder::readFrame() {
    return pipe_->read(frame_.data(), frame_.size()) == frame_.size();
}

bool VideoDecoder::frameAt(double time, const std::uint8_t** data, long long* index) {
    long long target = static_cast<long long>(std::floor(time * fps_ + 1e-6));
    if (target < 0) target = 0;

    if (!pipe_ || target < index_) {
        if (!restart()) return false;
    }

    while (index_ < target && !eof_) {
        if (!readFrame()) {
            eof_ = true;
            break;
        }
        ++index_;
    }

    if (index_ < 0) return false;
    *data = frame_.data();
    *index = index_;
    return true;
}
