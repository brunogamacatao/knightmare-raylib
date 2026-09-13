#pragma once

// Compile-time platform classification, used to tell "desktop" (mouse/keyboard, real window
// manager, one screen you can make fullscreen) apart from "mobile" (touch-only, no window to
// resize, the original libGDX port's target). Detected from each toolchain's own standard
// predefined macros - no custom CMake plumbing needed.
namespace Platform {

#if defined(__ANDROID__)
inline constexpr bool kMobile = true;
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
inline constexpr bool kMobile = true;
#else
inline constexpr bool kMobile = false;
#endif
#else
inline constexpr bool kMobile = false;
#endif

} // namespace Platform
