"""End-to-end target geometry and validation checks for the MM worker."""
import json
import math
from pathlib import Path
import subprocess
import sys
import unittest
from rdkit import Chem
from rdkit.Chem import AllChem, rdMolTransforms

ROOT = Path(__file__).resolve().parents[2]

class ConstraintTests(unittest.TestCase):
    def request(self, field, constraints):
        molecule = Chem.AddHs(Chem.MolFromSmiles("CCCC"))
        AllChem.EmbedMolecule(molecule, randomSeed=314)
        conf = molecule.GetConformer()
        return molecule, {"field": field, "charge": 0, "max_iterations": 3000, "chunk": 40,
            "atoms": [{"z": a.GetAtomicNum(), "p": list(conf.GetAtomPosition(a.GetIdx()))} for a in molecule.GetAtoms()],
            "bonds": [[b.GetBeginAtomIdx(), b.GetEndAtomIdx(), b.GetBondTypeAsDouble(), False] for b in molecule.GetBonds()],
            "constraints": constraints}

    def worker(self, request):
        program = [sys.executable, "-X", "utf8", str(ROOT / "tools/mm_optimize.py")]
        if len(sys.argv) > 1 and sys.argv[1].endswith(".exe"):
            program = [sys.argv[1]]
        result = subprocess.run(program, input=json.dumps(request) + "\n" + "chunk\n" * 100,
                                text=True, capture_output=True, encoding="utf-8", timeout=60)
        values = [json.loads(line) for line in result.stdout.splitlines()]
        self.assertTrue(values, result.stderr)
        return values

    def test_fixed_core(self):
        for field in ("MMFF94", "MMFF94s", "UFF"):
            with self.subTest(field=field):
                fixed=[0, 1, 4, 5, 6]
                constraints=[{"type":"fixed", "atoms":[i], "target":0} for i in fixed]
                constraints.append({"type":"distance", "atoms":[1,2], "target":1.7})
                _,request=self.request(field,constraints)
                values=self.worker(request)
                self.assertEqual(values[-1]["type"],"done",values[-1])
                for frame in values:
                    if "positions" in frame:
                        for i in fixed:self.assertEqual(frame["positions"][i],request["atoms"][i]["p"])
                self.assertLess(abs(math.dist(values[-1]["positions"][1],values[-1]["positions"][2])-1.7),0.03)
                self.assertTrue(any(math.dist(values[-1]["positions"][i],request["atoms"][i]["p"])>0.01 for i in range(len(request["atoms"])) if i not in fixed))

    def test_targets(self):
        constraints = [{"type": "distance", "atoms": [0,1], "target": 1.6},
                       {"type": "angle", "atoms": [0,1,2], "target": 110.0},
                       {"type": "dihedral", "atoms": [0,1,2,3], "target": -65.0}]
        for field in ("MMFF94", "MMFF94s", "UFF"):
            with self.subTest(field=field):
                molecule, request = self.request(field, constraints)
                values = self.worker(request)
                self.assertEqual(values[-1]["type"], "done", values[-1])
                for i, position in enumerate(values[-1]["positions"]):
                    molecule.GetConformer().SetAtomPosition(i, position)
                conf = molecule.GetConformer()
                actual = [rdMolTransforms.GetBondLength(conf,0,1), rdMolTransforms.GetAngleDeg(conf,0,1,2),
                          rdMolTransforms.GetDihedralDeg(conf,0,1,2,3)]
                print(field, actual, "converged", values[-1]["converged"])
                for value, target, tolerance in zip(actual, (1.6,110,-65), (0.02,0.1,0.1)):
                    self.assertLess(abs(value-target), tolerance)
                self.assertTrue(all(math.isfinite(v["energy"]) for v in values))

    def test_charged_molecules(self):
        for smiles in ("[NH4+]", "C[NH3+]", "CC(=O)[O-]", "[NH3+]CC(=O)[O-]"):
            for field in ("MMFF94", "MMFF94s", "UFF"):
                with self.subTest(smiles=smiles, field=field):
                    molecule = Chem.AddHs(Chem.MolFromSmiles(smiles))
                    AllChem.EmbedMolecule(molecule, randomSeed=314)
                    conf=molecule.GetConformer()
                    request={"field":field,"charge":Chem.GetFormalCharge(molecule),"max_iterations":500,"chunk":40,
                        "atoms":[{"z":a.GetAtomicNum(),"formal_charge":a.GetFormalCharge(),"p":list(conf.GetAtomPosition(a.GetIdx()))} for a in molecule.GetAtoms()],
                        "bonds":[[b.GetBeginAtomIdx(),b.GetEndAtomIdx(),b.GetBondTypeAsDouble(),False] for b in molecule.GetBonds()]}
                    values=self.worker(request)
                    self.assertEqual(values[0]["type"],"ready",values)
                    self.assertEqual(values[-1]["type"],"done",values[-1])
                    self.assertEqual(len(values[-1]["positions"]),molecule.GetNumAtoms())
                    self.assertTrue(math.isfinite(values[-1]["energy"]))
                    print(smiles,field,"done")

    def test_incomplete_hydrogens_use_original_atom_model(self):
        _,request=self.request("Auto",[])
        request["atoms"]=request["atoms"][:4]
        request["bonds"]=[bond for bond in request["bonds"] if max(bond[:2])<4]
        result=self.worker(request)[0]
        self.assertEqual(result["type"],"fallback")
        self.assertEqual(result["method"],"GFN-FF")

    def test_bond_retention_and_explicit_override(self):
        _,request=self.request("Auto",[{"type":"distance","atoms":[0,1],"target":1.7}])
        request["preserve_bond_lengths"]=True
        result=self.worker(request)[-1]
        self.assertEqual(result["type"],"done",result)
        for bond in request["bonds"]:
            a,b=bond[:2]
            target=1.7 if {a,b}=={0,1} else math.dist(request["atoms"][a]["p"],request["atoms"][b]["p"])
            self.assertLess(abs(math.dist(result["positions"][a],result["positions"][b])-target),0.025)

    def test_gfnff_fallback(self):
        _,request=self.request("Auto",[])
        request["charge"]=1
        self.assertEqual(self.worker(request)[0]["type"],"fallback")
        request["charge"]=0;request["atoms"][0]["z"]=26
        self.assertEqual(self.worker(request)[0]["method"],"GFN-FF")
        request["atoms"][0]["z"]=6;request["bonds"][0][3]=True
        self.assertEqual(self.worker(request)[0]["type"],"fallback")

    def test_invalid(self):
        invalid = [[{"type":"distance","atoms":[0,999],"target":1.5}],
                   [{"type":"angle","atoms":[0,1,1],"target":90}],
                   [{"type":"dihedral","atoms":[0,1,2,3],"target":200}],
                   [{"type":"distance","atoms":[0,1],"target":1.5},
                    {"type":"distance","atoms":[1,0],"target":1.6}]]
        for entries in invalid:
            _, request = self.request("Auto", entries)
            self.assertEqual(self.worker(request)[-1]["type"], "error")

if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]], verbosity=2)
