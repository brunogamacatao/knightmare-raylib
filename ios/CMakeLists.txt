# iOS build, driven by CMake's Xcode generator plus the ios-cmake toolchain file (raylib has no
# first-party iOS backend of its own - see ios/README.md for why this goes through SDL2 instead of
# the GLFW backend the desktop build uses, and for the caveats that come with that). Must be
# configured on macOS with Xcode installed; see README.md in this directory for the exact commands
# and, importantly, what is and isn't verified here.
cmake_minimum_required(VERSION 3.24)
project(knightmare-ios LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR
        "This must be configured with the ios-cmake toolchain (-DCMAKE_TOOLCHAIN_FILE=.../ios.toolchain.cmake "
        "-DPLATFORM=OS64) so CMAKE_SYSTEM_NAME is iOS - see ios/README.md.")
endif()

# SDL2 must already be available as an iOS build (its own Xcode project, or a prebuilt
# SDL2.xcframework) - there is no FetchContent step for it here because SDL2's iOS build is not a
# plain CMake target; see README.md.
find_package(SDL2 REQUIRED)

include(FetchContent)
set(RAYLIB_VERSION 5.5)
FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
# Selects raylib's PLATFORM_DESKTOP_SDL backend (see raylib/cmake/LibraryConfigurations.cmake) -
# the only raylib backend built on an API (SDL2) that itself has real iOS support.
set(PLATFORM "SDL" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE KNIGHTMARE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/../src/*.cpp")

add_executable(knightmare MACOSX_BUNDLE ${KNIGHTMARE_SOURCES})
target_include_directories(knightmare PRIVATE ${CMAKE_SOURCE_DIR}/../src)
target_link_libraries(knightmare PRIVATE raylib SDL2::SDL2)

set_target_properties(knightmare PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.brunogamacatao.knightmare"
    RESOURCE "${CMAKE_SOURCE_DIR}/../assets"
)

# Bundles assets/ into the .app - Assets.cpp loads via raylib's LoadFileData()/LoadTexture(),
# which resolve relative paths against the running process's working directory on this backend;
# GetApplicationDirectory()/chdir at startup may be needed here (see README.md's open items).
