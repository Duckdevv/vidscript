#ifndef AUDIOMIXER_HPP
#define AUDIOMIXER_HPP

#include <string>
#include <vector>

#include "Evaluator.hpp"

struct AudioMixOptions {
    std::string ffmpeg_path = "ffmpeg";
    std::string project_dir;
    std::string wav_path;
    int sample_rate = 48000;
};

struct AudioMixResult {
    bool has_audio = false;
    std::vector<std::string> warnings;
};

AudioMixResult mixAudio(const Evaluator& evaluator, const AudioMixOptions& options);

#endif
