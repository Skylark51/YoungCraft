# Refactor validation

- Structural refactor only; no intended feature change.
- Original member-function bodies were relocated without edits for:
  - `MainWindow`: 34/34 matched
  - `MoleculeView`: 51/51 matched
  - `AnalysisPanel`: 27/27 matched
  - `StyleEditorDialog`: 13/13 matched
- Public headers/signals/slots were retained.
- `GaussianParser::parse` remains the same single-pass parser body; helper routines were moved to `src/io/gaussian/GaussianParserHelpers.inc`.
- Removed from source package: all `build*` trees, `dist`, existing portable ZIP, generated executables/DLLs, and PNG UI reference captures.
- CMake source lists were updated for the split translation units.

Note: this environment does not contain the Qt6 development package, so a fresh native compile could not be executed here. The refactor was validated structurally by exact member-function-body comparison and source/CMake consistency checks.
