#include "mesh_ext.h"
#include "algorithms.h"

#include <geogram/basic/attributes.h>
#include <geogram/mesh/mesh_io.h>

namespace georo {

    bool load(GEO::Mesh &M, const std::string &filename) {
        initialize(false); // mesh I/O needs geogram initialized
        return GEO::mesh_load(filename, M);
    }

    bool save(const GEO::Mesh &M, const std::string &filename) {
        initialize(false);
        return GEO::mesh_save(M, filename);
    }

    void set_points(GEO::Mesh &M, const std::vector<double> &coordinates) {
        initialize(false);
        M.clear(false, false);
        GEO::index_t nv = GEO::index_t(coordinates.size() / 3);
        M.vertices.set_dimension(3);
        M.vertices.create_vertices(nv);
        for (GEO::index_t v = 0; v < nv; ++v) {
            double *p = M.vertices.point_ptr(v);
            p[0] = coordinates[3 * v];
            p[1] = coordinates[3 * v + 1];
            p[2] = coordinates[3 * v + 2];
        }
    }

    void set_surface(GEO::Mesh &M, const std::vector<double> &coordinates,
                     const std::vector<int> &triangles) {
        set_points(M, coordinates);
        GEO::index_t nt = GEO::index_t(triangles.size() / 3);
        M.facets.create_triangles(nt);
        for (GEO::index_t t = 0; t < nt; ++t) {
            M.facets.set_vertex(t, 0, GEO::index_t(triangles[3 * t]));
            M.facets.set_vertex(t, 1, GEO::index_t(triangles[3 * t + 1]));
            M.facets.set_vertex(t, 2, GEO::index_t(triangles[3 * t + 2]));
        }
        M.facets.connect();
    }

    std::vector<double> vertices(const GEO::Mesh &M) {
        GEO::index_t nv = M.vertices.nb();
        std::vector<double> result(3 * nv);
        for (GEO::index_t v = 0; v < nv; ++v) {
            const double *p = M.vertices.point_ptr(v);
            result[3 * v] = p[0];
            result[3 * v + 1] = p[1];
            result[3 * v + 2] = p[2];
        }
        return result;
    }

    std::vector<int> triangles(const GEO::Mesh &M) {
        std::vector<int> result;
        for (GEO::index_t f = 0; f < M.facets.nb(); ++f) {
            // Fan-triangulate (a no-op on triangles).
            GEO::index_t v0 = M.facets.vertex(f, 0);
            for (GEO::index_t lv = 1; lv + 1 < M.facets.nb_vertices(f); ++lv) {
                result.push_back(int(v0));
                result.push_back(int(M.facets.vertex(f, lv)));
                result.push_back(int(M.facets.vertex(f, lv + 1)));
            }
        }
        return result;
    }

    std::vector<double> tex_coords(const GEO::Mesh &M) {
        std::vector<double> result;
        GEO::Attribute<double> tex_coord;
        // Attribute binding needs a non-const manager even for reading.
        tex_coord.bind_if_is_defined(
            const_cast<GEO::Mesh &>(M).facet_corners.attributes(), "tex_coord"
        );
        if (!tex_coord.is_bound() || tex_coord.dimension() != 2) {
            return result;
        }
        for (GEO::index_t f = 0; f < M.facets.nb(); ++f) {
            // Same fan order as triangles(), one (u,v) per corner.
            GEO::index_t c0 = M.facets.corners_begin(f);
            for (GEO::index_t lv = 1; lv + 1 < M.facets.nb_vertices(f); ++lv) {
                for (GEO::index_t c : {c0, c0 + lv, c0 + lv + 1}) {
                    result.push_back(tex_coord[2 * c]);
                    result.push_back(tex_coord[2 * c + 1]);
                }
            }
        }
        return result;
    }

    int nb_vertices(const GEO::Mesh &M) {
        return int(M.vertices.nb());
    }

    int nb_facets(const GEO::Mesh &M) {
        return int(M.facets.nb());
    }

} // namespace georo
