// Drumbox/core/include/drumbox_core/dsp/Lfo.h

#pragma once

#include <cmath>

namespace drumbox_core {

// LFO temps réel, sans allocation, déterministe.
// phase01 est dans [0..1).
struct Lfo {
    float phase01 = 0.0f;

    void reset(float phase = 0.0f) {
        phase01 = phase;
        if (phase01 >= 1.0f) phase01 -= std::floor(phase01);
        if (phase01 < 0.0f)  phase01 -= std::floor(phase01);
    }

    static inline float clampf(float v, float lo, float hi) {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    static inline float wrap01(float p) {
        p -= std::floor(p);
        if (p < 0.0f) p += 1.0f;
        return p;
    }

    static inline float tri01(float p01) {
        // 0..1 -> -1..1
        const float t = wrap01(p01);
        return 4.0f * std::abs(t - 0.5f) - 1.0f;
    }

    float process(float rateHz, float sampleRate, int shape, float pulse01 = 0.5f) {
        const float sr = (sampleRate < 1.0f) ? 1.0f : sampleRate;
        const float r = clampf(rateHz, 0.0f, 200.0f);
        phase01 += r / sr;
        phase01 = wrap01(phase01);

        pulse01 = clampf(pulse01, 0.01f, 0.99f);

        switch (shape) {
            case 1: // triangle
                return tri01(phase01);
            case 2: // square
                return (phase01 < pulse01) ? 1.0f : -1.0f;
            default: // sine
                return std::sinf(2.0f * 3.14159265358979323846f * phase01);
        }
    }
};

} // namespace drumbox_core
