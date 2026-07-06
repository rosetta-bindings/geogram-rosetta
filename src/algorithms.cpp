#include "algorithms.h"

#include <geogram/NL/nl.h>
#include <geogram/basic/command_line.h>
#include <geogram/basic/command_line_args.h>
#include <geogram/basic/logger.h>
#include <geogram/mesh/mesh_CSG.h>
#include <geogram/mesh/mesh_surface_intersection.h>

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

    // --- Boolean operations -----------------------------------------------

    void mesh_union(GEO::Mesh &result, const GEO::Mesh &A, const GEO::Mesh &B,
                    bool verbose) {
        initialize(false);
        GEO::mesh_boolean_operation(result, A, B, "A+B", verbose);
    }

    void mesh_intersection(GEO::Mesh &result, const GEO::Mesh &A,
                           const GEO::Mesh &B, bool verbose) {
        initialize(false);
        GEO::mesh_boolean_operation(result, A, B, "A*B", verbose);
    }

    void mesh_difference(GEO::Mesh &result, const GEO::Mesh &A,
                         const GEO::Mesh &B, bool verbose) {
        initialize(false);
        GEO::mesh_boolean_operation(result, A, B, "A-B", verbose);
    }

    // --- Constructive Solid Geometry -------------------------------------

    bool csg_evaluate_string(const std::string &source, GEO::Mesh &result,
                             bool verbose) {
        initialize(verbose);
        GEO::CSGCompiler compiler;
        compiler.set_verbose(verbose);
        std::shared_ptr<GEO::Mesh> M = compiler.compile_string(source);
        if (M == nullptr) {
            return false;
        }
        result.copy(*M, false);
        return true;
    }

    bool csg_evaluate_file(const std::string &path, GEO::Mesh &result,
                           bool verbose) {
        initialize(verbose);
        GEO::CSGCompiler compiler;
        compiler.set_verbose(verbose);
        std::shared_ptr<GEO::Mesh> M = compiler.compile_file(path);
        if (M == nullptr) {
            return false;
        }
        result.copy(*M, false);
        return true;
    }

} // namespace georo
