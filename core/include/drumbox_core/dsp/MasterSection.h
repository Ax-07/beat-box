// Drumbox/core/include/drumbox_core/dsp/MasterSection.h

#pragma once
#include "drumbox_core/dsp/OnePole.h"
#include "drumbox_core/dsp/Saturation.h"

#include <algorithm>
#include <cmath>

namespace drumbox_core {

struct MasterSection
{
    void prepare(float sampleRate)
    {
        sr_ = (sampleRate > 8000.0f) ? sampleRate : 8000.0f;
        updateFilters();
        reset();
    }

    void reset()
    {
        lowLP_L.reset();
        lowLP_R.reset();
        highHP_L.reset();
        highHP_R.reset();
    }

    void setEqDb(float lowDb, float midDb, float highDb)
    {
        lowDb_ = std::clamp(lowDb, -24.0f, 24.0f);
        midDb_ = std::clamp(midDb, -24.0f, 24.0f);
        highDb_ = std::clamp(highDb, -24.0f, 24.0f);

        lowG_ = dbToLin(lowDb_);
        midG_ = dbToLin(midDb_);
        highG_ = dbToLin(highDb_);
    }

    void setClipper(bool enabled, int mode)
    {
        clipOn_ = enabled;
        clipMode_ = mode;
    }

    inline void process(float inL, float inR, float gainLin, float& outL, float& outR)
    {
        // 3-bandes simple via split (LP/HP) + mid résiduel.
        float lowL = lowLP_L.process(inL);
        float highL = highHP_L.process(inL);
        float midL = inL - lowL - highL;

        float lowR = lowLP_R.process(inR);
        float highR = highHP_R.process(inR);
        float midR = inR - lowR - highR;

        float yL = lowL * lowG_ + midL * midG_ + highL * highG_;
        float yR = lowR * lowG_ + midR * midG_ + highR * highG_;

        yL *= gainLin;
        yR *= gainLin;

        if (clipOn_)
        {
            switch (clipMode_)
            {
                default:
                case 0: yL = softClip(yL); yR = softClip(yR); break;
                case 1: yL = hardClip(yL); yR = hardClip(yR); break;
            }
        }

        // safety clamp (évite NaN/inf de polluer)
        yL = std::clamp(yL, -2.0f, 2.0f);
        yR = std::clamp(yR, -2.0f, 2.0f);

        outL = yL;
        outR = yR;
    }

private:
    static inline float dbToLin(float db)
    {
        return std::pow(10.0f, db / 20.0f);
    }

    static inline float hardClip(float x)
    {
        if (x > 1.0f) return 1.0f;
        if (x < -1.0f) return -1.0f;
        return x;
    }

    void updateFilters()
    {
        const float lowHz = 200.0f;
        const float highHz = 3000.0f;

        lowLP_L.setCutoff(lowHz, sr_);
        lowLP_R.setCutoff(lowHz, sr_);
        highHP_L.setCutoff(highHz, sr_);
        highHP_R.setCutoff(highHz, sr_);
    }

    float sr_ = 48000.0f;

    OnePoleLP lowLP_L{};
    OnePoleLP lowLP_R{};
    OnePoleHP highHP_L{};
    OnePoleHP highHP_R{};

    float lowDb_ = 0.0f;
    float midDb_ = 0.0f;
    float highDb_ = 0.0f;

    float lowG_ = 1.0f;
    float midG_ = 1.0f;
    float highG_ = 1.0f;

    bool clipOn_ = true;
    int clipMode_ = 0; // 0=softClip, 1=hard
};

} // namespace drumbox_core
