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

geo.initialize(False)  # True for geogram's log output

# ---------------------------------------------------------------- CSG ---
# Evaluate an OpenSCAD-style program: a sphere minus a cylinder.
csg = geo.Mesh()
ok = geo.csg_evaluate_string(
    """
    difference() {
        sphere(r = 10.0);
        cylinder(h = 30.0, r1 = 4.0, r2 = 4.0, center = true);
    }
    """,
    csg,
    False,
)
assert ok, "CSG evaluation failed"
print(f"CSG        : sphere minus cylinder -> "
      f"{csg.nb_vertices()} vertices, {csg.nb_facets()} facets")
csg.save("out_csg.obj")

# ----------------------------------------------------- Boolean operations ---
# Two overlapping spheres, this time through the mesh_* boolean API.
a, b = geo.Mesh(), geo.Mesh()
geo.csg_evaluate_string("sphere(r = 10.0);", a, False)
# geogram evaluates the *compiled* OpenSCAD format (.csg), where high-level
# transforms are lowered to multmatrix.
geo.csg_evaluate_string(
    "multmatrix([[1,0,0,6],[0,1,0,0],[0,0,1,0],[0,0,0,1]]) "
    "{ sphere(r = 10.0); }",
    b,
    False,
)

union = geo.Mesh()
geo.mesh_union(union, a, b, False)
inter = geo.Mesh()
geo.mesh_intersection(inter, a, b, False)
diff = geo.Mesh()
geo.mesh_difference(diff, a, b, False)
print(f"booleans   : union {union.nb_facets()}, "
      f"intersection {inter.nb_facets()}, "
      f"difference {diff.nb_facets()} facets")
union.save("out_union.obj")

# ------------------------------------------------------------- Remeshing ---
# Isotropic remesh of the union to 5000 points.
remeshed = geo.Mesh()
# (M_in, M_out, nb_points, dim, Lloyd iters, Newton iters, Newton m,
#  adjust, adjust_max_edge_distance, adjust_border_importance) — rosetta
# does not capture C++ default arguments, so every parameter is explicit.
geo.remesh_smooth(union, remeshed, 5000, 0, 5, 30, 7, True, 0.5, 2.0)
print(f"remeshing  : union remeshed to {remeshed.nb_vertices()} vertices, "
      f"{remeshed.nb_facets()} facets")
remeshed.save("out_remeshed.obj")

# ------------------------------------- Parameterization and texturing ---
# Build a UV atlas of the remeshed surface (LSCM + xatlas packing) and
# read the texture coordinates back (6 values per triangle).
geo.mesh_make_atlas(remeshed, 45.0, geo.ChartParameterizer.PARAM_LSCM,
                    geo.ChartPacker.PACK_XATLAS, False)
uv = remeshed.tex_coords()
tri = remeshed.triangles()
assert len(uv) == 2 * len(tri), "one (u,v) per triangle corner"
print(f"texturing  : {geo.mesh_get_charts(remeshed)} charts, "
      f"{len(uv) // 2} UV corners in "
      f"[{min(uv):.3f}, {max(uv):.3f}]")

# ------------------------------------------------- Surface reconstruction ---
# Turn the remeshed surface into a bare point cloud, then reconstruct a
# surface from the points alone with Co3Ne (smooth + co-cone triangles).
points = geo.Mesh()
points.set_points(remeshed.vertices())
print(f"pointcloud : {points.nb_vertices()} points, "
      f"{points.nb_facets()} facets")

# radius ~ average spacing x a few; the sphere pair is ~36 across.
geo.Co3Ne_smooth_and_reconstruct(points, 30, 2, 2.0)
print(f"reconstruct: Co3Ne rebuilt {points.nb_facets()} facets "
      f"from the point cloud")
points.save("out_reconstructed.obj")

# ------------------------------------------------------------- Repair ---
geo.mesh_repair(points, geo.MeshRepairMode.MESH_REPAIR_DEFAULT, 0.0)
geo.fill_holes(points, 1e30, 2000, True)
print(f"repair     : after repair + fill_holes -> "
      f"{points.nb_facets()} facets")

print("OK")
