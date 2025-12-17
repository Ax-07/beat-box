// Drumbox/core/include/drumbox_core/dsp/Oversampling2x.h

#pragma once

namespace drumbox_core {

// Oversampling 2x ultra simple (interpolation linéaire + moyenne à la décimation).
// Objectif: réduire l'aliasing des non-linéarités à coût minimal.
struct Oversampling2x
{
    void reset() { prevIn_ = 0.0f; }

    template <typename Fn>
    inline float process(float in, Fn&& processAt2x)
    {
        // 2 sous-échantillons: mid puis end
        const float mid = 0.5f * (prevIn_ + in);
        prevIn_ = in;

        const float y0 = processAt2x(mid);
        const float y1 = processAt2x(in);

        // Décimation simple
        return 0.5f * (y0 + y1);
    }

private:
    float prevIn_ = 0.0f;
};

} // namespace drumbox_core
