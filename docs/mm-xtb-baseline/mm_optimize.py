"""Standalone, line-oriented RDKit optimization worker for Youngcraft.

The first command provides a short visible warm-up; later commands run longer
minimizer chunks. Waiting for a command is the pause mechanism.
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
        if initial.get("charge", 0):
            raise ValueError("원자별 형식 전하 정보가 없어 하전 구조를 안전하게 파라미터화할 수 없습니다.")
        editable = Chem.RWMol()
        for index, atom in enumerate(atoms):
            z = int(atom["z"])
            if not 1 <= z <= 118:
                raise ValueError(f"원자 {index + 1}: 유효하지 않은 atomic number {z}")
            editable.AddAtom(Chem.Atom(z))
        types = {1.0: Chem.BondType.SINGLE, 2.0: Chem.BondType.DOUBLE,
                 3.0: Chem.BondType.TRIPLE, 1.5: Chem.BondType.AROMATIC}
        for bond in initial["bonds"]:
            a, b = int(bond[0]), int(bond[1])
            if len(bond) > 3 and bond[3]:
                raise ValueError(f"원자 {a + 1}–{b + 1}: 배위결합을 RDKit MMFF/UFF의 일반 공유결합으로 취급할 수 없습니다. 이 금속 착물 환경은 지원하지 않습니다.")
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
            raise ValueError("전이금속/란타넘족/악티늄족 환경은 RDKit MMFF/UFF의 결합·배위 파라미터를 Youngcraft에서 검증하지 않아 최적화하지 않습니다: "
                             + ", ".join(sorted({str(z) for z in metals})))
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
            raise ValueError(f"결합/원자가가 화학적으로 유효하지 않습니다: {error}") from error
        molecule = Chem.AddHs(molecule, addCoords=True)

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
            raise ValueError(f"{requested} 파라미터화 실패: 원소/결합 환경 ({symbols})에 사용할 수 있는 MMFF94/MMFF94s/UFF 파라미터가 없습니다.")

        maximum = max(1, min(100000, int(initial.get("max_iterations", 500))))
        step = 0
        previous = field.CalcEnergy()
        send({"type": "ready", "field": chosen, "iteration": 0,
              **metrics(field, molecule.GetNumAtoms())})
        for command in sys.stdin:
            if command.strip() != "chunk":
                raise ValueError(f"알 수 없는 명령: {command.strip()}")
            chunk_size = 8 if step == 0 else 40
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
    except Exception as error:
        send({"type": "error", "message": str(error)})


if __name__ == "__main__":
    main()
