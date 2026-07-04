#include "algorithms.h"

#include <geogram/NL/nl.h>
#include <geogram/basic/command_line.h>
#include <geogram/basic/command_line_args.h>
#include <geogram/basic/logger.h>
#include <geogram/mesh/mesh.h>
#include <geogram/mesh/mesh_CSG.h>
#include <geogram/mesh/mesh_fill_holes.h>
#include <geogram/mesh/mesh_remesh.h>
#include <geogram/mesh/mesh_repair.h>
#include <geogram/mesh/mesh_surface_intersection.h>
#include <geogram/parameterization/mesh_atlas_maker.h>
#include <geogram/points/co3ne.h>

#include <algorithm>
#include <stdexcept>

namespace {

    std::string lowercase(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return char(std::tolower(c));
        });
        return s;
    }

} // namespace

namespace georo {

    void initialize(bool verbose) {
        static bool initialized = false;
        if (!initialized) {
            GEO::initialize(GEO::GEOGRAM_INSTALL_NONE);
            // The algorithms read their tuning parameters from the
            // command-line argument registry.
            GEO::CmdLine::import_arg_group("standard");
            GEO::CmdLine::import_arg_group("algo");
            GEO::CmdLine::import_arg_group("pre");
            GEO::CmdLine::import_arg_group("remesh");
            GEO::CmdLine::import_arg_group("post");
            GEO::CmdLine::import_arg_group("opt");
            GEO::CmdLine::import_arg_group("co3ne");
            // Route OpenNL's printf/fprintf through geogram's Logger (as
            // CmdLine::parse does in geogram's own programs) so solver
            // messages honor the quiet flag instead of hitting stderr.
            nlPrintfFuncs(geogram_printf, geogram_fprintf);
            initialized = true;
        }
        GEO::Logger::instance()->set_quiet(!verbose);
    }

    // --- Surface reconstruction -----------------------------------------

    void co3ne_smooth(Mesh &points, int nb_neighbors, int nb_iterations) {
        initialize(false);
        GEO::Co3Ne_smooth(impl(points), GEO::index_t(nb_neighbors),
                          GEO::index_t(nb_iterations));
    }

    void co3ne_smooth_and_reconstruct(Mesh &points, int nb_neighbors,
                                      int nb_iterations, double radius) {
        initialize(false);
        GEO::Co3Ne_smooth_and_reconstruct(
            impl(points), GEO::index_t(nb_neighbors),
            GEO::index_t(nb_iterations), radius
        );
    }

    // --- Remeshing --------------------------------------------------------

    void remesh_smooth(Mesh &input, Mesh &output, int nb_points,
                       int nb_lloyd_iter, int nb_newton_iter, int newton_m) {
        initialize(false);
        GEO::remesh_smooth(impl(input), impl(output),
                           GEO::index_t(nb_points), 0,
                           GEO::index_t(nb_lloyd_iter),
                           GEO::index_t(nb_newton_iter),
                           GEO::index_t(newton_m));
    }

    // --- Repair -------------------------------------------------------------

    void mesh_repair(Mesh &M, double colocate_epsilon) {
        initialize(false);
        GEO::mesh_repair(impl(M), GEO::MESH_REPAIR_DEFAULT, colocate_epsilon);
    }

    void fill_holes(Mesh &M, double max_area, int max_edges) {
        initialize(false);
        GEO::fill_holes(impl(M), max_area, GEO::index_t(max_edges));
    }

    void tessellate_facets(Mesh &M, int max_nb_vertices) {
        initialize(false);
        GEO::tessellate_facets(impl(M), GEO::index_t(max_nb_vertices));
    }

    // --- Parameterization and texturing --------------------------------

    void make_atlas(Mesh &M, double hard_angles_threshold,
                    const std::string &parameterizer, const std::string &packer,
                    bool verbose) {
        initialize(false);

        GEO::ChartParameterizer param = GEO::PARAM_ABF;
        const std::string p = lowercase(parameterizer);
        if (p == "projection") {
            param = GEO::PARAM_PROJECTION;
        } else if (p == "lscm") {
            param = GEO::PARAM_LSCM;
        } else if (p == "spectral") {
            param = GEO::PARAM_SPECTRAL_LSCM;
        } else if (p == "abf") {
            param = GEO::PARAM_ABF;
        } else {
            throw std::invalid_argument(
                "make_atlas: parameterizer must be one of "
                "\"projection\", \"lscm\", \"spectral\", \"abf\" (got \"" +
                parameterizer + "\")"
            );
        }

        GEO::ChartPacker pack = GEO::PACK_XATLAS;
        const std::string k = lowercase(packer);
        if (k == "none") {
            pack = GEO::PACK_NONE;
        } else if (k == "tetris") {
            pack = GEO::PACK_TETRIS;
        } else if (k == "xatlas") {
            pack = GEO::PACK_XATLAS;
        } else {
            throw std::invalid_argument(
                "make_atlas: packer must be one of "
                "\"none\", \"tetris\", \"xatlas\" (got \"" + packer + "\")"
            );
        }

        GEO::mesh_make_atlas(impl(M), hard_angles_threshold, param, pack,
                             verbose);
    }

    int get_charts(Mesh &M) {
        initialize(false);
        return int(GEO::mesh_get_charts(impl(M)));
    }

    // --- Intersections and Boolean operations ----------------------------

    void mesh_union(Mesh &result, const Mesh &A, const Mesh &B, bool verbose) {
        initialize(false);
        GEO::mesh_boolean_operation(impl(result), impl(A), impl(B), "A+B",
                                    verbose);
    }

    void mesh_intersection(Mesh &result, const Mesh &A, const Mesh &B,
                           bool verbose) {
        initialize(false);
        GEO::mesh_boolean_operation(impl(result), impl(A), impl(B), "A*B",
                                    verbose);
    }

    void mesh_difference(Mesh &result, const Mesh &A, const Mesh &B,
                         bool verbose) {
        initialize(false);
        GEO::mesh_boolean_operation(impl(result), impl(A), impl(B), "A-B",
                                    verbose);
    }

    void mesh_remove_intersections(Mesh &M, int max_iter, bool verbose) {
        initialize(false);
        GEO::mesh_remove_intersections(impl(M), GEO::index_t(max_iter),
                                       verbose);
    }

    bool mesh_facets_have_intersection(Mesh &M, int f1, int f2) {
        initialize(false);
        return GEO::mesh_facets_have_intersection(
            impl(M), GEO::index_t(f1), GEO::index_t(f2)
        );
    }

    // --- Constructive Solid Geometry -------------------------------------

    bool csg_evaluate_string(const std::string &source, Mesh &result,
                             bool verbose) {
        initialize(verbose);
        GEO::CSGCompiler compiler;
        compiler.set_verbose(verbose);
        std::shared_ptr<GEO::Mesh> M = compiler.compile_string(source);
        if (M == nullptr) {
            return false;
        }
        impl(result).copy(*M, false);
        return true;
    }

    bool csg_evaluate_file(const std::string &path, Mesh &result,
                           bool verbose) {
        initialize(verbose);
        GEO::CSGCompiler compiler;
        compiler.set_verbose(verbose);
        std::shared_ptr<GEO::Mesh> M = compiler.compile_file(path);
        if (M == nullptr) {
            return false;
        }
        impl(result).copy(*M, false);
        return true;
    }

} // namespace georo
