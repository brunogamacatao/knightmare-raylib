#pragma once

#include <cmath>

// Degree-based trigonometry helpers, mirroring MMBasic's "option angle degrees" mode used
// throughout the source.
namespace Trig {

inline float sin(float deg) { return std::sin(deg * (float)M_PI / 180.0f); }
inline float cos(float deg) { return std::cos(deg * (float)M_PI / 180.0f); }

// atan2 returning degrees, compass convention handled by the caller (this is the plain math atan2).
inline float atan2(float y, float x) { return std::atan2(y, x) * 180.0f / (float)M_PI; }

inline float mod(float a, float m) {
    float r = std::fmod(a, m);
    return r < 0 ? r + m : r;
}

} // namespace Trig
