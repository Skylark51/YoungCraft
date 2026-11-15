# November 2026 development evidence (in progress)

## Baseline

Before this work, Gaussian and ORCA parsers, Opt/Freq/Scan/IRC/TD-DFT panels, frame animation, multi-document editing, project libraries, MM/xTB, and Gemmi CIF symmetry/packing import already existed. The save chooser supported images and XYZ, without CIF export. Starting an edit copied a CIF into an ordinary drawing document and detached its crystallographic metadata.

## Changes under validation

- Reject missing/non-numeric CIF atom coordinates instead of placing atoms at zero.
- Preserve the full parsed CIF document (including unrecognized tags and additional blocks) while replacing edited coordinate tokens. Preserve original precision/uncertainties for unchanged sites. Use atomic file replacement and reject unsupported site identity/count changes.
- Expose CIF save for the asymmetric unit; describe retained, unrecomputed derived data in the save status.
- Retain CIF metadata when entering asymmetric-unit editing. Rebuild symmetry mates and packing from edited coordinates on display-mode changes; retain the current view and explain failures if unsupported edits prevent rebuilding.
- Synchronize IRC and scan selection when navigating frames; avoid highlighting another frame's energy when the current frame has no energy.
- Restore frame-linked selection after all analysis lists have been populated.
- Correct an environment-dependent pre-existing orbital test expectation: an existing user SSH config is intentionally retained.

## Evidence and remaining gates

- Initial Windows Release build passed before the latest integration changes.
- Console-only structure regression: 7 passed, 0 failed, including four repository CIF/mmCIF round trips, malformed coordinates, metadata/multiple-block retention and atomic failure preservation. Latest symmetry-rebuild assertions are being rebuilt and verified.
- Initial CTest passed Gaussian, ORCA, structure, model, structure-tools, mechanism and single-instance suites. Orbital test exposed the SSH configuration expectation above. GUI suite was interrupted at the user's request because child processes caused visible flickering.
- Do not run GUI tests, application launches, SSH helpers or unrestricted CTest locally. Offscreen Qt does not guarantee invisible child processes. Further local validation must use console-only tests and hidden build processes.
- Required real-file corpus (20 Gaussian/ORCA and 10 CIF), comprehensive panel mapping, rendering regression, latest Release build, packaging/install and GitHub commit/push/merge are not yet complete.
- No completion claim is made by this report.

## December sequence

After finishing and validating CIF editing and periodic contacts: symmetry-aware neighbor/contact classification; Hirshfeld surface validation against reference structures; fingerprint plots with explicit surface provenance; energy frameworks only after independently validating the interaction-energy model and licensing.

## Public corpus evidence

The fetched corpus is stored outside versioned sources in `.build/november-corpus`. Run `python tools/fetch_november_corpus.py .build/november-corpus` to reproduce downloads. The source manifest pins cclib-data to commit `2aafcd411329005cda9a436e23efddfe1ab92f66`; `provenance.json` records SHA256 for all downloaded bytes. COD files retain their embedded authorship and revision records. Sources: https://github.com/cclib/cclib-data and https://www.crystallography.net/cod/1000041.html (COD entries 1000041–1000050).

The console-only `test_november_corpus` successfully read 20 calculation outputs and read/edited/re-saved all 10 real CIFs. It checked all asymmetric-site coordinates, occupancies, disorder groups, six cell parameters and space group, and checked packing size after re-expansion. Calculation checks covered finite coordinates, frame atom counts and valid scan/IRC frame references. This is a basic structural invariant pass, not proof of complete analysis extraction or panel alignment.

Inspection of the extraction counts revealed missing ORCA TD-DFT states in actual files (`dvb_td.out`, `1-ttt_td.out`); the parser does not yet extract them. ORCA normal-mode displacement/intensity coverage and scan/IRC parsing also require further work. These findings prevent declaring the analysis environment complete despite successful file reads.

The latest integration Release build and structure regression, including rebuild assertions, passed. The public-corpus console test reported 3 QtTest passes, 0 failures (one test processing all 30 files plus setup/cleanup). No graphical tests were launched in this continuation.

## ORCA TD-DFT extraction

Added parsing of TD-DFT/TDA singlet/triplet state energies, signed orbital coefficients and the electric-dipole absorption table. Separate labels retain spin identity when original state numbers repeat; spectrum numbering follows the combined table. Spin-forbidden rows are represented with zero electric-dipole oscillator strength. Velocity-dipole tables do not overwrite electric-dipole values. State records carry their source geometry frame index. The analysis table displays spin labels.

Console parser regression: 12 passed, 0 failed. Public corpus: 3 passes, 0 failures across 30 files. Actual ORCA `dvb_td.out` now has 10 states (5 singlets + 5 triplets), and `1-ttt_td.out` has 24. Assertions compare `5.352 eV`, `f=1.165369790`, triplet labels and spin-forbidden strength against the source output. Latest UI Release build, TD plot-to-state interaction and ORCA mode displacements remain pending.

## ORCA frequency extraction

Implemented complete Cartesian normal-mode block parsing (commit a displacement column only after all 3N components are present), IR intensity and Raman activity tables, and explicit IR units in the mode details. Old ORCA T**2 values retain their printed convention. Modern ORCA tables use the Int column in km/mol, without confusing it with eps or atomic-unit T**2. Format reference: https://www.faccts.de/docs/orca/6.0/manual/contents/typical/properties.html .

Console regression: 13 passes, no failures, including two-mode signed displacement vectors, IR/Raman values and a modern multi-column IR table. The 30-file public corpus passed again, including exact assertions for the first atom's mode-6 z displacement (0.078859), IR value (0.030792), Raman activity (2.483069), and displacement atom counts. An incremental rebuild is running after final source normalization; the UI Release build and graphical interaction checks are still pending. No GUI was launched.

## TD-DFT selection integration (under build)

Added discrete selection points to PlotWidget, independent of broadened curve samples, with selection highlighting and reset semantics. TD spectra now use transition rows for picking; table selection highlights the corresponding transition and updates the associated geometry frame without leaving the spectrum workspace. Gaussian states now retain their source frame index as ORCA states do. Spectrum broadening/normalization retains the selected table row.

Added a GUI regression that clicks a discrete transition between curve samples and verifies its row index and reset behavior. It is compiled locally but must only execute on remote CI. The new `november-analysis.yml` workflow builds Windows Release, checks the checksum-pinned corpus and runs selected existing rendering/editing/analysis regressions on the remote Windows runner. It has not been pushed or run yet. The local hidden build process is still active; no success is claimed for these latest UI changes.

## Periodic geometric contacts

Added a crystal-toolbar contact dialog with adjustable cutoff and a read-only atom-pair/lattice-translation/distance table. It uses the symmetry-expanded unit cell, rebuilds an edited asymmetric unit first, and enumerates all translations bounded by reciprocal cell lengths. Thus skew cells and cutoffs spanning multiple short lattice periods are covered. Inverse duplicates are removed. These are geometric atom contacts including covalent neighbors, not automatic hydrogen-bond or interaction-energy assignments.

The Release application and structure target built; console structure tests reported 8 passes, no failures, covering a 1.6 Å cell-boundary contact, a second lattice neighbor at 4 Å and a skew-cell contact. A final incremental rebuild checks the last interactive-search bounds guard. The contact UI has not been run locally. Remote CI for the preceding commit is still building.

## Integrity follow-up

ORCA geometry convergence RMS/MAX gradient and displacement values now attach to their current frame. Synthetic assertions cover all four metrics; the real CASSCF optimization fixture verifies frame 2 against the printed values. CIF export updates both fractional and Cartesian columns when both exist and rejects incomplete alternate coordinate sets. Console runs: ORCA 13 passes; structure 9 passes; public corpus 3 passes, with no failures.

Remote run 43766208458 built successfully and passed nine of ten console/calculation suites. xTB failed: the new workflow lacked the real runtime required by existing integration tests. Added the same SHA256-verified runtime provisioning used by the portable release workflow, explicit xTB text output, and independent remote GUI execution after a successful build even when a separate console suite fails. Subsequent remote verification is required; no GUI success is claimed yet.

## Full reaction-path corpus

Expanded the checksum-pinned corpus to 22 calculation outputs plus 10 COD CIFs with an actual 103-point Gaussian LQA IRC (TS plus 102 points) and a 12-point ORCA 4.2 relaxed dihedral scan. The new assertions first reproduced missing IRC and scan extraction.

Gaussian now recognizes printed path-direction lines and accumulated reaction coordinates, associating each completed path point with its evaluated frame and energy. Legacy IRC fallback uses each frame's own energy rather than aligning separate array tails. The real IRC checks 103 points, first nonzero coordinate 0.23513, final coordinate 23.17564, and exact frame-energy correspondence.

ORCA relaxed scan parsing retains axis coordinates, atom indices, per-point optimization histories, converged/in-progress state and final evaluated frames. The real scan checks 12 points, endpoints 0/360 degrees, source endpoint energies, completed status, and exact point-to-frame energy/coordinate matching (134 frames). Console regression: Gaussian 16 passes/1 optional skip, ORCA 13 passes, corpus 3 passes/0 failures across 32 files. Remote integration for the latest path changes and final application/package rebuild remain required.

## CIF workflow and unsupported-data safeguards

CIF export now explicitly rejects edited molecular/formal charges, multiplicity and explicit bonds, which cannot currently be serialized back into all relevant CIF categories. Unknown space-group names without explicit symmetry operations fail instead of silently substituting identity symmetry; explicit operations remain usable with an unknown descriptive name. Console structure regression: 10 passes, no failures.

Added a GUI workflow regression for opening CIF, coordinate edit without detaching metadata, unit-cell/asymmetric display round trip, undo/redo, and the actual CIF save chooser/file dialog followed by reload. It compiles in Windows Release but has not executed locally; it is queued for remote CI. The application and GUI test executable compiled successfully. Remote run 43767759353 has completed its pinned xTB backend build and is compiling YoungCraft. Wait for that run before pushing further CI-triggering changes, to retain its verification evidence.

## Remote follow-up and ORCA IRC

Remote run 43767759353 passed all ten console/calculation suites, including real xTB. Discrete spectrum selection and optimization graph opening passed. The existing optimizationGraphSynchronizes assertion expected the previous incorrect highlighting of another frame for a final frame without energy; corrected it. GUI tests now run in separate processes with individual logs.

ORCA IRC summary parsing attaches the documented full XYZ trajectory only after checking frame count, atom count, element order and finite coordinates. Missing or mismatched sidecars produce warnings and unlinked summary points. The x axis is explicitly a path step, not a mass-weighted reaction coordinate. Synthetic console regression: 14 passes, no failures. Actual-file corpus remains 22 outputs plus 10 CIFs. Initial portable packaging succeeded but must be rebuilt with this IRC change before installation.

## Full CIF metadata comparison and portable preparation

The public corpus now compares parsed values of every CIF data block, save frame, pair and loop column before/after coordinate editing, excluding only the changed atom coordinate tokens. This includes explicit symmetry-operation strings, occupancies, disorder, unknown tags and uncertainty tokens of unchanged coordinates. All 32 files passed the strengthened console corpus test (3 QtTest passes, zero failures). Latest local console CTest: 4/4 passed.

Final-source portable packaging via tools/package_portable.ps1 -ReuseMmRuntime succeeded. Built and packaged Youngcraft.exe SHA256: CDC67CEEDC83613A1D6468337366D67A2AA1FB3F3069B5864C55F34E7ED758BC. The ZIP is 107066193 bytes. Installation and merge remain pending latest remote GUI evidence; no local GUI was launched. README now describes CIF editing/saving, periodic contacts, ORCA analysis support and trajectory requirements.

## Direct panel-to-geometry regression

Added a GUI integration test that verifies IRC graph selection updates molecular coordinates, frame navigation updates the IRC list/graph, and TD graph/table selection updates the exact associated geometry. This complements the discrete mouse-picking test and parser frame-energy checks. Windows Release application and GUI test compilation passed. GUI execution remains remote-only and pending. The prior package hash above predates the added IRC widget object names; rebuild packaging after final source verification. Read-only verification of that prepared package found 1243 matching manifest entries and matching reused MM worker.

## Bounded remote GUI execution

Added tools/run_gui_regressions.py for remote-only GUI execution. It runs every named test separately, limits each to 120 seconds and writes per-case text logs plus a JSON status summary. Syntax was checked locally without launching GUI tests. This prevents an indefinitely waiting test from hiding the remaining cases. The already-running remote job 43768672446 remains under observation; it uses the previous unbounded loop. Current-source portable packaging completed again; installation is still gated on remote GUI evidence.

## OpenGL test-context diagnosis

Inspection found that the application requests OpenGL 3.3 Core before QApplication, whereas the GUI test entry point requested only shared contexts. Aligned the test context with the application and added a checked initializeOpenGLFunctions return in MoleculeView so unsupported contexts cannot call uninitialized functions. Application and GUI test Release compilation passed without local GUI execution. Remote Windows rendering now uses Qt-distributed Mesa software OpenGL, pinned to official SHA256 c61f801c1760aa24b02c7a8354323cddf368b86f8f5c34b50b3b224f7d728afd. This is a CI-only graphics selection. Runtime GUI confirmation remains required; these changes address a concrete context mismatch, not a claim that all prior failures were caused by it.
