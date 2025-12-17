// Drumbox/core/include/drumbox_core/dsp/ReverbSchroeder.h

#pragma once
#include <cstdint>
#include <algorithm>

namespace drumbox_core {

// Reverb courte type Schroeder (comb + allpass), buffers fixes, sans allocation.
// Conçue pour une "kick tail" (petites tailles, CPU léger).
struct ReverbSchroeder
{
    void prepare(float sampleRate)
    {
        sr_ = (sampleRate > 8000.0f) ? sampleRate : 8000.0f;
        reset();
    }

    void reset()
    {
        combL0_.reset(); combL1_.reset(); combL2_.reset(); combL3_.reset();
        combR0_.reset(); combR1_.reset(); combR2_.reset(); combR3_.reset();
        apL0_.reset(); apL1_.reset();
        apR0_.reset(); apR1_.reset();
    }

    // amount: 0..1 (wet)
    // size:   0..1 (feedback / "room")
    // tone:   0..1 (0=dark (damp fort), 1=bright (damp faible))
    void setParams(float amount, float size, float tone)
    {
        wet_ = clamp01(amount);
        room_ = std::clamp(size, 0.0f, 1.0f);
        // damp = coeff de lissage du lowpass dans la boucle: plus grand = plus sombre
        // tone=1 => damp faible (bright)
        damp_ = std::clamp(0.05f + (1.0f - tone) * 0.70f, 0.0f, 0.99f);

        // feedback: taille perçue
        feedback_ = std::clamp(0.25f + room_ * 0.65f, 0.0f, 0.98f);
    }

    // Entrée mono, sortie stéréo.
    inline void processMono(float x, float& outL, float& outR)
    {
        // petite pré-atténuation pour éviter de saturer la boucle
        const float in = x * 0.25f;

        float l = 0.0f;
        float r = 0.0f;

        l += combL0_.process(in, feedback_, damp_);
        l += combL1_.process(in, feedback_, damp_);
        l += combL2_.process(in, feedback_, damp_);
        l += combL3_.process(in, feedback_, damp_);

        r += combR0_.process(in, feedback_, damp_);
        r += combR1_.process(in, feedback_, damp_);
        r += combR2_.process(in, feedback_, damp_);
        r += combR3_.process(in, feedback_, damp_);

        l = apL0_.process(l);
        l = apL1_.process(l);
        r = apR0_.process(r);
        r = apR1_.process(r);

        // Normalisation légère (dépend du nb de combs)
        l *= 0.25f;
        r *= 0.25f;

        const float wet = wet_;
        outL = l * wet;
        outR = r * wet;
    }

private:
    static inline float clamp01(float v) { return (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v); }

    template <int N>
    struct Comb
    {
        float buf[N]{};
        int idx = 0;
        float filterStore = 0.0f;

        void reset()
        {
            for (int i = 0; i < N; ++i) buf[i] = 0.0f;
            idx = 0;
            filterStore = 0.0f;
        }

        inline float process(float input, float feedback, float damp)
        {
            const float output = buf[idx];

            // lowpass dans la boucle (freeverb style)
            filterStore = output * (1.0f - damp) + filterStore * damp;
            buf[idx] = input + filterStore * feedback;

            if (++idx >= N) idx = 0;
            return output;
        }
    };

    template <int N>
    struct Allpass
    {
        float buf[N]{};
        int idx = 0;
        float feedback = 0.5f;

        void reset()
        {
            for (int i = 0; i < N; ++i) buf[i] = 0.0f;
            idx = 0;
        }

        inline float process(float input)
        {
            const float bufout = buf[idx];
            const float output = -input + bufout;
            buf[idx] = input + bufout * feedback;

            if (++idx >= N) idx = 0;
            return output;
        }
    };

    // Delays inspirés Freeverb (mais réduits) — valeurs fixes (pas d'allocation).
    // L/R légèrement différents pour élargir la stéréo.
    Comb<1116> combL0_{};
    Comb<1188> combL1_{};
    Comb<1277> combL2_{};
    Comb<1356> combL3_{};

    Comb<1139> combR0_{};
    Comb<1211> combR1_{};
    Comb<1300> combR2_{};
    Comb<1379> combR3_{};

    Allpass<225> apL0_{};
    Allpass<341> apL1_{};
    Allpass<248> apR0_{};
    Allpass<364> apR1_{};

    float sr_ = 48000.0f;
    float wet_ = 0.0f;
    float room_ = 0.5f;
    float feedback_ = 0.6f;
    float damp_ = 0.3f;
};

} // namespace drumbox_core
