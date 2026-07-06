#pragma once

// Include shim used as the manifest header for GEO::Mesh (and the enums
// declared in geogram/mesh/mesh.h).
//
// <new> must come first: geogram's basic/memory.h uses std::get_new_handler
// without including <new> (transitive on libstdc++ / Apple libc++, but NOT in
// the clang-p2996 fork's libc++ that compiles the reflection generator).
#include <new>

#include <geogram/mesh/mesh.h>
