#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include "Media.hpp"

struct EncoderOptions {
    std::string ffmpeg_path = "ffmpeg";
    std::string output_path;
    std::string audio_path;
    int width = 0;
    int height = 0;
    double fps = 30.0;
    int crf = 18;
    std::string preset = "medium";
};

class Encoder {
public:
    Encoder() = default;

    void start(const EncoderOptions& options);
    void writeFrame(const std::uint8_t* rgba, std::size_t bytes);
    void finish();
    bool running() const { return pipe_.isOpen(); }

private:
    Pipe pipe_;
};

#endif
