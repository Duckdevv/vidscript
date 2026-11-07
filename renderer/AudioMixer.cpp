#include "AudioMixer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>

#include "Media.hpp"

namespace {

void writeLe32(std::ofstream& out, std::uint32_t value) {
    const char bytes[4] = {static_cast<char>(value & 0xFF), static_cast<char>((value >> 8) & 0xFF),
                           static_cast<char>((value >> 16) & 0xFF), static_cast<char>((value >> 24) & 0xFF)};
    out.write(bytes, 4);
}

void writeLe16(std::ofstream& out, std::uint16_t value) {
    const char bytes[2] = {static_cast<char>(value & 0xFF), static_cast<char>((value >> 8) & 0xFF)};
    out.write(bytes, 2);
}

void writeWav(const std::string& path, const std::vector<float>& stereo, int sampleRate) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw RenderError("cannot write audio file '" + path + "'");

    const std::uint32_t dataBytes = static_cast<std::uint32_t>(stereo.size() * sizeof(std::int16_t));
    out.write("RIFF", 4);
    writeLe32(out, 36 + dataBytes);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    writeLe32(out, 16);
    writeLe16(out, 1);
    writeLe16(out, 2);
    writeLe32(out, static_cast<std::uint32_t>(sampleRate));
    writeLe32(out, static_cast<std::uint32_t>(sampleRate * 4));
    writeLe16(out, 4);
    writeLe16(out, 16);
    out.write("data", 4);
    writeLe32(out, dataBytes);

    std::vector<std::int16_t> block(4096);
    std::size_t index = 0;
    while (index < stereo.size()) {
        const std::size_t count = std::min(block.size(), stereo.size() - index);
        for (std::size_t i = 0; i < count; ++i) {
            const float clipped = std::max(-1.0f, std::min(1.0f, stereo[index + i]));
            block[i] = static_cast<std::int16_t>(std::lround(clipped * 32767.0f));
        }
        out.write(reinterpret_cast<const char*>(block.data()), static_cast<std::streamsize>(count * sizeof(std::int16_t)));
        index += count;
    }
    if (!out) throw RenderError("cannot write audio file '" + path + "'");
}

}

AudioMixResult mixAudio(const Evaluator& evaluator, const AudioMixOptions& options) {
    AudioMixResult result;
    const Timeline& timeline = evaluator.timeline();
    const int sampleRate = options.sample_rate;
    const int totalFrames = timeline.total_frames;
    const double fps = timeline.fps;
    const long long totalSamples = std::llround(static_cast<double>(totalFrames) * sampleRate / fps);

    std::vector<float> mix(static_cast<std::size_t>(totalSamples) * 2, 0.0f);
    bool pitchWarned = false;

    for (const AssetTimeline& asset : timeline.assets) {
        if (!asset.info.placed) continue;
        if (asset.info.type != AssetType::Audio && asset.info.type != AssetType::Video) continue;

        const std::string path = resolveProjectPath(options.project_dir, asset.info.path);
        std::string error;
        const MediaInfo media = probeMedia(options.ffmpeg_path, path, error);
        if (!media.valid) throw RenderError(error);
        if (!media.has_audio) continue;

        std::vector<float> pcm;
        if (!decodeAudio(options.ffmpeg_path, path, asset.info.take_start, asset.info.take_end, sampleRate, pcm, error)) {
            throw RenderError(error);
        }
        if (pcm.empty()) continue;

        const PropTrack& speed = asset.tracks.at("speed");
        const PropTrack& pause = asset.tracks.at("pause");
        const PropTrack& volume = asset.tracks.at("volume");
        auto muteIt = asset.tracks.find("mute");
        const PropTrack* mute = muteIt == asset.tracks.end() ? nullptr : &muteIt->second;

        auto pitchIt = asset.tracks.find("pitch_scale");
        if (!pitchWarned && pitchIt != asset.tracks.end() && !pitchIt->second.segments.empty()) {
            result.warnings.push_back("pitch_scale is not supported by the audio mixer and is ignored");
            pitchWarned = true;
        }

        const long long pcmFrames = static_cast<long long>(pcm.size() / 2);
        bool contributed = false;

        for (int frame = 0; frame < totalFrames; ++frame) {
            if (pause.boolean(frame)) continue;
            if (mute && mute->boolean(frame)) continue;

            const long long first = std::llround(static_cast<double>(frame) * sampleRate / fps);
            const long long last = std::llround(static_cast<double>(frame + 1) * sampleRate / fps);
            const long long count = last - first;
            if (count <= 0) continue;

            const double gainStart = volume.number(frame);
            const double gainEnd = frame + 1 < totalFrames ? volume.number(frame + 1) : gainStart;
            if (gainStart == 0.0 && gainEnd == 0.0) continue;

            const double position = asset.media_time[static_cast<std::size_t>(frame)];
            const double step = speed.number(frame) / fps;

            for (long long i = 0; i < count; ++i) {
                const long long outIndex = first + i;
                if (outIndex >= totalSamples) break;

                const double t = static_cast<double>(i) / static_cast<double>(count);
                const double source = (position + step * t) * sampleRate;
                const long long base = static_cast<long long>(std::floor(source));
                if (base < 0 || base >= pcmFrames) continue;

                const double frac = source - static_cast<double>(base);
                const std::size_t a = static_cast<std::size_t>(base) * 2;
                const std::size_t b = base + 1 < pcmFrames ? a + 2 : a;
                const double gain = gainStart + (gainEnd - gainStart) * t;

                const double left = pcm[a] + (pcm[b] - pcm[a]) * frac;
                const double right = pcm[a + 1] + (pcm[b + 1] - pcm[a + 1]) * frac;
                mix[static_cast<std::size_t>(outIndex) * 2] += static_cast<float>(left * gain);
                mix[static_cast<std::size_t>(outIndex) * 2 + 1] += static_cast<float>(right * gain);
                contributed = true;
            }
        }

        if (contributed) result.has_audio = true;
    }

    if (result.has_audio) writeWav(options.wav_path, mix, sampleRate);
    return result;
}
