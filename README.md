# Knightmare - raylib / C++20 port

A modern C++20 ([raylib](https://www.raylib.com/)) port of [`knightmare-libgdx`](../knightmare-libgdx),
itself a Java/libGDX port of [leonicolas/knightmare-cmm2](https://github.com/leonicolas/knightmare-cmm2),
a fan-made recreation of the 1986 MSX Konami game **Knightmare**. This port replaces libGDX/JVM with
raylib and targets **desktop (Linux, Windows), Android, and iOS** from one shared C++ codebase.

All game logic (`src/`) is a line-by-line translation of the libGDX port's Java: same constants,
same per-tick simulation order, same weapon/boss/collision tables. See individual source files for
notes on the handful of deliberate behavioural differences (mostly around entity storage - see
`GameState.h` - and platform-specific input/rendering, covered below).

All original assets are reused as-is: sprites and map tilesets (`assets/tiles/*.png`), sound
effects (`assets/sfx/*.wav`), music (`assets/music/*.ogg`), and compiled map data
(`assets/maps/*.map`) - same files as `knightmare-libgdx/assets`.

## Project layout

- `src/` - all game/engine code, shared by every platform (no platform-specific game logic; see
  `Platform.h` for the one compile-time desktop/mobile switch that exists, used only for input and
  layout, not gameplay)
- `assets/` - shared assets (tiles, sfx, music, compiled maps)
- `CMakeLists.txt` - builds desktop (default) or Android (`-DPLATFORM=Android`); fetches raylib
  automatically via CMake's `FetchContent`
- `android/` - Gradle wrapper project around the same `CMakeLists.txt`, producing an APK
- `ios/` - iOS build setup - **unverified, see `ios/README.md` before relying on it**

## Desktop (Linux / Windows)

Requires CMake 3.24+ and a C++20 compiler (tested with GCC 13). Requires network access on first
configure (downloads raylib's source via `FetchContent`).

```bash
cmake -S . -B build
cmake --build build -j
./build/bin/knightmare
```

Desktop always runs **fullscreen at the monitor's native resolution**, keyboard-only (arrow
keys/WASD to move, space or Z to fire) - there is no on-screen touch overlay on desktop, unlike the
original libGDX port's desktop launcher (which showed the touch controls too, since libGDX made
that essentially free). Mouse clicks still work for the title menu.

Windows builds with the same `CMakeLists.txt` via MSVC or MinGW (`cmake -S . -B build -G "Visual
Studio 17 2022"`, or an MSYS2/MinGW toolchain) - not cross-compiled/tested from this Linux
environment, but nothing in the build is Linux-specific (raylib itself is fetched and built the
same way on Windows).

## Android

Requires the Android SDK (`compileSdk`/`targetSdk` 36) and NDK (tested with r27c), and a system
CMake (see `android/local.properties`'s `cmake.dir`, which points AGP at a plain system CMake
install instead of requiring the SDK's separate "cmake" package). Update `sdk.dir`/`cmake.dir` in
`android/local.properties` for your machine (that file is machine-specific and not meant to be
committed as-is).

```bash
cd android
./gradlew assembleDebug          # -> android/app/build/outputs/apk/debug/app-debug.apk
./gradlew installDebug           # build + install on a connected device/emulator
```

This app has **no Java or Kotlin code at all** - it's a pure `android.app.NativeActivity`
(see `android/app/src/main/AndroidManifest.xml`'s `android:hasCode="false"` and
`android.app.lib_name` meta-data) whose native library is built from the exact same
`src/*.cpp`/`CMakeLists.txt` as the desktop build, via `-DPLATFORM=Android` (see
`android/app/build.gradle.kts`). raylib's own Android integration provides the real
`android_main()` entry point and calls this codebase's ordinary `main()` from inside it once the
native window is ready (see `src/main.cpp`).

Portrait orientation with on-screen virtual controls (d-pad bottom-left, fire button bottom-right)
- this is unchanged from the original libGDX/Android port's design.

Verified: this actually builds a working APK (all three ABIs: `arm64-v8a`, `armeabi-v7a`,
`x86_64`) in this environment.

## iOS

**Not verified** - raylib has no official iOS backend, so this goes through a less-traveled path
(raylib's SDL2 backend, since SDL2 itself supports iOS). There was no macOS/Xcode available to
actually build or run this during development. See `ios/README.md` for the full explanation, setup
steps, and a list of specific things to check first if it doesn't work out of the box (asset
loading paths being the most likely culprit).

## Credits

- Original MSX game: Konami (1986)
- MMBasic version: Leonardo Berardino ([leonicolas/knightmare-cmm2](https://github.com/leonicolas/knightmare-cmm2))
- Music: H0ffman
- libGDX/Android port: community port based on the above (`../knightmare-libgdx`)
- This raylib/C++20 port: translated from the libGDX port
