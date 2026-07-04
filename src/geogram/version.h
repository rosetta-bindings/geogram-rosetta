/*
 * Stand-in for geogram's CMake-generated <geogram/version.h>
 * (configured from src/lib/geogram/basic/version.h.in).
 *
 * geogram-rosetta compiles the geogram sources directly into each binding
 * target (see manifest.json "user_sources"), without running geogram's own
 * CMake, so the generated header does not exist. This directory (src/) is
 * listed before extern/geogram/src/lib in the include path, so
 * #include <geogram/version.h> resolves here.
 *
 * GEOGRAM_VERSION is normally passed with -D by geogram's CMake
 * (basic/common.cpp uses it); it is provided here for the same reason.
 * Keep the version in sync with the geogram checkout in extern/geogram.
 */
#ifndef GEOGRAM_BASIC_VERSION
#define GEOGRAM_BASIC_VERSION

#define VORPALINE_VERSION_MAJOR "1"
#define VORPALINE_VERSION_MINOR "10"
#define VORPALINE_VERSION_PATCH "1"
#define VORPALINE_VERSION "1.10.1-rosetta"
#define VORPALINE_BUILD_NUMBER ""
#define VORPALINE_BUILD_DATE ""
#define VORPALINE_SVN_REVISION ""

#ifndef GEOGRAM_VERSION
#define GEOGRAM_VERSION VORPALINE_VERSION
#endif

#endif
