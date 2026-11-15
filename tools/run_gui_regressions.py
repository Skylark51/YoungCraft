"""Run GUI regressions on a remote CI desktop, with per-case evidence and timeout."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--exe", required=True, type=Path)
parser.add_argument("--timeout", type=float, default=120)
parser.add_argument("tests", nargs="+")
args = parser.parse_args()
results = []
for test in args.tests:
    output = Path(f"gui-november-{test}.txt")
    try:
        result = subprocess.run(
            [str(args.exe.resolve()), test, "-o", f"{output},txt"],
            timeout=args.timeout,
            check=False,
        )
        status = "passed" if result.returncode == 0 else "failed"
        entry = {"test": test, "status": status, "exit_code": result.returncode}
    except subprocess.TimeoutExpired:
        entry = {"test": test, "status": "timeout", "timeout_seconds": args.timeout}
    results.append(entry)
    Path("gui-november-summary.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(entry), flush=True)
sys.exit(0 if all(item["status"] == "passed" for item in results) else 1)
