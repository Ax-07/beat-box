// Drumbox/core/include/drumbox_core/dsp/Ott3Band.h

#pragma once
#include "drumbox_core/dsp/OnePole.h"

#include <algorithm>
#include <cmath>

namespace drumbox_core {

// OTT très simplifiée (3 bandes) : split LP/HP + mid résiduel,
// upward/downward "soft" pilotés par un seul Amount.
// Objectif : caractère/présence, pas une OTT clinique.
struct Ott3Band
{
    void prepare(float sampleRate)
    {
        sr_ = (sampleRate > 8000.0f) ? sampleRate : 8000.0f;
        setParams(amount_, attackMs_, releaseMs_);
        reset();
    }

    void reset()
    {
        lowLP_L.reset(); lowLP_R.reset();
        highHP_L.reset(); highHP_R.reset();
        envLow_L = envMid_L = envHigh_L = 0.0f;
        envLow_R = envMid_R = envHigh_R = 0.0f;
    }

    void setParams(float amount, float attackMs = 2.0f, float releaseMs = 60.0f)
    {
        amount_ = std::clamp(amount, 0.0f, 1.0f);
        attackMs_ = std::clamp(attackMs, 0.1f, 50.0f);
        releaseMs_ = std::clamp(releaseMs, 5.0f, 500.0f);

        // split points fixes (simple)
        lowLP_L.setCutoff(180.0f, sr_);
        lowLP_R.setCutoff(180.0f, sr_);
        highHP_L.setCutoff(2600.0f, sr_);
        highHP_R.setCutoff(2600.0f, sr_);

        atkCoeff_ = msToCoeff(attackMs_, sr_);
        relCoeff_ = msToCoeff(releaseMs_, sr_);
    }

    inline void process(float inL, float inR, float& outL, float& outR)
    {
        if (amount_ <= 0.0001f)
        {
            outL = inL;
            outR = inR;
            return;
        }

        // 3-band split
        float lowL = lowLP_L.process(inL);
        float highL = highHP_L.process(inL);
        float midL = inL - lowL - highL;

        float lowR = lowLP_R.process(inR);
        float highR = highHP_R.process(inR);
        float midR = inR - lowR - highR;

        // env followers (per band, per channel)
        envLow_L = follow(envLow_L, std::fabs(lowL));
        envMid_L = follow(envMid_L, std::fabs(midL));
        envHigh_L = follow(envHigh_L, std::fabs(highL));

        envLow_R = follow(envLow_R, std::fabs(lowR));
        envMid_R = follow(envMid_R, std::fabs(midR));
        envHigh_R = follow(envHigh_R, std::fabs(highR));

        // gains (OTT-ish): upward pour les faibles niveaux + downward pour les forts
        const float gLowL = ottGain(envLow_L);
        const float gMidL = ottGain(envMid_L);
        const float gHighL = ottGain(envHigh_L);

        const float gLowR = ottGain(envLow_R);
        const float gMidR = ottGain(envMid_R);
        const float gHighR = ottGain(envHigh_R);

        float yL = lowL * gLowL + midL * gMidL + highL * gHighL;
        float yR = lowR * gLowR + midR * gMidR + highR * gHighR;

        // léger trim pour éviter de gonfler trop
        const float trim = 1.0f - 0.35f * amount_;
        outL = yL * trim;
        outR = yR * trim;
    }

private:
    static inline float msToCoeff(float ms, float sr)
    {
        // coeff pour smoothing: z += (1-a)(x-z) ; a = exp(-1/(tau*sr))
        const float t = (std::max)(0.0001f, ms * 0.001f);
        const float a = std::exp(-1.0f / (t * sr));
        return std::clamp(a, 0.0f, 0.999999f);
    }

    inline float follow(float z, float x)
    {
        const float coeff = (x > z) ? atkCoeff_ : relCoeff_;
        // z = a*z + (1-a)*x
        return z * coeff + x * (1.0f - coeff);
    }

    inline float ottGain(float env) const
    {
        // thresholds fixes (simple) :
        // - upward: pousse les signaux sous -24dB (~0.063)
        // - downward: compresse au-dessus de -9dB (~0.355)
        const float eps = 1.0e-6f;
        const float upT = 0.063f;
        const float downT = 0.355f;

        float gUp = 1.0f;
        if (env < upT)
        {
            const float ratio = upT / (env + eps);
            // limite la remontée
            gUp = 1.0f + amount_ * std::clamp(ratio - 1.0f, 0.0f, 6.0f);
        }

        float gDown = 1.0f;
        if (env > downT)
        {
            const float over = (env - downT) / downT;
            // réduction douce (jusqu'à ~-12dB)
            gDown = 1.0f / (1.0f + amount_ * 2.5f * over);
            gDown = std::clamp(gDown, 0.25f, 1.0f);
        }

        const float g = gUp * gDown;
        return std::clamp(g, 0.25f, 6.0f);
    }

    float sr_ = 48000.0f;
    float amount_ = 0.0f;
    float attackMs_ = 2.0f;
    float releaseMs_ = 60.0f;

    float atkCoeff_ = 0.9f;
    float relCoeff_ = 0.99f;

    OnePoleLP lowLP_L{};
    OnePoleLP lowLP_R{};
    OnePoleHP highHP_L{};
    OnePoleHP highHP_R{};

    float envLow_L = 0.0f, envMid_L = 0.0f, envHigh_L = 0.0f;
    float envLow_R = 0.0f, envMid_R = 0.0f, envHigh_R = 0.0f;
};

} // namespace drumbox_core
