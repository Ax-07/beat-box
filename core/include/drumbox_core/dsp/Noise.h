#pragma once
#include "drumbox_core/Types.h"

namespace drumbox_core {

struct Noise {
    u32 state = 0x12345678u;

    void seed(u32 s) { state = (s == 0 ? 0x12345678u : s); }

    // xorshift32
    u32 nextU32() {
        u32 x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    float white() {
        // [0, 1]
        const float u = (nextU32() & 0x00FFFFFFu) / 16777216.0f;
        // [-1, 1]
        return 2.0f * u - 1.0f;
    }
};

} // namespace drumbox_core
