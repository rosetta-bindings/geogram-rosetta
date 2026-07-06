#!/usr/bin/env python3
"""geogram — desktop 3D demo (the index.html viewer, in Python).

Same workflow as the browser demo, but through the native Python binding
and a pyvista/Qt window: load data/Intergalactic_Spaceship.obj (or any
mesh geogram can read), run repair / fill_holes / CVT remeshing on the
GEO::Mesh, and view the result plain, with a wireframe overlay, with
vertex points, or flat-shaded.

Build the Python target first (see README step 3), then:

    pip install pyvista pyvistaqt PyQt5   # or PySide6
    python3 example_python_GUI.py

Unlike the browser (no filesystem in wasm, OBJ parsed in JS), here the
mesh is read natively with mesh.load(), so any geogram format works
(.obj, .off, .ply, .stl, .mesh/.meshb, .geogram). Algorithms run
synchronously on the UI thread — same as the wasm demo — so the window
freezes during a big remesh; the log prints per-operation timings.
"""

import sys
import time
from pathlib import Path

import numpy as np
import pyvista as pv
from pyvistaqt import QtInteractor
from qtpy import QtCore, QtWidgets

sys.path.insert(0, str(Path(__file__).parent / "bindings" / "python-expanded"))
import geogram as geo

DATA = Path(__file__).parent / "data" / "Intergalactic_Spaceship.obj"


def to_polydata(mesh: "geo.Mesh") -> pv.PolyData:
    """GEO::Mesh -> pyvista surface (flat arrays -> (n,3) + VTK face list)."""
    verts = np.asarray(mesh.vertices()).reshape(-1, 3)
    tris = np.asarray(mesh.triangles(), dtype=np.int64).reshape(-1, 3)
    faces = np.hstack([np.full((len(tris), 1), 3, dtype=np.int64), tris])
    return pv.PolyData(verts, faces.ravel())


class Viewer(QtWidgets.QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("geogram x Rosetta — Python 3D demo")
        self.resize(1280, 800)

        self.mesh = geo.Mesh()      # the working GEO::Mesh
        self.original = geo.Mesh()  # pristine copy for Reset
        self.loaded = False

        # ---------------------------------------------------------- panel
        panel = QtWidgets.QWidget()
        panel.setFixedWidth(340)
        lay = QtWidgets.QVBoxLayout(panel)

        def row(*widgets):
            h = QtWidgets.QHBoxLayout()
            for w in widgets:
                h.addWidget(w)
            lay.addLayout(h)

        self.b_load = QtWidgets.QPushButton(f"Load {DATA.name}")
        self.b_open = QtWidgets.QPushButton("Open…")
        row(self.b_load, self.b_open)

        self.b_repair = QtWidgets.QPushButton("Repair")
        self.b_holes = QtWidgets.QPushButton("Fill holes")
        self.b_reset = QtWidgets.QPushButton("Reset")
        row(self.b_repair, self.b_holes, self.b_reset)

        self.n_points = QtWidgets.QSpinBox()
        self.n_points.setRange(1000, 200000)
        self.n_points.setSingleStep(1000)
        self.n_points.setValue(10000)
        self.b_remesh = QtWidgets.QPushButton("Remesh (CVT)")
        row(QtWidgets.QLabel("points"), self.n_points, self.b_remesh)

        self.n_lloyd = QtWidgets.QSpinBox()
        self.n_lloyd.setRange(0, 50)
        self.n_lloyd.setValue(5)
        self.n_newton = QtWidgets.QSpinBox()
        self.n_newton.setRange(0, 50)
        self.n_newton.setValue(10)
        row(QtWidgets.QLabel("Lloyd"), self.n_lloyd,
            QtWidgets.QLabel("Newton"), self.n_newton)

        self.c_wire = QtWidgets.QCheckBox("wireframe")
        self.c_points = QtWidgets.QCheckBox("points")
        self.c_flat = QtWidgets.QCheckBox("flat shading")
        row(self.c_wire, self.c_points, self.c_flat)
        self.b_fit = QtWidgets.QPushButton("Fit view")
        row(self.b_fit)

        self.stats = QtWidgets.QLabel("no mesh")
        lay.addWidget(self.stats)
        self.log_box = QtWidgets.QPlainTextEdit()
        self.log_box.setReadOnly(True)
        lay.addWidget(self.log_box, stretch=1)

        # ------------------------------------------------------- viewport
        self.plotter = QtInteractor(self)
        self.plotter.set_background("#14161a")

        central = QtWidgets.QWidget()
        h = QtWidgets.QHBoxLayout(central)
        h.setContentsMargins(0, 0, 0, 0)
        h.addWidget(panel)
        h.addWidget(self.plotter.interactor, stretch=1)
        self.setCentralWidget(central)

        # -------------------------------------------------------- wiring
        self.b_load.clicked.connect(lambda: self.load(DATA))
        self.b_open.clicked.connect(self.open_dialog)
        self.b_repair.clicked.connect(lambda: self.run(
            "mesh_repair", lambda: geo.mesh_repair(
                self.mesh, geo.MeshRepairMode.MESH_REPAIR_DEFAULT, 0.0)))
        self.b_holes.clicked.connect(lambda: self.run(
            "fill_holes", lambda: geo.fill_holes(self.mesh, 1e30, 10000, True)))
        self.b_remesh.clicked.connect(self.remesh)
        self.b_reset.clicked.connect(lambda: self.run(
            "reset to original", lambda: self.mesh.copy(
                self.original, True, geo.MeshElementsFlags.MESH_ALL_ELEMENTS)))
        self.b_fit.clicked.connect(self.plotter.reset_camera)
        for c in (self.c_wire, self.c_points, self.c_flat):
            c.toggled.connect(lambda _=False: self.refresh())

        self.log("module loaded — click “Load” (or “Open…” for any mesh "
                 "geogram reads: .obj, .off, .ply, .stl, .mesh, .geogram).")
        geo.initialize(False)  # True for geogram's log output on stdout

    # ------------------------------------------------------------ helpers
    def log(self, msg: str) -> None:
        self.log_box.appendPlainText(msg)
        self.log_box.verticalScrollBar().setValue(
            self.log_box.verticalScrollBar().maximum())

    def run(self, label: str, fn, fit: bool = False) -> None:
        """Run one geogram operation with timing + log, then redraw."""
        if not self.loaded:
            self.log("load a mesh first")
            return
        self.log(label + "…")
        self.setEnabled(False)
        QtWidgets.QApplication.processEvents()  # let the log line paint
        t = time.perf_counter()
        try:
            fn()
            self.log(f"  done ({time.perf_counter() - t:.1f} s)")
            self.refresh(fit=fit)
        except Exception as e:  # geogram throws std::exception on bad input
            self.log(f"  failed: {e}")
        finally:
            self.setEnabled(True)

    # --------------------------------------------------------------- I/O
    def open_dialog(self) -> None:
        path, _ = QtWidgets.QFileDialog.getOpenFileName(
            self, "Open mesh", str(DATA.parent),
            "Meshes (*.obj *.off *.ply *.stl *.mesh *.meshb *.geogram)")
        if path:
            self.load(Path(path))

    def load(self, path: Path) -> None:
        self.log(f"loading {path.name}…")
        QtWidgets.QApplication.processEvents()
        t = time.perf_counter()
        if not self.mesh.load(str(path)):
            self.log(f"  failed to load {path}")
            return
        self.original.copy(self.mesh, True,
                           geo.MeshElementsFlags.MESH_ALL_ELEMENTS)
        self.loaded = True
        self.log(f"  loaded ({time.perf_counter() - t:.1f} s)")
        self.refresh(fit=True)

    # ---------------------------------------------------------- algorithms
    def remesh(self) -> None:
        n = self.n_points.value()
        lloyd, newton = self.n_lloyd.value(), self.n_newton.value()

        def op():
            out = geo.Mesh()
            # (M_in, M_out, nb_points, dim, Lloyd, Newton, Newton_m, adjust,
            #  adjust_max_edge_distance, adjust_border_importance) — rosetta
            # does not capture C++ defaults, so every parameter is explicit.
            geo.remesh_smooth(self.mesh, out, n, 0, lloyd, newton, 7,
                              True, 0.5, 2.0)
            self.mesh.copy(out, True, geo.MeshElementsFlags.MESH_ALL_ELEMENTS)

        self.run(f"remesh_smooth to {n:,} points "
                 f"({lloyd} Lloyd + {newton} Newton)", op)

    # ------------------------------------------------------------- redraw
    def refresh(self, fit: bool = False) -> None:
        if not self.loaded:
            return
        poly = to_polydata(self.mesh)
        # name= replaces the previous actor of the same name in pyvista.
        self.plotter.add_mesh(
            poly, name="surface", color="#b8894a", metallic=0.35,
            roughness=0.5, pbr=True,
            smooth_shading=not self.c_flat.isChecked())
        wire = self.plotter.add_mesh(
            poly, name="wire", style="wireframe", color="#dddddd",
            opacity=0.35, line_width=2)
        pts = self.plotter.add_mesh(
            poly.points, name="cloud", color="#7fd0ff", point_size=3,
            render_points_as_spheres=False)
        wire.SetVisibility(self.c_wire.isChecked())
        pts.SetVisibility(self.c_points.isChecked())
        self.stats.setText(f"{self.mesh.nb_vertices():,} vertices · "
                           f"{self.mesh.nb_facets():,} facets")
        if fit:
            self.plotter.reset_camera()
        self.plotter.render()


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    viewer = Viewer()
    viewer.show()
    sys.exit(app.exec_())
