#pragma once
namespace drumbox_core {

// Soft clip (approx tanh) : musical, stable, rapide
inline float softClip(float x) {
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

} // namespace drumbox_core
