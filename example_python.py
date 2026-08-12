# End-to-end demo of the geogram bindings: CSG, booleans, remeshing,
# atlas parameterization / texturing and Co3Ne surface reconstruction.
#
#   cmake -S bindings/python-expanded -B bindings/python-expanded/build
#   cmake --build bindings/python-expanded/build -j
#   python3 example_python.py
import math
import sys

sys.path.insert(0, "bindings/python-expanded")
import geogram as geo

# No initialize() call: geogram's lifecycle (GEO::initialize, the CmdLine
# argument groups, the OpenNL log routing) runs when the module loads, from
# the manifest's "module_init". The log starts off — turn it on with:
#     geo.Logger.instance().set_quiet(False)

# ---------------------------------------------------------------- CSG ---
# GEO::CSGCompiler is bound directly: compile_string / compile_file return the
# mesh they built (a std::shared_ptr<Mesh> on the C++ side — the object stays
# alive for as long as this handle does), and compile_file takes its path as a
# plain string.
#
# A compiler instance is SINGLE-USE: geogram keeps parser/builder state, so a
# second compile on the same object comes back empty. One CSGCompiler per
# program — they are cheap.
def csg_eval(program):
    compiler = geo.CSGCompiler()
    compiler.set_verbose(False)  # True for the CSG tree and its timings
    return compiler.compile_string(program)


# Evaluate an OpenSCAD-style program: a sphere minus a cylinder.
csg = csg_eval(
    """
    difference() {
        sphere(r = 10.0);
        cylinder(h = 30.0, r1 = 4.0, r2 = 4.0, center = true);
    }
    """
)
print(f"CSG        : sphere minus cylinder -> "
      f"{csg.vertices.nb()} vertices, {csg.facets.nb()} facets")
csg.save("out_csg.obj")

# ----------------------------------------------------- Boolean operations ---
# Two overlapping spheres, this time through the mesh_* boolean API. geogram
# declares each of these twice (a flags variant and this bool-verbose one); the
# manifest picks this one by signature.
a = csg_eval("sphere(r = 10.0);")
# geogram evaluates the *compiled* OpenSCAD format (.csg), where high-level
# transforms are lowered to multmatrix.
b = csg_eval(
    "multmatrix([[1,0,0,6],[0,1,0,0],[0,0,1,0],[0,0,0,1]]) "
    "{ sphere(r = 10.0); }"
)

union = geo.Mesh()
geo.mesh_union(union, a, b, False)
inter = geo.Mesh()
geo.mesh_intersection(inter, a, b, False)
diff = geo.Mesh()
geo.mesh_difference(diff, a, b, False)
print(f"booleans   : union {union.facets.nb()}, "
      f"intersection {inter.facets.nb()}, "
      f"difference {diff.facets.nb()} facets")
union.save("out_union.obj")

# ------------------------------------------------------------- Remeshing ---
# Isotropic remesh of the union to 5000 points.
remeshed = geo.Mesh()
# (M_in, M_out, nb_points, dim, Lloyd iters, Newton iters, Newton m,
#  adjust, adjust_max_edge_distance, adjust_border_importance) — rosetta
# does not capture C++ default arguments, so every parameter is explicit.
geo.remesh_smooth(union, remeshed, 5000, 0, 5, 30, 7, True, 0.5, 2.0)
print(f"remeshing  : union remeshed to {remeshed.vertices.nb()} vertices, "
      f"{remeshed.facets.nb()} facets")
remeshed.save("out_remeshed.obj")

# ------------------------------------- Parameterization and texturing ---
# Build a UV atlas of the remeshed surface (LSCM + xatlas packing) and
# read the texture coordinates back (6 values per triangle).
geo.mesh_make_atlas(remeshed, 45.0, geo.ChartParameterizer.PARAM_LSCM,
                    geo.ChartPacker.PACK_XATLAS, False)
# Geometry and UVs both come out of geogram's own API. triangulate() is a no-op
# on an already-triangulated mesh and fan-triangulates polygons otherwise, so
# the corner arrays below line up three-per-triangle.
remeshed.facets.triangulate()
tri = remeshed.facet_corners.vertex_indices()
# get_doubles writes through two references in C++; the manifest declares them
# out-parameters, so Python gets them back as a tuple.
ok, uv, dim = remeshed.facet_corners.attributes().get_doubles("tex_coord")
assert ok and dim == 2, "the atlas writes a 2-D tex_coord corner attribute"
assert len(uv) == 2 * len(tri), "one (u,v) per triangle corner"
print(f"texturing  : {geo.mesh_get_charts(remeshed)} charts, "
      f"{len(uv) // 2} UV corners in "
      f"[{min(uv):.3f}, {max(uv):.3f}]")

# ------------------------------------------------- Surface reconstruction ---
# Turn the remeshed surface into a bare point cloud, then reconstruct a
# surface from the points alone with Co3Ne (smooth + co-cone triangles).
# Geometry moves through geogram's own API: remeshing can leave
# higher-dimensional points (normals appended), so truncate to xyz before
# reading the flat coordinate array back.
remeshed.vertices.set_dimension(3)
points = geo.Mesh()
points.vertices.assign_points(remeshed.vertices.point_coordinates(), 3, False)
print(f"pointcloud : {points.vertices.nb()} points, "
      f"{points.facets.nb()} facets")

# radius ~ average spacing x a few; the sphere pair is ~36 across.
geo.Co3Ne_smooth_and_reconstruct(points, 30, 2, 2.0)
print(f"reconstruct: Co3Ne rebuilt {points.facets.nb()} facets "
      f"from the point cloud")
points.save("out_reconstructed.obj")

# ------------------------------------------------------------- Repair ---
geo.mesh_repair(points, geo.MeshRepairMode.MESH_REPAIR_DEFAULT, 0.0)
geo.fill_holes(points, 1e30, 2000, True)
print(f"repair     : after repair + fill_holes -> "
      f"{points.facets.nb()} facets")

print("OK")
