#pragma once

#include "geogram_mesh.h"
#include <string>

// The few free functions geogram's own API cannot expose directly:
//   - initialize():   GEO::initialize + the CmdLine argument groups the
//                     algorithms read their tuning parameters from;
//   - the boolean helpers: GEO::mesh_union / mesh_intersection /
//                     mesh_difference are overloaded (enum-flags and
//                     bool-verbose variants) and ^^name is ill-formed for
//                     an overload set;
//   - the CSG entry points: CSGCompiler::compile_* return
//                     std::shared_ptr<Mesh>, which no backend marshals.
//
// Everything else — remesh_smooth, mesh_repair, fill_holes,
// tessellate_facets, Co3Ne_*, mesh_make_atlas, mesh_get_charts,
// mesh_remove_intersections, mesh_facets_have_intersection — is bound
// straight from the GEO:: headers in manifest.json.
namespace georo {

    // Initialize geogram and import the command-line argument groups. Call
    // it first (with verbose=true to enable geogram's logger); the georo
    // helpers also run it lazily.
    void initialize(bool verbose);

    // ------------------------------------------------------------------
    // Boolean operations (A and B must be closed surface meshes without
    // self-intersections).
    // ------------------------------------------------------------------

    // result = A ∪ B
    void mesh_union(GEO::Mesh &result, const GEO::Mesh &A, const GEO::Mesh &B,
                    bool verbose);
    // result = A ∩ B
    void mesh_intersection(GEO::Mesh &result, const GEO::Mesh &A,
                           const GEO::Mesh &B, bool verbose);
    // result = A − B
    void mesh_difference(GEO::Mesh &result, const GEO::Mesh &A,
                         const GEO::Mesh &B, bool verbose);

    // ------------------------------------------------------------------
    // Constructive Solid Geometry
    // ------------------------------------------------------------------
    // Evaluates OpenSCAD .csg programs (the *compiled* OpenSCAD format:
    // openscad model.scad -o model.csg), e.g.
    //   difference() { sphere(r=10); cylinder(h=30, r1=3, r2=3, center=true); }
    // Objects: square, circle, cube, sphere, cylinder, polyhedron, polygon,
    // import, surface, text. Instructions: multmatrix, resize, union,
    // intersection, difference, group, color, hull, linear_extrude,
    // rotate_extrude, projection, minkowski, render. Note: high-level
    // transforms (translate, rotate, scale) are OpenSCAD sugar that its
    // compiler lowers to multmatrix — write them as
    //   multmatrix([[1,0,0,tx],[0,1,0,ty],[0,0,1,tz],[0,0,0,1]]) { ... }

    // Evaluate a CSG program from a source string into result.
    bool csg_evaluate_string(const std::string &source, GEO::Mesh &result,
                             bool verbose);

    // Evaluate a .csg / .scad file into result.
    bool csg_evaluate_file(const std::string &path, GEO::Mesh &result,
                           bool verbose);

} // namespace georo
