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

// No initialize() call: geogram's lifecycle (GEO::initialize, the CmdLine
// argument groups, the OpenNL log routing) runs when the module loads, from
// the manifest's "module_init". The log starts off — turn it on with:
//     geo.Logger.instance().set_quiet(false);

// ---------------------------------------------------------------- CSG ---
// GEO::CSGCompiler is bound directly: compile_string / compile_file return the
// mesh they built (a std::shared_ptr<Mesh> on the C++ side — this JS handle
// adopts it and keeps the object alive), and compile_file takes its path as a
// plain string.
//
// A compiler instance is SINGLE-USE: geogram keeps parser/builder state, so a
// second compile on the same object comes back empty. One CSGCompiler per
// program — they are cheap.
function csgEval(program) {
    const compiler = new geo.CSGCompiler();
    compiler.set_verbose(false); // true for the CSG tree and its timings
    return compiler.compile_string(program);
}

const csg = csgEval(
    `difference() {
         sphere(r = 10.0);
         cylinder(h = 30.0, r1 = 4.0, r2 = 4.0, center = true);
     }`
);
console.log(`CSG        : sphere minus cylinder -> ` +
            `${csg.vertices.nb()} vertices, ${csg.facets.nb()} facets`);
csg.save("out_csg_node.obj");

// ----------------------------------------------------- Boolean operations ---
// geogram evaluates the *compiled* OpenSCAD format (.csg): high-level
// transforms are written as multmatrix.
const a = csgEval("sphere(r = 10.0);");
const b = csgEval(
    "multmatrix([[1,0,0,6],[0,1,0,0],[0,0,1,0],[0,0,0,1]]) { sphere(r = 10.0); }"
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
// Geometry and UVs both come out of geogram's own API; get_doubles writes
// through two references in C++, which the manifest declares out-parameters, so
// JS receives them as array elements.
remeshed.facets.triangulate();
const tri = remeshed.facet_corners.vertex_indices();
const [ok, uv, dim] = remeshed.facet_corners.attributes().get_doubles("tex_coord");
console.assert(ok && dim === 2, "the atlas writes a 2-D tex_coord corner attribute");
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
