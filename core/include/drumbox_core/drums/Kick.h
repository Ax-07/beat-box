#pragma once
#include "drumbox_core/Types.h"
#include "drumbox_core/dsp/EnvelopeExp.h"
#include <cmath>

namespace drumbox_core {

struct Kick {
    bool active = false;
    float phase = 0.0f;

    EnvelopeExp ampEnv;
    EnvelopeExp pitchEnv;

    float baseFreq = 55.0f;     // Hz (fin)
    float attackFreq = 120.0f;  // Hz (début)

    void prepare(double /*sr*/) {
        ampEnv.setDecay(0.9995f);
        pitchEnv.setDecay(0.995f);
    }

    void trigger(float vel) {
        active = true;
        phase = 0.0f;
        ampEnv.trigger(vel);
        pitchEnv.trigger(1.0f);
    }

    float process(float sr) {
        if (!active) return 0.0f;

        const float amp = ampEnv.process();
        const float p = pitchEnv.process(); // 1 -> 0
        const float freq = baseFreq + (attackFreq - baseFreq) * p;

        phase += (2.0f * kPi) * freq / sr;
        if (phase > 2.0f * kPi) phase -= 2.0f * kPi;

        float out = amp * std::sinf(phase);

        if (!ampEnv.isActive()) active = false;
        return out;
    }
};

} // namespace drumbox_core
