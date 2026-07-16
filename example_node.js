#!/usr/bin/env node
"use strict";

/*
 * geogram — Node binding demo (mirrors example_python.py).
 *
 * Build first:
 *   cd bindings/node-expanded && npm i && npm run build
 * then, from the project root:
 *   node example_node.js
 *
 * CSG, boolean operations, CVT remeshing, atlas parameterization /
 * texturing and Co3Ne surface reconstruction, all through the
 * rosetta-generated N-API addon.
 */

const path = require("path");

const geo = require(path.join(
    __dirname, "bindings", "node-expanded", "build", "Release", "geogram.node"
));

geo.initialize(false); // true for geogram's log output

// ---------------------------------------------------------------- CSG ---
const csg = new geo.Mesh();
const ok = geo.csg_evaluate_string(
    `difference() {
         sphere(r = 10.0);
         cylinder(h = 30.0, r1 = 4.0, r2 = 4.0, center = true);
     }`,
    csg,
    false
);
console.assert(ok, "CSG evaluation failed");
console.log(`CSG        : sphere minus cylinder -> ` +
            `${csg.vertices.nb()} vertices, ${csg.facets.nb()} facets`);
csg.save("out_csg_node.obj");

// ----------------------------------------------------- Boolean operations ---
// geogram evaluates the *compiled* OpenSCAD format (.csg): high-level
// transforms are written as multmatrix.
const a = new geo.Mesh();
const b = new geo.Mesh();
geo.csg_evaluate_string("sphere(r = 10.0);", a, false);
geo.csg_evaluate_string(
    "multmatrix([[1,0,0,6],[0,1,0,0],[0,0,1,0],[0,0,0,1]]) { sphere(r = 10.0); }",
    b, false
);

const union = new geo.Mesh();
geo.mesh_union(union, a, b, false);
const inter = new geo.Mesh();
geo.mesh_intersection(inter, a, b, false);
const diff = new geo.Mesh();
geo.mesh_difference(diff, a, b, false);
console.log(`booleans   : union ${union.facets.nb()}, ` +
            `intersection ${inter.facets.nb()}, ` +
            `difference ${diff.facets.nb()} facets`);

// ------------------------------------------------------------- Remeshing ---
const remeshed = new geo.Mesh();
// (M_in, M_out, nb_points, dim, Lloyd, Newton, Newton_m, adjust,
//  adjust_max_edge_distance, adjust_border_importance) — rosetta does not
// capture C++ default arguments, so every parameter is explicit.
geo.remesh_smooth(union, remeshed, 5000, 0, 5, 30, 7, true, 0.5, 2.0);
console.log(`remeshing  : union remeshed to ${remeshed.vertices.nb()} ` +
            `vertices, ${remeshed.facets.nb()} facets`);

// ------------------------------------- Parameterization and texturing ---
geo.mesh_make_atlas(remeshed, 45.0, geo.ChartParameterizer.PARAM_LSCM,
                    geo.ChartPacker.PACK_XATLAS, false);
const uv = remeshed.tex_coords();
const tri = remeshed.triangles();
console.assert(uv.length === 2 * tri.length, "one (u,v) per corner");
console.log(`texturing  : ${geo.mesh_get_charts(remeshed)} charts, ` +
            `${uv.length / 2} UV corners in ` +
            `[${Math.min(...uv).toFixed(3)}, ${Math.max(...uv).toFixed(3)}]`);

// ------------------------------------------------- Surface reconstruction ---
// Geometry moves through geogram's own API: remeshing can leave
// higher-dimensional points (normals appended), so truncate to xyz before
// reading the flat coordinate array back.
remeshed.vertices.set_dimension(3);
const points = new geo.Mesh();
points.vertices.assign_points(remeshed.vertices.point_coordinates(), 3, false);
console.log(`pointcloud : ${points.vertices.nb()} points, ` +
            `${points.facets.nb()} facets`);

geo.Co3Ne_smooth_and_reconstruct(points, 30, 2, 2.0);
console.log(`reconstruct: Co3Ne rebuilt ${points.facets.nb()} facets ` +
            `from the point cloud`);

// ------------------------------------------------------------- Repair ---
geo.mesh_repair(points, geo.MeshRepairMode.MESH_REPAIR_DEFAULT, 0.0);
geo.fill_holes(points, 1e30, 2000, true);
console.log(`repair     : after repair + fill_holes -> ` +
            `${points.facets.nb()} facets`);

console.log("OK");
