#ifndef MEDIA_HPP
#define MEDIA_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class RenderError : public std::runtime_error {
public:
    explicit RenderError(const std::string& message) : std::runtime_error(message) {}
};

struct MediaInfo {
    bool valid = false;
    bool has_video = false;
    bool has_audio = false;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    double duration = 0.0;
};

std::string resolveProjectPath(const std::string& projectDir, const std::string& path);
std::string quoteArg(const std::string& value);
std::string formatNumber(double value);

class Pipe {
public:
    Pipe() = default;
    ~Pipe();

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    bool openRead(const std::string& command);
    bool openWrite(const std::string& command);
    bool isOpen() const { return file_ != nullptr; }
    std::size_t read(void* buffer, std::size_t bytes);
    bool write(const void* buffer, std::size_t bytes);
    int close();

private:
    std::FILE* file_ = nullptr;
};

MediaInfo probeMedia(const std::string& ffmpeg, const std::string& path, std::string& error);

bool decodeAudio(const std::string& ffmpeg, const std::string& path, double start, double end, int sampleRate,
                 std::vector<float>& stereo, std::string& error);

class VideoDecoder {
public:
    VideoDecoder(std::string ffmpeg, std::string path, double start, double end, int width, int height,
                 double decodeFps);
    ~VideoDecoder();

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    bool frameAt(double time, const std::uint8_t** data, long long* index);

private:
    std::string ffmpeg_;
    std::string path_;
    double start_;
    double end_;
    int width_;
    int height_;
    double fps_;
    std::unique_ptr<Pipe> pipe_;
    std::vector<std::uint8_t> frame_;
    long long index_ = -1;
    bool eof_ = false;

    bool restart();
    bool readFrame();
};

bool decodeImageRgba(const std::string& ffmpeg, const std::string& path, int& width, int& height,
                     std::vector<std::uint8_t>& rgba, std::string& error);

#endif
