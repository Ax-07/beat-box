#pragma once
#include <cmath>

namespace drumbox_core {

struct OnePoleLP {
    float a = 0.0f;
    float z = 0.0f;

    void setCutoff(float cutoffHz, float sampleRate) {
        // simple: a = 1 - exp(-2π fc / sr)
        const float x = -2.0f * 3.14159265358979323846f * cutoffHz / sampleRate;
        a = 1.0f - std::exp(x);
    }

    float process(float in) {
        z += a * (in - z);
        return z;
    }

    void reset() { z = 0.0f; }
};

struct OnePoleHP {
    OnePoleLP lp;
    float process(float in) { return in - lp.process(in); }
    void setCutoff(float cutoffHz, float sr) { lp.setCutoff(cutoffHz, sr); }
    void reset() { lp.reset(); }
};

} // namespace drumbox_core
