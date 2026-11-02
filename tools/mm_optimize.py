"""Standalone, line-oriented RDKit optimization worker for Youngcraft.

Each command runs a small real minimizer chunk. Waiting for a command is the pause mechanism.
"""
import json
import math
import sys


def send(payload):
    print(json.dumps(payload, separators=(",", ":"), allow_nan=False), flush=True)


def metrics(field, count, previous=None):
    energy = field.CalcEnergy()
    gradient = field.CalcGrad()
    lengths = [math.sqrt(sum(gradient[3 * i + j] ** 2 for j in range(3)))
               for i in range(count)]
    return {"energy": energy, "delta_energy": 0.0 if previous is None else energy - previous,
            "rms_gradient": math.sqrt(sum(value * value for value in lengths) / count),
            "max_gradient": max(lengths)}


def add_constraints(field, chosen, constraints, count):
    seen = set()
    for entry in constraints:
        kind = entry.get("type")
        ids = entry.get("atoms", [])
        target = entry.get("target")
        size = {"fixed": 1, "distance": 2, "angle": 3, "dihedral": 4}.get(kind)
        if (not size or len(ids) != size or any(type(i) is not int or not 0 <= i < count for i in ids)
                or len(set(ids)) != size or type(target) not in (float, int) or not math.isfinite(target)):
            raise ValueError("Constraint 원자 번호 또는 목표값이 유효하지 않습니다.")
        if (kind == "distance" and target <= 0 or kind == "angle" and not 0 < target < 180
                or kind == "dihedral" and not -180 <= target <= 180):
            raise ValueError("Constraint 목표값 범위를 확인하세요.")
        key = (kind, min(tuple(ids), tuple(reversed(ids))))
        if key in seen:
            raise ValueError("같은 내부 좌표의 constraint가 중복되었습니다.")
        seen.add(key)
        if kind == "fixed":
            field.AddFixedPoint(ids[0])
            continue
        prefix = "UFF" if chosen == "UFF" else "MMFF"
        suffix = {"distance": "Distance", "angle": "Angle", "dihedral": "Torsion"}[kind]
        getattr(field, prefix + "Add" + suffix + "Constraint")(*ids, False, target, target, 10000.0)


class GfnffFallback(Exception):
    """The input needs a force field that uses total molecular charge."""


def main():
    try:
        from rdkit import Chem
        from rdkit.Chem import AllChem

        raw = sys.stdin.readline()
        if not raw:
            raise ValueError("초기 구조 데이터가 없습니다.")
        initial = json.loads(raw)
        atoms = initial["atoms"]
        if not atoms:
            raise ValueError("최적화할 원자가 없습니다.")
        total_charge = int(initial.get("charge", 0))
        formal_charges = [int(atom.get("formal_charge", 0)) for atom in atoms]
        if sum(formal_charges) != total_charge:
            raise GfnffFallback("총전하를 사용하는 GFN-FF로 최적화합니다.")
        editable = Chem.RWMol()
        for index, atom in enumerate(atoms):
            z = int(atom["z"])
            if not 1 <= z <= 118:
                raise ValueError(f"원자 {index + 1}: 유효하지 않은 atomic number {z}")
            rd_atom = Chem.Atom(z)
            rd_atom.SetFormalCharge(formal_charges[index])
            editable.AddAtom(rd_atom)
        types = {1.0: Chem.BondType.SINGLE, 2.0: Chem.BondType.DOUBLE,
                 3.0: Chem.BondType.TRIPLE, 1.5: Chem.BondType.AROMATIC}
        for bond in initial["bonds"]:
            a, b = int(bond[0]), int(bond[1])
            if len(bond) > 3 and bond[3]:
                raise GfnffFallback("배위결합 구조를 GFN-FF로 최적화합니다.")
            order = float(bond[2])
            kind = types.get(order)
            if kind is None:
                raise ValueError(f"원자 {a + 1}–{b + 1}: 결합 차수 {order}는 지원되지 않습니다.")
            if a == b or not (0 <= a < len(atoms) and 0 <= b < len(atoms)):
                raise ValueError("결합의 원자 인덱스가 유효하지 않습니다.")
            editable.AddBond(a, b, kind)
        transition_metals = (set(range(21, 31)) | set(range(39, 49)) |
                             set(range(57, 81)) | set(range(89, 113)))
        metals = [int(atom["z"]) for atom in atoms if int(atom["z"]) in transition_metals]
        if metals:
            raise GfnffFallback("금속 구조를 GFN-FF로 최적화합니다.")
        molecule = editable.GetMol()
        conformer = Chem.Conformer(len(atoms))
        for index, atom in enumerate(atoms):
            position = [float(v) for v in atom["p"]]
            if len(position) != 3 or not all(math.isfinite(v) for v in position):
                raise ValueError(f"원자 {index + 1}: 좌표가 유효하지 않습니다.")
            conformer.SetAtomPosition(index, position)
        molecule.AddConformer(conformer)
        try:
            Chem.SanitizeMol(molecule)
        except Exception as error:
            raise GfnffFallback("입력 결합 환경을 GFN-FF로 최적화합니다.") from error
        hydrogenated = Chem.AddHs(molecule, addCoords=True)
        if hydrogenated.GetNumAtoms() != len(atoms):
            raise GfnffFallback("입력 원자와 결합을 그대로 사용하는 GFN-FF로 최적화합니다.")
        molecule = hydrogenated

        requested = initial.get("field", "Auto")
        candidates = ["MMFF94", "MMFF94s", "UFF"] if requested == "Auto" else [requested]
        if requested not in ("Auto", "MMFF94", "MMFF94s", "UFF"):
            raise ValueError(f"알 수 없는 force field: {requested}")
        field = None
        chosen = None
        for candidate in candidates:
            if candidate == "UFF":
                if not AllChem.UFFHasAllMoleculeParams(molecule):
                    continue
                field = AllChem.UFFGetMoleculeForceField(molecule, ignoreInterfragInteractions=False)
            else:
                properties = AllChem.MMFFGetMoleculeProperties(molecule, mmffVariant=candidate)
                if properties is None:
                    continue
                field = AllChem.MMFFGetMoleculeForceField(
                    molecule, properties, ignoreInterfragInteractions=False)
            if field is not None:
                chosen = candidate
                break
        if field is None:
            symbols = ", ".join(sorted({Chem.GetPeriodicTable().GetElementSymbol(int(a["z"])) for a in atoms}))
            raise GfnffFallback(f"{requested} 대신 {symbols} 구조에 GFN-FF를 사용합니다.")

        entries=initial.get("constraints", [])
        if initial.get("preserve_bond_lengths", False):
            explicit={tuple(sorted(entry["atoms"])) for entry in entries if entry.get("type")=="distance"}
            prefix="UFF" if chosen=="UFF" else "MMFF"
            for bond in initial["bonds"]:
                a,b=int(bond[0]),int(bond[1])
                if tuple(sorted((a,b))) not in explicit:
                    length=math.dist(atoms[a]["p"],atoms[b]["p"])
                    getattr(field,prefix+"AddDistanceConstraint")(a,b,False,length,length,10000.0)
        add_constraints(field, chosen, entries, len(atoms))
        field.Initialize()

        maximum = max(1, min(100000, int(initial.get("max_iterations", 500))))
        chunk_size = max(1, min(40, int(initial.get("chunk", 8))))
        step = 0
        previous = field.CalcEnergy()
        send({"type": "ready", "field": chosen, "iteration": 0,
              **metrics(field, molecule.GetNumAtoms())})
        for command in sys.stdin:
            if command.strip() != "chunk":
                raise ValueError(f"알 수 없는 명령: {command.strip()}")
            iterations = min(chunk_size, maximum - step)
            # One persistent ForceField object, one minimizer call per chunk.
            # This RDKit build has no usable Snapshot Python binding.
            converged = field.Minimize(maxIts=iterations, forceTol=1e-4,
                                       energyTol=1e-6) == 0
            step += iterations
            values = field.Positions()
            positions = [list(values[3 * i:3 * i + 3]) for i in range(len(atoms))]
            current = metrics(field, molecule.GetNumAtoms(), previous)
            previous = current["energy"]
            finished = converged or step >= maximum
            send({"type": "done" if finished else "step", "field": chosen,
                  "iteration": step, "positions": positions, "converged": converged,
                  "iteration_exact": not converged,
                  "reason": "converged" if converged else ("max_iterations" if finished else "running"),
                  **current})
            if finished:
                return
    except GfnffFallback as reason:
        send({"type": "fallback", "method": "GFN-FF", "message": str(reason)})
    except Exception as error:
        send({"type": "error", "message": str(error)})


if __name__ == "__main__":
    main()
