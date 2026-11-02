# Lab complex prototype — 2026-10-08

Reference: user-supplied `5-MnIII-12MTP-CHNone-forNA-2-scan-freq-ts-freq-ircfwd-opt-freq.out` (charge +1, multiplicity 5).

The complete Input orientation table contains 62 atoms. Original atom 1 is Mn; atoms 2–43 form the C14H24N4 ligand; atoms 44–45 form peroxo; atoms 46–62 form cyclohexanone C6H10O and are excluded. Ligand donor atoms are original N3, N6, N10, N18. Peroxo is eta2-bound through both O atoms, with reference O–O distance 1.3909 Å. The exported complex has 45 real atoms and six metal coordination bonds. Atom ranges, element counts and peroxo connectivity are checked by `tools/prepare_lab_mtp.py` before extraction. Source SHA-256 and original atom numbers are stored in each CJSON.

The ligand graph uses distance-derived single bonds and the six-membered pyridine ring's aromatic bond orders. Explicit hydrogens from the source are retained. Metal replacement changes the center element; ligand/peroxo starting coordinates remain in the supplied reference metal frame.

Engine: [GFN-FF in xTB](https://xtb-docs.readthedocs.io/en/latest/gfnff.html), a molecular mechanics force field. It does not calculate spin states. The assembly workflow uses gas phase and optional [harmonic distance restraints](https://xtb-docs.readthedocs.io/en/latest/xcontrol.html#constraining-potentials), force constant 1.0 atomic units, for existing coordination bonds and metal-bound O–O bonds. This is initial structure preparation, not evidence of an equilibrium Fe geometry or relative spin stability. Switching from Mn to Fe retains Mn-derived distance targets by default; targets can be released in the calculation dialog.

Validation completed:

- Release compilation and native CJSON loading/assembly: 45 atoms, six coordination bonds, charge +1, correct peroxo atoms, substrate excluded.
- Actual bundled xTB 6.7.1 GFN-FF optimization through the C++ XtbRunner for both Fe and Mn: converged. Method command is `--gfnff` without electronic `--uhf` arguments.
- With restraints, Fe O–O = 1.3952 Å and Mn O–O = 1.3956 Å; maximum change of any restrained distance < 0.013 Å.
- Unrestrained Fe trial elongated one Fe–N distance to 2.9358 Å, motivating the default restraint. The user can turn restraints off explicitly.
- Successful reference outputs: `docs/lab-complex-examples/Fe-12MTP-peroxo-GFNFF.xyz` and `Mn-12MTP-peroxo-GFNFF.xyz`.

Additional fragments are discovered from `lab_fragments` when the assembly dialog opens. CJSON property `category` selects `lab-ligand` or `lab-adduct`; `name` controls the visible label; `totalCharge` supplies the fragment charge. Each fragment needs one dummy anchor with explicit donor connections. Atom coordinate overlap below 0.45 Å rejects assembly. The current prototype provides Fe/Mn + 12-MTP + eta2-peroxo; other oxygen binding modes require separate prepared fragments.
