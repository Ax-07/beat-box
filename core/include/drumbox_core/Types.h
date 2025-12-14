#pragma once
#include <cstdint>

namespace drumbox_core {

using i32 = int32_t;
using u32 = uint32_t;
using u64 = uint64_t;

constexpr int kLanes = 3;     // Kick, Snare, Hat
constexpr int kSteps = 16;    // 16-step sequencer
constexpr float kPi = 3.14159265358979323846f;

} // namespace drumbox_core
