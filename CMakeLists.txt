cmake_minimum_required(VERSION 3.24...3.30)
project(knightmare LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# PLATFORM: Desktop (default, Linux/Windows/macOS), Android, or Web. iOS is built from ios/ via its
# own CMake + Xcode generator (see ios/README.md) rather than through this PLATFORM switch, since
# raylib has no first-party iOS backend to select here.
if(NOT PLATFORM)
    set(PLATFORM "Desktop")
endif()

include(FetchContent)
set(RAYLIB_VERSION 5.5)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)
# Static, no examples/games built alongside our own target - we only want the library.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE KNIGHTMARE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

if(${PLATFORM} MATCHES "Android")
    # NativeActivity requires a shared library named lib<name>.so, loaded by the Android Gradle
    # module in android/ (see its build.gradle.kts externalNativeBuild block, which invokes this
    # same CMakeLists.txt with -DPLATFORM=Android for every ABI it targets).
    add_library(knightmare SHARED ${KNIGHTMARE_SOURCES})
    find_library(ANDROID_LOG_LIB log)
    target_link_libraries(knightmare PRIVATE ${ANDROID_LOG_LIB} android)
    # raylib compiles android_native_app_glue.c (which defines ANativeActivity_onCreate, the real
    # entry point NativeActivity looks up via dlsym) into libraylib.a, but nothing in our own code
    # calls that symbol directly - so the linker has no reason to pull that object file out of the
    # static archive, and it's silently dropped, leaving it missing from the final .so (crashes at
    # launch with "UnsatisfiedLinkError: undefined symbol: ANativeActivity_onCreate"). Forcing it
    # as an undefined symbol makes the linker pull it in and export it.
    target_link_options(knightmare PRIVATE "-Wl,-u,ANativeActivity_onCreate")
    # Android 15+ devices can use a 16 KB memory page size; a shared library whose ELF LOAD
    # segments are only 4 KB-aligned (the NDK's old default) still loads there today, but shows a
    # compatibility warning and is expected to eventually be rejected outright. This NDK
    # version (r27c) doesn't default to 16 KB alignment yet, so it's requested explicitly - see
    # https://developer.android.com/guide/practices/page-sizes.
    target_link_options(knightmare PRIVATE "-Wl,-z,max-page-size=16384")
else()
    add_executable(knightmare ${KNIGHTMARE_SOURCES})
endif()

target_compile_features(knightmare PRIVATE cxx_std_20)
target_link_libraries(knightmare PRIVATE raylib)

if(APPLE)
    target_link_libraries(knightmare PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

# Desktop builds run straight out of the build directory: keep a copy of assets/ next to the
# executable so the relative "assets/..." paths (see Assets.cpp) resolve without installing
# anything. Android instead bundles assets/ into the APK (see android/app/build.gradle.kts).
if(${PLATFORM} MATCHES "Desktop")
    add_custom_command(TARGET knightmare POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:knightmare>/assets
        COMMENT "Copying assets next to the knightmare executable"
    )
    set_target_properties(knightmare PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
