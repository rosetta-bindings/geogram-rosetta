// example_wasm.js — Node.js demo for the geogram wasm-expanded binding
// (mirrors example_python.py).
//
// Build first:
//   source ~/emsdk/emsdk_env.sh
//   emcmake cmake -S bindings/wasm-expanded -B bindings/wasm-expanded/build
//   cmake --build bindings/wasm-expanded/build -j
// then, from the project root:
//   node example_wasm.js
//
// embind notes:
//   * JS arrays do not auto-convert: build Module.vector_double /
//     Module.vector_int explicitly, and read results back element by
//     element (.size() / .get(i)).
//   * Objects created on the C++ heap (meshes, vectors) must be released
//     with .delete() — embind does not garbage-collect them.

const createModule = require("./bindings/wasm-expanded/build/geogram.js");

function toVector(Module, ctor, array) {
    const v = new Module[ctor]();
    for (const x of array) v.push_back(x);
    return v;
}

function fromVector(v) {
    const out = new Array(v.size());
    for (let i = 0; i < v.size(); ++i) out[i] = v.get(i);
    v.delete();
    return out;
}

createModule().then((Module) => {
    Module.initialize(false); // true for geogram's log output

    // -------------------------------------------------------------- CSG ---
    const csg = new Module.Mesh();
    const ok = Module.csg_evaluate_string(
        `difference() {
             sphere(r = 10.0);
             cylinder(h = 30.0, r1 = 4.0, r2 = 4.0, center = true);
         }`,
        csg,
        false
    );
    console.assert(ok, "CSG evaluation failed");
    console.log(`CSG        : sphere minus cylinder -> ` +
                `${csg.nb_vertices()} vertices, ${csg.nb_facets()} facets`);

    // --------------------------------------------------- Boolean operations ---
    const a = new Module.Mesh();
    const b = new Module.Mesh();
    Module.csg_evaluate_string("sphere(r = 10.0);", a, false);
    Module.csg_evaluate_string(
        "multmatrix([[1,0,0,6],[0,1,0,0],[0,0,1,0],[0,0,0,1]]) " +
        "{ sphere(r = 10.0); }",
        b, false
    );

    const union = new Module.Mesh();
    Module.mesh_union(union, a, b, false);
    const inter = new Module.Mesh();
    Module.mesh_intersection(inter, a, b, false);
    const diff = new Module.Mesh();
    Module.mesh_difference(diff, a, b, false);
    console.log(`booleans   : union ${union.nb_facets()}, ` +
                `intersection ${inter.nb_facets()}, ` +
                `difference ${diff.nb_facets()} facets`);

    // ----------------------------------------------------------- Remeshing ---
    const remeshed = new Module.Mesh();
    Module.remesh_smooth(union, remeshed, 5000, 5, 30, 7);
    console.log(`remeshing  : union remeshed to ${remeshed.nb_vertices()} ` +
                `vertices, ${remeshed.nb_facets()} facets`);

    // ----------------------------------- Parameterization and texturing ---
    Module.make_atlas(remeshed, 45.0, "lscm", "xatlas", false);
    const uv = fromVector(remeshed.tex_coords());
    console.log(`texturing  : ${Module.get_charts(remeshed)} charts, ` +
                `${uv.length / 2} UV corners in ` +
                `[${Math.min(...uv).toFixed(3)}, ` +
                `${Math.max(...uv).toFixed(3)}]`);

    // ----------------------------------------------- Surface reconstruction ---
    const coords = remeshed.vertices(); // vector_double (owned by JS)
    const points = new Module.Mesh();
    points.set_points(coords);
    coords.delete();
    console.log(`pointcloud : ${points.nb_vertices()} points, ` +
                `${points.nb_facets()} facets`);

    Module.co3ne_smooth_and_reconstruct(points, 30, 2, 2.0);
    console.log(`reconstruct: Co3Ne rebuilt ${points.nb_facets()} facets ` +
                `from the point cloud`);

    // ----------------------------------------------------------- Repair ---
    Module.mesh_repair(points, 0.0);
    Module.fill_holes(points, 1e30, 2000);
    console.log(`repair     : after repair + fill_holes -> ` +
                `${points.nb_facets()} facets`);

    [csg, a, b, union, inter, diff, remeshed, points].forEach((m) =>
        m.delete()
    );
    console.log("OK");
});
