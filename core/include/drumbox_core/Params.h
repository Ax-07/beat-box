#pragma once
#include <atomic>

namespace drumbox_core {

struct Params {
    // Global
    std::atomic<float> masterGain { 0.6f };

    // Kick
    std::atomic<float> kickDecay      { 0.9995f };
    std::atomic<float> kickAttackFreq { 120.0f };
    std::atomic<float> kickBaseFreq   { 55.0f };

    // Snare
    std::atomic<float> snareDecay     { 0.9975f };
    std::atomic<float> snareToneFreq  { 180.0f };
    std::atomic<float> snareNoiseMix  { 0.75f };

    // Hat
    std::atomic<float> hatDecay  { 0.96f };
    std::atomic<float> hatCutoff { 7000.0f };
};

} // namespace drumbox_core
