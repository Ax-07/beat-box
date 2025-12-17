// Drumbox/core/include/drumbox_core/dsp/FxSection.h

#pragma once
#include "drumbox_core/dsp/AllpassDelay.h"
#include "drumbox_core/dsp/EnvelopeADExp.h"
#include "drumbox_core/dsp/FreqShifter.h"
#include "drumbox_core/dsp/OnePole.h"
#include "drumbox_core/dsp/Ott3Band.h"
#include "drumbox_core/dsp/Saturation.h"

#include <algorithm>
#include <cmath>

namespace drumbox_core {

// FX section comprenant :
// - Disperse: petit diffuseur all-pass (donne du smear/transient spread)
// - Inflator: drive + soft clip + mix
struct FxSection
{
    void prepare(float sampleRate)
    {
        sampleRate_ = (sampleRate > 8000.0f) ? sampleRate : 8000.0f;
        shifter_.prepare(sampleRate);
        ott_.prepare(sampleRate);
        reset();
    }

    void reset()
    {
        apL0.reset(); apL1.reset(); apL2.reset(); apL3.reset();
        apR0.reset(); apR1.reset(); apR2.reset(); apR3.reset();
        shifter_.reset();
        env_.value = 0.0f;
        env_.stage = EnvelopeADExp::Off;
        envVel_ = 1.0f;
        toneLP_L.reset();
        toneLP_R.reset();
        ott_.reset();
    }

    void setShiftHz(float hz)
    {
        shiftHz_ = std::clamp(hz, -2000.0f, 2000.0f);
        shifter_.setShiftHz(shiftHz_);
    }

    void setStereo(float amount)
    {
        stereo_ = clamp01(amount);
    }

    void setDiffusion(float amount)
    {
        diffusion_ = clamp01(amount);
        const float fb = std::clamp(diffusion_ * 0.85f, 0.0f, 0.85f);
        apL0.feedback = fb; apL1.feedback = fb; apL2.feedback = fb; apL3.feedback = fb;
        apR0.feedback = fb; apR1.feedback = fb; apR2.feedback = fb; apR3.feedback = fb;
    }

    void setCleanDirty(float amount)
    {
        cleanDirty_ = clamp01(amount);
    }

    void setTone(float amount)
    {
        tone_ = clamp01(amount);

        // 0=dark => cutoff bas, 1=bright => cutoff haut
        float cutoff = 700.0f + tone_ * 17000.0f;
        const float nyq = 0.5f * sampleRate_;
        if (cutoff > nyq * 0.45f)
            cutoff = nyq * 0.45f;

        toneLP_L.setCutoff(cutoff, sampleRate_);
        toneLP_R.setCutoff(cutoff, sampleRate_);
    }

    void setEnv(float attackCoeff, float decayCoeff, float vol)
    {
        env_.setAttack(std::clamp(attackCoeff, 0.0f, 0.999999f));
        env_.setDecay(std::clamp(decayCoeff, 0.0f, 0.999999f));
        envVol_ = clamp01(vol);
    }

    void triggerEnv(float velocity01)
    {
        envVel_ = clamp01(velocity01);
        env_.trigger(0.0f);
    }

    void setDisperse(float amount)
    {
        disperseMix_ = clamp01(amount);
    }

    void setInflator(float amount, float mix)
    {
        inflatorAmt_ = clamp01(amount);
        inflatorMix_ = clamp01(mix);
    }

    void setOtt(float amount)
    {
        ottAmount_ = clamp01(amount);
        ott_.setParams(ottAmount_);
    }

    inline void process(float inL, float inR, float& outL, float& outR)
    {
        const float dryL = inL;
        const float dryR = inR;

        float xL = inL;
        float xR = inR;

        // Frequency shifter (SSB-ish)
        if (std::abs(shiftHz_) > 0.001f)
        {
            xL = shifter_.process(xL);
            xR = shifter_.process(xR);
        }

        // Stereo width (mid/side). Amount 0 = no-op.
        if (stereo_ > 0.0001f)
        {
            const float mid = 0.5f * (xL + xR);
            float side = 0.5f * (xL - xR);

            // width goes from 1.0 to 2.0 (plus stable)
            const float width = 1.0f + stereo_ * 1.0f;
            side *= width;

            const float wL = mid + side;
            const float wR = mid - side;

            // blend to avoid huge level jumps
            xL = xL * (1.0f - stereo_) + wL * stereo_;
            xR = xR * (1.0f - stereo_) + wR * stereo_;
        }

        // Disperse (wet only)
        float dL = xL;
        float dR = xR;
        if (disperseMix_ > 0.0001f)
        {
            dL = apL3.process(apL2.process(apL1.process(apL0.process(dL))));
            dR = apR3.process(apR2.process(apR1.process(apR0.process(dR))));

            // crossfade dry/wet
            xL = xL * (1.0f - disperseMix_) + dL * disperseMix_;
            xR = xR * (1.0f - disperseMix_) + dR * disperseMix_;
        }

        // Inflator (drive + soft clip + mix)
        if (inflatorAmt_ > 0.0001f)
        {
            const float drive = 1.0f + inflatorAmt_ * 12.0f;
            const float yL = softClip(xL * drive);
            const float yR = softClip(xR * drive);
            const float m = inflatorMix_;
            xL = xL * (1.0f - m) + yL * m;
            xR = xR * (1.0f - m) + yR * m;
        }

        // OTT (3 bandes)
        if (ottAmount_ > 0.0001f)
        {
            float oL = 0.0f;
            float oR = 0.0f;
            ott_.process(xL, xR, oL, oR);
            xL = oL;
            xR = oR;
        }

        // Envelope transient sur la voie FX (gain 1..(1+envVol))
        if (envVol_ > 0.0001f && env_.isActive())
        {
            const float e = std::clamp(env_.process(), 0.0f, 1.0f);
            const float g = 1.0f + envVol_ * envVel_ * e;
            xL *= g;
            xR *= g;
        }

        // Tone (LP) en fin de chaîne FX
        if (tone_ < 0.999f)
        {
            xL = toneLP_L.process(xL);
            xR = toneLP_R.process(xR);
        }

        // Clean/Dirty mix: 0=clean (bypass FX), 1=dirty (full FX)
        const float m = cleanDirty_;
        outL = dryL * (1.0f - m) + xL * m;
        outR = dryR * (1.0f - m) + xR * m;
    }

private:
    static inline float clamp01(float v) { return (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v); }

    // Delays fixes (pas d’alloc). Valeurs différentes L/R pour élargir.
    AllpassDelay<113> apL0{};
    AllpassDelay<151> apL1{};
    AllpassDelay<197> apL2{};
    AllpassDelay<269> apL3{};

    AllpassDelay<127> apR0{};
    AllpassDelay<163> apR1{};
    AllpassDelay<211> apR2{};
    AllpassDelay<281> apR3{};

    FreqShifter shifter_{};
    OnePoleLP toneLP_L{};
    OnePoleLP toneLP_R{};

    Ott3Band ott_{};

    float sampleRate_ = 48000.0f;
    float shiftHz_ = 0.0f;
    float stereo_ = 0.0f;
    float diffusion_ = 0.0f;
    float cleanDirty_ = 1.0f;
    float tone_ = 0.5f;
    EnvelopeADExp env_{};
    float envVol_ = 0.0f;
    float envVel_ = 1.0f;
    float disperseMix_ = 0.0f;
    float inflatorAmt_ = 0.0f;
    float inflatorMix_ = 0.5f;
    float ottAmount_ = 0.0f;
};

} // namespace drumbox_core
