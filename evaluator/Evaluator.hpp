#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "AST.hpp"
#include "FrameState.hpp"
#include "Resolver.hpp"
#include "Timeline.hpp"

class Evaluator {
public:
    explicit Evaluator(Timeline timeline);
    explicit Evaluator(const ProgramAST& program);

    Evaluator(const Evaluator&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;
    Evaluator(Evaluator&&) = default;
    Evaluator& operator=(Evaluator&&) = default;

    FrameState evaluate(int frame) const;

    int totalFrames() const { return timeline_.total_frames; }
    double fps() const { return timeline_.fps; }
    int width() const { return timeline_.width; }
    int height() const { return timeline_.height; }
    const Timeline& timeline() const { return timeline_; }

private:
    Timeline timeline_;

    AssetState stateOf(const AssetTimeline& asset, int frame) const;
    CameraState cameraOf(const AssetTimeline& asset, int frame) const;
};

#endif
