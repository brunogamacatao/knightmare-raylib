#include "raylib.h"
#include "App.h"
#include "Platform.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include <unistd.h>
#endif
#endif

// Single entry point for every platform. Desktop (Linux/Windows) links this straight into an
// executable and the OS calls main() directly. Android is different only in how it gets here: it
// builds as a shared library loaded by a NativeActivity, and raylib's own android_native_app_glue
// integration (compiled into raylib when built with PLATFORM=Android - see rcore_android.c)
// provides the real android_main() entry point and calls this exact main(argc, argv) from inside
// it once the native window is ready, so no separate Android-specific main is needed here.
int main(int /*argc*/, char* /*argv*/[]) {
    SetConfigFlags(FLAG_VSYNC_HINT);
    // On mobile, passing 0x0 tells raylib to use the device's actual display resolution as the
    // "screen" size (see rcore.c's SetupFramebuffer). Passing a size that DOESN'T match the real
    // display instead - as this used to do unconditionally - makes raylib letterbox/offset the
    // render buffer to the requested aspect ratio (its "Upscaling required" path), which then
    // renders one screen's worth of content shifted away from (0,0) by an offset our own layout
    // code (GameRenderer, VirtualControls - both computed purely from GetScreenWidth/Height) has no
    // way to know about, misplacing the HUD/game-area/controls-bar vertically. Desktop still opens
    // at a fixed windowed size here; it's immediately replaced by the real monitor size below.
    if constexpr (Platform::kMobile) InitWindow(0, 0, "Knightmare");
    else InitWindow(480, 854, "Knightmare");

#if defined(__APPLE__) && TARGET_OS_IPHONE
    // Every asset path in this codebase (Assets.cpp, MapData.cpp) is relative to the process's
    // working directory, which on iOS does not start out as the app bundle's Resources/ folder the
    // way it does on desktop (see ../CMakeLists.txt's post-build copy) or Android (raylib
    // transparently redirects file loads through the APK's asset manager there instead). This is
    // the best-effort fix for that - unverified, see ios/README.md's "known open items".
    chdir(GetApplicationDirectory());
#endif

    if constexpr (!Platform::kMobile) {
        // Desktop always runs fullscreen at the monitor's own resolution, keyboard/mouse only, with
        // no on-screen touch controls bar (see VirtualControls/GameRenderer, which check
        // Platform::kMobile) - unlike the original libGDX port, whose desktop launcher also drew
        // the touch controls (there, mostly "for free", for convenience/testing). Monitor size can
        // only be queried once the window/display backend exists, hence the resize-then-toggle
        // instead of sizing the window up front.
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    }

    SetTargetFPS(60);
    InitAudioDevice();

    {
        App app;

        while (!WindowShouldClose() && !app.quitRequested()) {
            float delta = GetFrameTime();
            BeginDrawing();
            app.step(delta);
            EndDrawing();
        }
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
