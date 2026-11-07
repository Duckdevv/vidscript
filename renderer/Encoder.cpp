#include "Encoder.hpp"

#include <algorithm>
#include <cctype>
#include <csignal>

namespace {

std::string extensionOf(const std::string& path) {
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return std::string();
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

}

void Encoder::start(const EncoderOptions& options) {
#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
#endif
    if (options.width <= 0 || options.height <= 0) throw RenderError("invalid output resolution");
    if (options.output_path.empty()) throw RenderError("no output file was given");

    const std::string ext = extensionOf(options.output_path);
    const bool webm = ext == "webm";
    const bool hasAudio = !options.audio_path.empty();

    std::string command = quoteArg(options.ffmpeg_path) + " -y -hide_banner -loglevel error";
    command += " -f rawvideo -pix_fmt rgba -s " + std::to_string(options.width) + "x" + std::to_string(options.height);
    command += " -framerate " + formatNumber(options.fps) + " -i -";
    if (hasAudio) command += " -i " + quoteArg(options.audio_path);

    if (webm) {
        command += " -c:v libvpx-vp9 -crf " + std::to_string(options.crf + 12) + " -b:v 0 -pix_fmt yuv420p";
    } else {
        command += " -c:v libx264 -preset " + options.preset + " -crf " + std::to_string(options.crf) + " -pix_fmt yuv420p";
        command += " -vf \"pad=ceil(iw/2)*2:ceil(ih/2)*2\"";
        if (ext == "mp4" || ext == "mov" || ext.empty()) command += " -movflags +faststart";
    }

    if (hasAudio) {
        command += webm ? " -c:a libopus -b:a 160k" : " -c:a aac -b:a 192k";
        command += " -shortest";
    }

    command += " " + quoteArg(options.output_path);

    if (!pipe_.openWrite(command)) throw RenderError("cannot start ffmpeg ('" + options.ffmpeg_path + "')");
}

void Encoder::writeFrame(const std::uint8_t* rgba, std::size_t bytes) {
    if (!pipe_.write(rgba, bytes)) throw RenderError("ffmpeg stopped accepting frames");
}

void Encoder::finish() {
    const int status = pipe_.close();
    if (status != 0) throw RenderError("ffmpeg failed while encoding (exit status " + std::to_string(status) + ")");
}
