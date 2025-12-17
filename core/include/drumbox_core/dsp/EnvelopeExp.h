// Drumbox/core/include/drumbox_core/dsp/EnvelopeExp.h

#pragma once
#include <cmath>

namespace drumbox_core {

struct EnvelopeExp {
    float value = 0.0f;
    float decay = 0.999f; // proche de 1 = long

    void trigger(float start = 1.0f) { value = start; }
    void setDecay(float d) { decay = d; }

    float process() {
        float out = value;
        value *= decay;
        return out;
    }

    bool isActive(float threshold = 1e-4f) const { return value > threshold; }
};

} // namespace drumbox_core
