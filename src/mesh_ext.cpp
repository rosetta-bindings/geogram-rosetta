#include "mesh_ext.h"

#include <geogram/basic/attributes.h>

namespace georo {

    std::vector<int> triangles(const GEO::Mesh &M) {
        if (M.facets.are_simplices()) {
            const GEO::vector<GEO::index_t> &v =
                M.facet_corners.vertex_indices();
            return std::vector<int>(v.begin(), v.end());
        }
        std::vector<int> result;
        for (GEO::index_t f = 0; f < M.facets.nb(); ++f) {
            // Fan-triangulate polygonal facets.
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
        GEO::vector<double> uv;
        GEO::index_t dim = 0;
        if (!M.facet_corners.attributes().get_doubles("tex_coord", uv, dim) ||
            dim != 2) {
            return {};
        }
        if (M.facets.are_simplices()) {
            return std::vector<double>(uv.begin(), uv.end());
        }
        std::vector<double> result;
        for (GEO::index_t f = 0; f < M.facets.nb(); ++f) {
            // Same fan order as triangles(), one (u,v) per corner.
            GEO::index_t c0 = M.facets.corners_begin(f);
            for (GEO::index_t lv = 1; lv + 1 < M.facets.nb_vertices(f); ++lv) {
                for (GEO::index_t c : {c0, c0 + lv, c0 + lv + 1}) {
                    result.push_back(uv[2 * c]);
                    result.push_back(uv[2 * c + 1]);
                }
            }
        }
        return result;
    }

} // namespace georo
