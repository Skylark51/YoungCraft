# Youngcraft module map

Purpose: keep Codex edits local. Prefer touching only the module named for the requested behavior. Do not search or rewrite unrelated files unless compilation proves a dependency.

## Main window / workflow
- `src/app/MainWindow.cpp` — constructor/destructor only
- `src/app/mainwindow/MainWindow_Ui.cpp` — dock/widget wiring
- `.../MainWindow_Menus.cpp` — menu definitions
- `.../MainWindow_Toolbar.cpp` — toolbar
- `.../MainWindow_File.cpp` — open/load/recent/clear/title
- `.../MainWindow_Navigation.cpp` — explorer and frame/frequency/scan/IRC/atom selection
- `.../MainWindow_Animation.cpp` — frame/frequency playback and animation export
- `.../MainWindow_Style.cpp` — style editor/presets/perspective
- `.../MainWindow_Export.cpp` — copy/save image
- `.../MainWindow_Bonds.cpp` — bond edit/recount
- `.../MainWindow_Window.cpp` — status, keyboard, close behavior

## 3D renderer
- `src/render/MoleculeView.cpp` — lifetime only
- `src/render/moleculeview/MoleculeShaders.inc` — GLSL shaders only
- `.../MoleculeView_State.cpp` — molecule/style/coordinates/image state
- `.../MoleculeView_Camera.cpp` — framing, camera, projection/bounds
- `.../MoleculeView_GL.cpp` — OpenGL initialization/programs/meshes
- `.../MoleculeView_Surface.cpp` — cube/isosurface buffers
- `.../MoleculeView_Shadows.cpp` — shadow pass only
- `.../MoleculeView_AtomsBonds.cpp` — atom colors/radii and bond drawing
- `.../MoleculeView_Scene.cpp` — main scene draw pass
- `.../MoleculeView_Overlays.cpp` — labels/vibration vectors
- `.../MoleculeView_Interaction.cpp` — mouse/picking/pan/rotate/zoom

## Analysis panel
- `src/ui/AnalysisPanel.cpp` — construction/data/navigation
- `src/ui/analysispanel/AnalysisPanel_Geometry.cpp` — optimization frames
- `.../AnalysisPanel_Frequency.cpp` — frequency UI/control
- `.../AnalysisPanel_Coordinates.cpp` — coordinate view
- `.../AnalysisPanel_ScanIrc.cpp` — scan/IRC plots
- `.../AnalysisPanel_Tddft.cpp` — TD-DFT table/spectrum
- `.../AnalysisPanel_AtomsOrbitals.cpp` — atom/orbital tables

## Style editor
- `src/ui/StyleEditorDialog.cpp` — dialog lifetime/apply/save
- `src/ui/styleeditor/StyleEditor_Lighting.cpp` — light/background
- `.../StyleEditor_Atoms.cpp` — atom display/colors
- `.../StyleEditor_Bonds.cpp` — bond style
- `.../StyleEditor_Isosurface.cpp` — isosurface/simple tabs

## Parsers
- `src/io/GaussianParser.cpp` — single-pass parse state machine
- `src/io/gaussian/GaussianParserHelpers.inc` — parsing helper routines

### Codex rule
Start with `docs/MODULE_MAP.md`. For a localized request, open only the mapped `.h/.cpp/.inc` files plus `CMakeLists.txt` only if a new source file is truly required. Avoid repository-wide searches by default.
