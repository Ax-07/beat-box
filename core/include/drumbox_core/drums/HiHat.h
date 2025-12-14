// DrumBox/core/include/drumbox_core/drums/HiHat.h

#pragma once
#include "drumbox_core/Types.h"
#include "drumbox_core/dsp/EnvelopeExp.h"
#include "drumbox_core/dsp/Noise.h"
#include "drumbox_core/dsp/OnePole.h"

namespace drumbox_core {

struct HiHat {
    bool active = false;
    EnvelopeExp ampEnv;
    Noise noise;
    OnePoleHP hp;

    float cutoff = 7000.0f; // brightness
    float cutoffCached = -1.0f;

    void updateFilterIfNeeded(float sr) {
    if (cutoff != cutoffCached) {
        hp.setCutoff(cutoff, sr);
        cutoffCached = cutoff;
    }
}
    void prepare(double sr) {
        ampEnv.setDecay(0.96f); // très court
        noise.seed(0xCAFE4321u);
        hp.setCutoff(cutoff, (float)sr);
    }

    void trigger(float vel) {
        active = true;
        ampEnv.trigger(vel);
    }

    float process(float /*sr*/) {
        if (!active) return 0.0f;

        const float amp = ampEnv.process();
        float n = noise.white();
        n = hp.process(n);

        float out = amp * n;

        if (!ampEnv.isActive()) active = false;
        return out;
    }
};

} // namespace drumbox_core
