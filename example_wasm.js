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
//   * mesh.vertices() / mesh.facets() return borrowed (non-owning) handles
//     to the member objects living inside the mesh — do NOT .delete() them,
//     and don't use them after the mesh itself is deleted.

const createModule = require("./bindings/wasm/build/geogram.js");

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
    // No initialize() call: geogram's lifecycle runs when the module loads,
    // from the manifest's "module_init". The log starts off — turn it on
    // with Module.Logger.instance().set_quiet(false).

    // -------------------------------------------------------------- CSG ---
    // GEO::CSGCompiler is bound directly: compile_string / compile_file return
    // the mesh they built (a std::shared_ptr<Mesh> on the C++ side, which
    // embind hands over as a Mesh handle), and compile_file takes its path as a
    // plain string.
    //
    // A compiler instance is SINGLE-USE: geogram keeps parser/builder state, so
    // a second compile on the same object comes back empty. One CSGCompiler per
    // program — they are cheap.
    const csgEval = (program) => {
        const compiler = new Module.CSGCompiler();
        compiler.set_verbose(false); // true for the CSG tree and its timings
        const m = compiler.compile_string(program);
        compiler.delete();           // embind handles are freed explicitly
        return m;
    };

    const csg = csgEval(
        `difference() {
             sphere(r = 10.0);
             cylinder(h = 30.0, r1 = 4.0, r2 = 4.0, center = true);
         }`
    );
    console.log(`CSG        : sphere minus cylinder -> ` +
                `${csg.vertices().nb()} vertices, ${csg.facets().nb()} facets`);

    // --------------------------------------------------- Boolean operations ---
    const a = csgEval("sphere(r = 10.0);");
    const b = csgEval(
        "multmatrix([[1,0,0,6],[0,1,0,0],[0,0,1,0],[0,0,0,1]]) " +
        "{ sphere(r = 10.0); }"
    );

    const union = new Module.Mesh();
    Module.mesh_union(union, a, b, false);
    const inter = new Module.Mesh();
    Module.mesh_intersection(inter, a, b, false);
    const diff = new Module.Mesh();
    Module.mesh_difference(diff, a, b, false);
    console.log(`booleans   : union ${union.facets().nb()}, ` +
                `intersection ${inter.facets().nb()}, ` +
                `difference ${diff.facets().nb()} facets`);

    // ----------------------------------------------------------- Remeshing ---
    const remeshed = new Module.Mesh();
    // every parameter explicit (rosetta does not capture C++ defaults)
    Module.remesh_smooth(union, remeshed, 5000, 0, 5, 30, 7, true, 0.5, 2.0);
    console.log(`remeshing  : union remeshed to ${remeshed.vertices().nb()} ` +
                `vertices, ${remeshed.facets().nb()} facets`);

    // ----------------------------------- Parameterization and texturing ---
    Module.mesh_make_atlas(remeshed, 45.0,
        Module.ChartParameterizer.PARAM_LSCM,
        Module.ChartPacker.PACK_XATLAS, false);
    // get_doubles writes through two references in C++, which the manifest
    // declares out-parameters; embind has no tuple, so they come back as a
    // plain array: [ok, values, dim]. `values` is a vector_double handle.
    remeshed.facets().triangulate();
    const [uvOk, uvVec, uvDim] = remeshed.facet_corners().attributes().get_doubles("tex_coord");
    console.assert(uvOk && uvDim === 2, "the atlas writes a 2-D tex_coord corner attribute");
    const uv = fromVector(uvVec);
    uvVec.delete();
    console.log(`texturing  : ${Module.mesh_get_charts(remeshed)} charts, ` +
                `${uv.length / 2} UV corners in ` +
                `[${Math.min(...uv).toFixed(3)}, ` +
                `${Math.max(...uv).toFixed(3)}]`);

    // ----------------------------------------------- Surface reconstruction ---
    // Geometry moves through geogram's own API: remeshing can leave
    // higher-dimensional points (normals appended), so truncate to xyz
    // before reading the flat coordinate array back.
    remeshed.vertices().set_dimension(3);
    const coords = remeshed.vertices().point_coordinates(); // vector_double (owned by JS)
    const points = new Module.Mesh();
    points.vertices().assign_points(coords, 3, false);
    coords.delete();
    console.log(`pointcloud : ${points.vertices().nb()} points, ` +
                `${points.facets().nb()} facets`);

    Module.Co3Ne_smooth_and_reconstruct(points, 30, 2, 2.0);
    console.log(`reconstruct: Co3Ne rebuilt ${points.facets().nb()} facets ` +
                `from the point cloud`);

    // ----------------------------------------------------------- Repair ---
    Module.mesh_repair(points, Module.MeshRepairMode.MESH_REPAIR_DEFAULT, 0.0);
    Module.fill_holes(points, 1e30, 2000, true);
    console.log(`repair     : after repair + fill_holes -> ` +
                `${points.facets().nb()} facets`);

    [csg, a, b, union, inter, diff, remeshed, points].forEach((m) =>
        m.delete()
    );
    console.log("OK");
});
