// Drumbox/core/include/drumbox_core/dsp/EnvelopeADExp.h

#pragma once

namespace drumbox_core {

// Envelope attaque + decay exponentiels, sans allocation et déterministe.
// attack/decay sont des coefficients (0..1), proches de 1 => long.
struct EnvelopeADExp {
    enum Stage : int { Off = 0, Attack = 1, Decay = 2 };

    float value = 0.0f;
    float attack = 0.0f; // 0 = instant
    float decay = 0.999f;
    Stage stage = Off;

    void trigger(float start = 0.0f) {
        value = start;
        stage = Attack;
    }

    void setAttack(float a) { attack = a; }
    void setDecay(float d) { decay = d; }

    float process(float threshold = 1.0e-4f) {
        if (stage == Off)
            return 0.0f;

        if (stage == Attack)
        {
            // One-pole vers 1.0 : (1 - value) *= attack
            value = 1.0f - (1.0f - value) * attack;
            if ((1.0f - value) <= threshold)
            {
                value = 1.0f;
                stage = Decay;
            }
            return value;
        }

        // Decay
        float out = value;
        value *= decay;
        if (value <= threshold)
        {
            value = 0.0f;
            stage = Off;
        }
        return out;
    }

    bool isActive(float threshold = 1.0e-4f) const {
        return stage != Off && value > threshold;
    }
};

} // namespace drumbox_core
