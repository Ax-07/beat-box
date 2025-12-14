#pragma once
#include "drumbox_core/Types.h"

namespace drumbox_core {

struct Transport {
    float bpm = 120.0f;
    bool playing = true;

    double sampleRate = 48000.0;
    u64 currentFrame = 0;

    int stepIndex = 0;
    double nextStepFrame = 0.0;

    void prepare(double sr) {
        sampleRate = sr;
        reset();
    }

    void reset() {
        currentFrame = 0;
        stepIndex = 0;
        nextStepFrame = 0.0;
    }

    // 16 steps/bar en 4/4 -> 4 steps par noire
    double framesPerStep() const {
        const double secondsPerBeat = 60.0 / (double)bpm;
        const double stepsPerBeat = 4.0;
        return sampleRate * secondsPerBeat / stepsPerBeat;
    }
};

} // namespace drumbox_core
