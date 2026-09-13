# iOS build - status and instructions

**Honesty up front:** raylib has no official first-party iOS backend (only Desktop/GLFW,
Android, Web, and DRM - confirmed by reading raylib 5.5's own `cmake/LibraryConfigurations.cmake`).
This directory sets up the one credible community-known path - raylib's alternate
`PLATFORM_DESKTOP_SDL` backend, on top of SDL2 (which does have real, official iOS support) - but
it has **not been built or run on an actual device or simulator**, because doing so requires
macOS + Xcode, which was not available in the environment this port was created in. Treat this as
a well-researched starting point, not a verified one, and budget time to debug it on a Mac.

If that risk isn't acceptable, the fallback is to skip native iOS and ship the desktop/Android
targets (both fully built and tested - see the top-level README), or to revisit iOS later against
whatever raylib's iOS support looks like at that time.

## Why SDL and not the desktop (GLFW) backend

The desktop and Android targets (`../CMakeLists.txt`) use raylib's default backends: GLFW for
desktop, a custom Android glue for Android. GLFW does not support iOS. raylib's build system does
offer a second desktop-family backend, `PLATFORM_DESKTOP_SDL` (selected via `-DPLATFORM=SDL`),
which links SDL2 instead of GLFW for windowing/input/audio - and SDL2 itself has a real, maintained
iOS backend. In principle, raylib's `rcore.c` calls into SDL the same way on any platform SDL
supports, so PLATFORM_DESKTOP_SDL *should* carry over to iOS - but this specific combination isn't
one raylib's own CI covers, so real iOS-specific issues (app suspend/resume, the Metal-backed GLES
context SDL sets up on iOS, touch-vs-mouse event mapping) are plausible and unverified here.

## Building (on macOS, with Xcode installed)

1. **Get SDL2 built for iOS.** This is the part with no CMake shortcut: SDL2's iOS support ships as
   its own Xcode project, not a portable CMake target. Either:
   - Build `Xcode/SDL/SDL.xcodeproj` from the [SDL2 source](https://github.com/libsdl-org/SDL)
     (tag `release-2.30.x` or newer) for the `iOS` scheme, or
   - Use a prebuilt `SDL2.xcframework` (e.g. from Homebrew's `sdl2` cask sources, or SDL2's GitHub
     Releases, if one is published for iOS).
   Point CMake at it via `-DSDL2_DIR=<path to the SDL2 CMake config>` or by adding the
   `.xcframework` to `find_package(SDL2)`'s search path.

2. **Get the [ios-cmake](https://github.com/leetal/ios-cmake) toolchain file** (a single
   `ios.toolchain.cmake`) - this is what teaches plain CMake to target iOS instead of macOS.

3. Configure and generate an Xcode project:
   ```bash
   cmake -S ios -B ios/build -G Xcode \
       -DCMAKE_TOOLCHAIN_FILE=/path/to/ios-cmake/ios.toolchain.cmake \
       -DPLATFORM=OS64 \
       -DDEPLOYMENT_TARGET=13.0 \
       -DSDL2_DIR=/path/to/SDL2/build/cmake
   open ios/build/knightmare-ios.xcodeproj
   ```

4. In Xcode: set your Team/signing on the `knightmare` target, then build and run on a simulator
   or device.

## Known open items to check on a Mac

- **Asset loading paths.** `Assets.cpp`/`MapData.cpp` load everything through raylib's
  `LoadFileData()`/`LoadTexture()` with paths like `"assets/tiles/maps.png"`, relative to the
  process's working directory - correct as-is for desktop (see `../CMakeLists.txt`'s
  post-build asset copy) and for Android (raylib redirects file loads through the APK's asset
  manager automatically on that platform). Neither mechanism applies on iOS: the working directory
  at launch is not the app bundle's `Resources/` folder. This CMakeLists marks `../assets` as a
  bundle `RESOURCE`, but `main.cpp` will likely need an iOS-specific startup step - probably calling
  `chdir(GetApplicationDirectory())` (or `SDL_GetBasePath()`) right after `InitWindow()` - before
  any asset loads happen, so the existing "assets/..." relative paths resolve inside the bundle.
  Untested; check this first if textures/sounds/maps fail to load.
- **Touch input.** `VirtualControls` reads `GetTouchPointCount()`/`GetTouchPosition()`/
  `GetTouchPointId()` and `IsMouseButtonDown(MOUSE_BUTTON_LEFT)` - both are raylib-level APIs that
  PLATFORM_DESKTOP_SDL should translate from SDL's touch/mouse events, but this has not been
  exercised on an actual touchscreen through this backend.
  `Platform::kMobile` (`../src/Platform.h`) already resolves to `true` for `TARGET_OS_IPHONE`, so
  the on-screen controls and portrait layout (matching the original libGDX Android port, per the
  top-level README) are enabled the same way they are on Android - no changes needed there.
- **Audio.** raylib's `raudio` module uses miniaudio, which has its own iOS (Core Audio) backend
  independent of the SDL/GLFW choice above, so music/sfx should work without extra wiring - but
  again, not verified here.
- **App lifecycle.** Backgrounding/foregrounding, interruptions (phone calls), and screen lock are
  not handled specially anywhere in this codebase; whether SDL's default iOS lifecycle handling is
  sufficient as-is is untested.
