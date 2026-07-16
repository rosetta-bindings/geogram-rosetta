-- End-to-end demo of the geogram bindings: CSG, booleans, remeshing,
-- atlas parameterization / texturing and Co3Ne surface reconstruction.
--
--   cmake -S bindings/lua-expanded -B bindings/lua-expanded/build
--   cmake --build bindings/lua-expanded/build -j
--   lua example_lua.lua
package.cpath = "bindings/lua-expanded/build/?.so;" .. package.cpath
local geo = require("geogram")

geo.initialize(false)  -- true for geogram's log output

------------------------------------------------------------------- CSG ---
-- Evaluate an OpenSCAD-style program: a sphere minus a cylinder.
local csg = geo.Mesh.new()
local ok = geo.csg_evaluate_string(
    [[
    difference() {
        sphere(r = 10.0);
        cylinder(h = 30.0, r1 = 4.0, r2 = 4.0, center = true);
    }
    ]],
    csg,
    false)
assert(ok, "CSG evaluation failed")
print(string.format("CSG        : sphere minus cylinder -> %d vertices, %d facets",
                    csg.vertices:nb(), csg.facets:nb()))
csg:save("out_csg.obj")

-------------------------------------------------------- Boolean operations ---
-- Two overlapping spheres, this time through the mesh_* boolean API.
local a, b = geo.Mesh.new(), geo.Mesh.new()
geo.csg_evaluate_string("sphere(r = 10.0);", a, false)
-- geogram evaluates the *compiled* OpenSCAD format (.csg), where high-level
-- transforms are lowered to multmatrix.
geo.csg_evaluate_string(
    "multmatrix([[1,0,0,6],[0,1,0,0],[0,0,1,0],[0,0,0,1]]) " ..
    "{ sphere(r = 10.0); }",
    b,
    false)

local union = geo.Mesh.new()
geo.mesh_union(union, a, b, false)
local inter = geo.Mesh.new()
geo.mesh_intersection(inter, a, b, false)
local diff = geo.Mesh.new()
geo.mesh_difference(diff, a, b, false)
print(string.format("booleans   : union %d, intersection %d, difference %d facets",
                    union.facets:nb(), inter.facets:nb(), diff.facets:nb()))
union:save("out_union.obj")

---------------------------------------------------------------- Remeshing ---
-- Isotropic remesh of the union to 5000 points.
local remeshed = geo.Mesh.new()
-- (M_in, M_out, nb_points, dim, Lloyd iters, Newton iters, Newton m,
--  adjust, adjust_max_edge_distance, adjust_border_importance) — rosetta
-- does not capture C++ default arguments, so every parameter is explicit.
geo.remesh_smooth(union, remeshed, 5000, 0, 5, 30, 7, true, 0.5, 2.0)
print(string.format("remeshing  : union remeshed to %d vertices, %d facets",
                    remeshed.vertices:nb(), remeshed.facets:nb()))
remeshed:save("out_remeshed.obj")

---------------------------------------- Parameterization and texturing ---
-- Build a UV atlas of the remeshed surface (LSCM + xatlas packing) and
-- read the texture coordinates back (6 values per triangle).
geo.mesh_make_atlas(remeshed, 45.0, geo.ChartParameterizer.PARAM_LSCM,
                    geo.ChartPacker.PACK_XATLAS, false)
local uv = remeshed:tex_coords()
local tri = remeshed:triangles()
assert(#uv == 2 * #tri, "one (u,v) per triangle corner")
local uv_min, uv_max = math.huge, -math.huge
for i = 1, #uv do
    uv_min = math.min(uv_min, uv[i])
    uv_max = math.max(uv_max, uv[i])
end
print(string.format("texturing  : %d charts, %d UV corners in [%.3f, %.3f]",
                    geo.mesh_get_charts(remeshed), #uv // 2, uv_min, uv_max))

---------------------------------------------------- Surface reconstruction ---
-- Turn the remeshed surface into a bare point cloud, then reconstruct a
-- surface from the points alone with Co3Ne (smooth + co-cone triangles).
-- Geometry moves through geogram's own API: remeshing can leave
-- higher-dimensional points (normals appended), so truncate to xyz before
-- reading the flat coordinate array back.
remeshed.vertices:set_dimension(3)
local points = geo.Mesh.new()
points.vertices:assign_points(remeshed.vertices:point_coordinates(), 3, false)
print(string.format("pointcloud : %d points, %d facets",
                    points.vertices:nb(), points.facets:nb()))

-- radius ~ average spacing x a few; the sphere pair is ~36 across.
geo.Co3Ne_smooth_and_reconstruct(points, 30, 2, 2.0)
print(string.format("reconstruct: Co3Ne rebuilt %d facets from the point cloud",
                    points.facets:nb()))
points:save("out_reconstructed.obj")

---------------------------------------------------------------- Repair ---
geo.mesh_repair(points, geo.MeshRepairMode.MESH_REPAIR_DEFAULT, 0.0)
geo.fill_holes(points, 1e30, 2000, true)
print(string.format("repair     : after repair + fill_holes -> %d facets",
                    points.facets:nb()))

print("OK")
