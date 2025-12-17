// Drumbox/core/include/drumbox_core/dsp/AllpassDelay.h

#pragma once

namespace drumbox_core {

template <int N>
struct AllpassDelay
{
    float buf[N]{};
    int idx = 0;
    float feedback = 0.5f; // 0..0.9 typique

    void reset()
    {
        for (int i = 0; i < N; ++i) buf[i] = 0.0f;
        idx = 0;
    }

    inline float process(float x)
    {
        const float b = buf[idx];
        const float y = -x + b;
        buf[idx] = x + b * feedback;
        if (++idx >= N) idx = 0;
        return y;
    }
};

} // namespace drumbox_core
