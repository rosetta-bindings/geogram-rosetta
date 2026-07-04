# -------------------------------------------------------------------
# Bootstrap only: fetches geogram into extern/ and builds the
# rosetta_gen tool. The bindings themselves are NOT built here — see
# README.md (rosetta_gen -> generator -> per-language projects under
# bindings/).
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
# ROSETTA — expected as a sibling checkout (../rosetta), like the other
# *-rosetta projects; the manifest's "rosetta_include" points there too.
# The rosetta_gen code generator lives in tools/ and has its own
# standalone CMakeLists.txt — build it as a subdirectory.
# NOTE: rosetta_gen requires CMake >= 3.28.
# ----------------------------------------------------------------------------
set(ROSETTA_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../rosetta)
if(NOT EXISTS ${ROSETTA_SOURCE_DIR}/include/rosetta/generate.h)
    message(FATAL_ERROR
        "rosetta not found at ${ROSETTA_SOURCE_DIR} — clone "
        "https://github.com/xaliphostes/rosetta.git next to this project.")
endif()

message(STATUS "Building rosetta_gen tool...")
add_subdirectory(
    ${ROSETTA_SOURCE_DIR}/tools/rosetta_gen
    ${CMAKE_BINARY_DIR}/extern/rosetta_gen-build
)
