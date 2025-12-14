#pragma once
#include "drumbox_core/Types.h"
#include "drumbox_core/dsp/EnvelopeExp.h"
#include "drumbox_core/dsp/Noise.h"
#include <cmath>

namespace drumbox_core {

struct Snare {
    bool active = false;

    EnvelopeExp ampEnv;
    EnvelopeExp toneEnv;

    Noise noise;
    float tonePhase = 0.0f;
    float toneFreq = 180.0f;  // Hz
    float noiseMix = 0.75f;   // 0..1

    void prepare(double /*sr*/) {
        ampEnv.setDecay(0.9975f);
        toneEnv.setDecay(0.993f);
        noise.seed(0xBEEF1234u);
    }

    void trigger(float vel) {
        active = true;
        ampEnv.trigger(vel);
        toneEnv.trigger(1.0f);
        tonePhase = 0.0f;
    }

    float process(float sr) {
        if (!active) return 0.0f;

        const float amp = ampEnv.process();
        const float t = toneEnv.process();

        tonePhase += (2.0f * kPi) * toneFreq / sr;
        if (tonePhase > 2.0f * kPi) tonePhase -= 2.0f * kPi;

        const float tone = t * std::sinf(tonePhase);
        const float n = noise.white();

        float out = amp * (noiseMix * n + (1.0f - noiseMix) * tone);

        if (!ampEnv.isActive()) active = false;
        return out;
    }
};

} // namespace drumbox_core
