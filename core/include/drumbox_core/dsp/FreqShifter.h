// Drumbox/core/include/drumbox_core/dsp/FreqShifter.h

#pragma once

#include <algorithm>
#include <cmath>

namespace drumbox_core {

// Frequency shifter (single-sideband) using a small Hilbert transformer approximation.
// Portable, no allocations, intended as a character FX (not a perfect shifter).
struct FreqShifter
{
    void prepare(float sampleRate)
    {
        sampleRate_ = (sampleRate > 8000.0f) ? sampleRate : 8000.0f;
        reset();
    }

    void reset()
    {
        phase_ = 0.0f;
        for (auto& ap : iPath_) ap.reset();
        for (auto& ap : qPath_) ap.reset();
        delay_ = 0.0f;
    }

    void setShiftHz(float hz)
    {
        // Clamp to something sane to avoid extreme modulation.
        shiftHz_ = std::clamp(hz, -2000.0f, 2000.0f);
    }

    inline float process(float x)
    {
        // 4-stage allpass Hilbert approximation (fixed coefficients).
        float i = x;
        i = iPath_[0].process(i);
        i = iPath_[1].process(i);
        i = iPath_[2].process(i);
        i = iPath_[3].process(i);

        float q = x;
        q = qPath_[0].process(q);
        q = qPath_[1].process(q);
        q = qPath_[2].process(q);
        q = qPath_[3].process(q);

        // Align the paths a bit (1-sample delay on I path).
        const float iAligned = delay_;
        delay_ = i;

        const float hz = shiftHz_;
        const float w = (std::abs(hz) / sampleRate_) * (2.0f * 3.14159265358979323846f);

        phase_ += w;
        if (phase_ > 2.0f * 3.14159265358979323846f)
            phase_ -= 2.0f * 3.14159265358979323846f;

        const float c = std::cos(phase_);
        const float s = std::sin(phase_);

        // Upper vs lower sideband selection via sign.
        // +hz: y = I*cos - Q*sin
        // -hz: y = I*cos + Q*sin
        return (hz >= 0.0f) ? (iAligned * c - q * s) : (iAligned * c + q * s);
    }

private:
    struct Allpass1
    {
        // Coefficients chosen to be stable and give a usable quadrature approximation.
        // (Fixed set; good enough for character FX.)
        float a = 0.0f;
        float z = 0.0f;

        inline void reset() { z = 0.0f; }

        inline float process(float x)
        {
            const float y = z - a * x;
            z = x + a * y;
            return y;
        }
    };

    // Coeff sets for a compact Hilbert approximation (4 stages per path).
    // These are generic values used in many lightweight implementations.
    Allpass1 iPath_[4] = { {0.041666667f, 0.0f}, {0.138888889f, 0.0f}, {0.333333333f, 0.0f}, {0.666666667f, 0.0f} };
    Allpass1 qPath_[4] = { {0.090909091f, 0.0f}, {0.230769231f, 0.0f}, {0.500000000f, 0.0f}, {0.818181818f, 0.0f} };

    float sampleRate_ = 48000.0f;
    float shiftHz_ = 0.0f;
    float phase_ = 0.0f;
    float delay_ = 0.0f;
};

} // namespace drumbox_core
