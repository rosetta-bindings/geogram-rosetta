#include "Mesh.h"
#include "algorithms.h"

#include <geogram/mesh/mesh.h>
#include <geogram/mesh/mesh_io.h>

namespace georo {

    GEO::Mesh &impl(Mesh &m) {
        return *m.impl_;
    }

    const GEO::Mesh &impl(const Mesh &m) {
        return *m.impl_;
    }

    Mesh::Mesh() {
        initialize(false);
        impl_ = new GEO::Mesh(3, false);
    }

    Mesh::~Mesh() {
        delete impl_;
    }

    bool Mesh::load(const std::string &filename) {
        return GEO::mesh_load(filename, *impl_);
    }

    bool Mesh::save(const std::string &filename) const {
        return GEO::mesh_save(*impl_, filename);
    }

    void Mesh::clear() {
        impl_->clear(false, false);
    }

    void Mesh::assign(const Mesh &other) {
        impl_->copy(*other.impl_, true);
    }

    void Mesh::set_points(const std::vector<double> &coordinates) {
        impl_->clear(false, false);
        GEO::index_t nv = GEO::index_t(coordinates.size() / 3);
        impl_->vertices.set_dimension(3);
        impl_->vertices.create_vertices(nv);
        for (GEO::index_t v = 0; v < nv; ++v) {
            double *p = impl_->vertices.point_ptr(v);
            p[0] = coordinates[3 * v];
            p[1] = coordinates[3 * v + 1];
            p[2] = coordinates[3 * v + 2];
        }
    }

    void Mesh::set_surface(const std::vector<double> &coordinates,
                           const std::vector<int> &triangles) {
        set_points(coordinates);
        GEO::index_t nt = GEO::index_t(triangles.size() / 3);
        impl_->facets.create_triangles(nt);
        for (GEO::index_t t = 0; t < nt; ++t) {
            impl_->facets.set_vertex(t, 0, GEO::index_t(triangles[3 * t]));
            impl_->facets.set_vertex(t, 1, GEO::index_t(triangles[3 * t + 1]));
            impl_->facets.set_vertex(t, 2, GEO::index_t(triangles[3 * t + 2]));
        }
        impl_->facets.connect();
    }

    std::vector<double> Mesh::vertices() const {
        GEO::index_t nv = impl_->vertices.nb();
        std::vector<double> result(3 * nv);
        for (GEO::index_t v = 0; v < nv; ++v) {
            const double *p = impl_->vertices.point_ptr(v);
            result[3 * v] = p[0];
            result[3 * v + 1] = p[1];
            result[3 * v + 2] = p[2];
        }
        return result;
    }

    std::vector<int> Mesh::triangles() const {
        std::vector<int> result;
        for (GEO::index_t f = 0; f < impl_->facets.nb(); ++f) {
            // Fan-triangulate (a no-op on triangles).
            GEO::index_t v0 = impl_->facets.vertex(f, 0);
            for (GEO::index_t lv = 1; lv + 1 < impl_->facets.nb_vertices(f);
                 ++lv) {
                result.push_back(int(v0));
                result.push_back(int(impl_->facets.vertex(f, lv)));
                result.push_back(int(impl_->facets.vertex(f, lv + 1)));
            }
        }
        return result;
    }

    std::vector<double> Mesh::tex_coords() const {
        std::vector<double> result;
        GEO::Attribute<double> tex_coord;
        tex_coord.bind_if_is_defined(
            impl_->facet_corners.attributes(), "tex_coord"
        );
        if (!tex_coord.is_bound() || tex_coord.dimension() != 2) {
            return result;
        }
        for (GEO::index_t f = 0; f < impl_->facets.nb(); ++f) {
            // Same fan order as triangles(), one (u,v) per corner.
            GEO::index_t c0 = impl_->facets.corners_begin(f);
            for (GEO::index_t lv = 1; lv + 1 < impl_->facets.nb_vertices(f);
                 ++lv) {
                for (GEO::index_t c :
                     {c0, c0 + lv, c0 + lv + 1}) {
                    result.push_back(tex_coord[2 * c]);
                    result.push_back(tex_coord[2 * c + 1]);
                }
            }
        }
        return result;
    }

    int Mesh::nb_vertices() const {
        return int(impl_->vertices.nb());
    }

    int Mesh::nb_facets() const {
        return int(impl_->facets.nb());
    }

} // namespace georo
