# xTB relaxed PES scan and TS candidate workflow

Youngcraft's xTB dialog supports a one-dimensional **relaxed** PES scan with GFN2-xTB or GFN1-xTB. Use **도구 → 빠른 양자화학 계산 (xTB)**, choose **Relaxed PES Scan (TS 후보)**, and specify an internal coordinate (distance, angle, or dihedral), its one-based atom indices, start/end values, and 3–501 scan points. The currently selected atoms initialize the controls (2 distance, 3 angle, 4 dihedral). Selecting a different coordinate type recalculates the starting value from the displayed structure.

The scan is launched as a normal bundled xTB geometry optimization with an xcontrol `$scan` block, for example:

```text
$scan
 distance: 1, 2, 1.00000000; 1.00000000, 1.40000000, 5
$end
```

```text
xtb.exe input.xyz --gfn 2 --uhf 0 --chrg 0 --input framework.inp --opt normal --cycles 500
```

The scan runs asynchronously and reads **xtbscan.log** as the process writes complete XYZ frames. The plot supports absolute Hartree energies and relative kcal/mol energies. Clicking a point previews that geometry, which can then be applied, opened separately, or exported as XYZ. The scan log and xTB text output remain in the calculation's temporary directory.

A strict **interior local maximum** of the scan energies is marked as a **TS candidate** and selected automatically. If the energy profile is monotonic, no TS candidate is reported; the user can still inspect any scanned geometry. The app does not label an unverified maximum as a converged transition state.

**Validation is required:** a constrained PES maximum is only an initial guess. To establish a first-order saddle point, perform an unconstrained TS optimization and a vibrational frequency calculation with exactly one physically relevant imaginary mode, then verify both directions using IRC. For transition-metal complexes, validate the electronic/spin state and refine the energetics at an appropriate DFT level.

Constraints from the geometry editor remain supported, but the scanned coordinate must not also appear as an independent fixed constraint. Automatic bond-retention restraints on the scanned distance are suppressed. For scans of chemical reactions, bond-retention options are switched off on entering scan mode; turn on individual restraints only when chemically justified. GFN-FF is excluded from TS-candidate scans.

The implementation is in `src/compute/XtbRunner.{h,cpp}` and `src/ui/XtbDialog.{h,cpp}`. Native parsing, input validation, and relaxed scan tests are in `tests/xtb/test_xtb.cpp`. This GitHub branch has not been built or run on the user's Windows workspace.
