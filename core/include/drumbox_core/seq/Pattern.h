#pragma once
#include "drumbox_core/Types.h"

namespace drumbox_core {

struct Step {
    bool  on  = false;
    float vel = 1.0f; // 0..1
};

struct Pattern {
    Step steps[kLanes][kSteps]{};

    void clear() {
        for (int l = 0; l < kLanes; ++l)
            for (int s = 0; s < kSteps; ++s)
                steps[l][s] = Step{};
    }

    void setStep(int lane, int step, bool on, float vel) {
        if (lane < 0 || lane >= kLanes) return;
        if (step < 0 || step >= kSteps) return;
        steps[lane][step].on = on;
        steps[lane][step].vel = vel;
    }

    Step getStep(int lane, int step) const {
        if (lane < 0 || lane >= kLanes) return Step{};
        if (step < 0 || step >= kSteps) return Step{};
        return steps[lane][step];
    }
};

} // namespace drumbox_core
