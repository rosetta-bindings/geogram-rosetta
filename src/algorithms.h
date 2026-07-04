#pragma once

#include "Mesh.h"
#include <string>

// Free functions mirroring geogram's algorithm API, expressed on the
// georo::Mesh wrapper. Each one is a one-line delegation to the
// corresponding GEO:: function (see algorithms.cpp) — no algorithmic code
// lives here.
//
// All functions lazily initialize geogram (GEO::initialize + command-line
// arg groups) on first use; call initialize(true) first if you want
// geogram's log output.
namespace georo {

    // Initialize geogram and import the command-line argument groups the
    // algorithms read their tuning parameters from. Called automatically by
    // every function below; call it explicitly with verbose=true to enable
    // geogram's logger output.
    void initialize(bool verbose);

    // ------------------------------------------------------------------
    // Surface reconstruction (Co3Ne: Concurrent Co-Cones)
    // ------------------------------------------------------------------

    // Smooth a point cloud by projection onto the best approximating plane
    // of the nb_neighbors nearest neighbors.
    void co3ne_smooth(Mesh &points, int nb_neighbors, int nb_iterations);

    // Smooth a point cloud then reconstruct a triangulated surface.
    // radius is the maximum distance used to connect neighbors with
    // triangles. The mesh is modified in place: on exit it contains the
    // reconstructed surface.
    void co3ne_smooth_and_reconstruct(Mesh &points, int nb_neighbors,
                                      int nb_iterations, double radius);

    // ------------------------------------------------------------------
    // Remeshing
    // ------------------------------------------------------------------

    // Isotropic remeshing by Centroidal Voronoi Tesselation: resample
    // input with nb_points, optimized by nb_lloyd_iter Lloyd iterations
    // then nb_newton_iter Newton iterations (newton_m evaluations per
    // step, 7 is a good value).
    void remesh_smooth(Mesh &input, Mesh &output, int nb_points,
                       int nb_lloyd_iter, int nb_newton_iter, int newton_m);

    // ------------------------------------------------------------------
    // Repair
    // ------------------------------------------------------------------

    // Glue colocated vertices / remove degenerate and duplicated facets,
    // connect and reorient the facets (default geogram repair mode).
    // colocate_epsilon is the tolerance used to merge vertices (0 = exact).
    void mesh_repair(Mesh &M, double colocate_epsilon);

    // Fill the holes whose area is smaller than max_area and with fewer
    // than max_edges border edges.
    void fill_holes(Mesh &M, double max_area, int max_edges);

    // Subdivide facets with more than max_nb_vertices vertices
    // (max_nb_vertices = 3 triangulates the mesh).
    void tessellate_facets(Mesh &M, int max_nb_vertices);

    // ------------------------------------------------------------------
    // Parameterization and texturing
    // ------------------------------------------------------------------

    // Segment the (triangulated) mesh into charts, flatten each chart and
    // pack them in texture space. The u,v coordinates are stored in the
    // mesh and read back with Mesh::tex_coords().
    //  - hard_angles_threshold: dihedral angles larger than this (degrees)
    //    are chart boundaries
    //  - parameterizer: "projection", "lscm", "spectral" or "abf"
    //  - packer: "none", "tetris" or "xatlas"
    void make_atlas(Mesh &M, double hard_angles_threshold,
                    const std::string &parameterizer, const std::string &packer,
                    bool verbose);

    // Number of charts of a parameterized mesh (stored in the "chart"
    // facet attribute by make_atlas).
    int get_charts(Mesh &M);

    // ------------------------------------------------------------------
    // Intersections and Boolean operations
    // ------------------------------------------------------------------
    // A and B must be closed surface meshes without self-intersections.

    // result = A ∪ B
    void mesh_union(Mesh &result, const Mesh &A, const Mesh &B, bool verbose);
    // result = A ∩ B
    void mesh_intersection(Mesh &result, const Mesh &A, const Mesh &B,
                           bool verbose);
    // result = A − B
    void mesh_difference(Mesh &result, const Mesh &A, const Mesh &B,
                         bool verbose);

    // Resolve the self-intersections of a surface mesh (at most max_iter
    // passes of intersection removal / re-triangulation).
    void mesh_remove_intersections(Mesh &M, int max_iter, bool verbose);

    // Test whether two facets of the same mesh intersect.
    bool mesh_facets_have_intersection(Mesh &M, int f1, int f2);

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
    bool csg_evaluate_string(const std::string &source, Mesh &result,
                             bool verbose);

    // Evaluate a .csg / .scad file into result.
    bool csg_evaluate_file(const std::string &path, Mesh &result, bool verbose);

} // namespace georo
