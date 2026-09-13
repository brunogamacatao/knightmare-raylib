#pragma once

#include <random>

// Small shared RNG helper, standing in for libGDX's MathUtils.random()/Math.random() calls
// scattered through the original port (enemy shoot chance, boss retry timers).
namespace Rnd {

inline float value() {
    static std::mt19937 gen{std::random_device{}()};
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(gen);
}

} // namespace Rnd
