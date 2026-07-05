# -------------------------------------------------------------------
# Bootstrap only: fetches geogram AND rosetta into extern/ and builds
# the rosetta_gen tool (-> extern/rosetta/bin/rosetta_gen). The
# bindings themselves are NOT built here — see README.md
# (rosetta_gen -> generator -> per-language projects under bindings/).
#
#   cmake -B build && cmake --build build
# -------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)
project(geogram-rosetta VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include(FetchContent)

# ----------------------------------------------------------------------------
# GEOGRAM library — sources only.
# The binding projects compile the geogram sources directly (manifest
# "user_sources"), so geogram is downloaded but its own build is never
# configured: FetchContent_Populate, not FetchContent_MakeAvailable.
# NOTE: geogram uses submodules (OpenNL, zlib, rply, libMeshb, xatlas,
# HLBFGS, ...); if extern/geogram was cloned by hand, make sure they are
# initialized: git submodule update --init --depth 1
# ----------------------------------------------------------------------------
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/extern/geogram/src/lib/geogram/mesh/mesh.h)
    message(STATUS "Fetching Geogram library...")
    FetchContent_Declare(
        geogram
        GIT_REPOSITORY https://github.com/BrunoLevy/geogram
        GIT_TAG        main
        GIT_SHALLOW    TRUE
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/geogram
        BINARY_DIR ${CMAKE_BINARY_DIR}/extern/geogram-build
    )
    FetchContent_Populate(geogram)
endif()

# ----------------------------------------------------------------------------
# ROSETTA library — fetched into extern/ like geogram (sources only; the
# manifest's "rosetta_include" points to extern/rosetta/include).
# Same guard as geogram: an existing checkout (or a symlink to a local
# working copy) is left untouched.
# ----------------------------------------------------------------------------
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/extern/rosetta/include/rosetta/generate.h)
    message(STATUS "Fetching Rosetta library...")
    FetchContent_Declare(
        rosetta
        GIT_REPOSITORY https://github.com/xaliphostes/rosetta.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/rosetta
        BINARY_DIR ${CMAKE_BINARY_DIR}/extern/rosetta-build
    )
    FetchContent_Populate(rosetta)
endif()

# The rosetta_gen code generator lives in tools/ and has its own standalone
# CMakeLists.txt — build it as a subdirectory. Its binary always lands in
# <rosetta>/bin, i.e. extern/rosetta/bin/rosetta_gen.
# NOTE: rosetta_gen requires CMake >= 3.28.
message(STATUS "Building rosetta_gen tool...")
add_subdirectory(
    ${CMAKE_SOURCE_DIR}/extern/rosetta/tools/rosetta_gen
    ${CMAKE_BINARY_DIR}/extern/rosetta_gen-build
)
