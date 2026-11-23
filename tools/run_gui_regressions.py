"""Run Qt GUI cases on a remote CI desktop, retaining logs, skips and timeouts."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


def discover_tests(exe, timeout):
    result = subprocess.run([str(exe), "-functions"], capture_output=True,
                            text=True, encoding="utf-8", errors="replace",
                            timeout=timeout, check=True)
    names = re.findall(r"^([A-Za-z_]\w*)\(\)\s*$", result.stdout, re.MULTILINE)
    names = [name for name in names if not name.endswith("_data")]
    if not names:
        raise RuntimeError("Qt -functions returned no test functions")
    return names


def parse_log(log):
    skips = []
    for line in log.splitlines():
        match = re.match(r"SKIP\s*:\s*(.*?)\s+(.*)", line)
        if match:
            skips.append({"case": match.group(1), "reason": match.group(2)})
    totals = re.findall(r"Totals:\s*(\d+) passed,\s*(\d+) failed,\s*(\d+) skipped", log)
    return skips, ({"passed": int(totals[-1][0]), "failed": int(totals[-1][1]),
                    "skipped": int(totals[-1][2])} if totals else None)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--timeout", type=float, default=120)
    parser.add_argument("--all", action="store_true", help="Discover cases with Qt -functions")
    parser.add_argument("--output-dir", type=Path, default=Path("."))
    parser.add_argument("tests", nargs="*")
    args = parser.parse_args(argv)
    if not args.all and not args.tests:
        parser.error("provide test names or --all")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    exe = args.exe.resolve()
    tests = list(dict.fromkeys((discover_tests(exe, args.timeout) if args.all else []) + args.tests))
    results = []
    for test in tests:
        output = args.output_dir.resolve() / f"gui-november-{test}.txt"
        output.write_text("", encoding="utf-8")  # Never parse an earlier run after a crash.
        entry = {"test": test, "log": str(output)}
        try:
            result = subprocess.run([str(exe), test, "-o", f"{output},txt"],
                                    timeout=args.timeout, check=False)
            entry.update(status="passed" if result.returncode == 0 else "failed",
                         exit_code=result.returncode)
        except subprocess.TimeoutExpired:
            entry.update(status="timeout", exit_code=None, timeout_seconds=args.timeout)
        skips, totals = parse_log(output.read_text(encoding="utf-8", errors="replace"))
        entry.update(skips=skips, totals=totals)
        if entry["status"] == "passed":
            if totals is None:
                entry["status"] = "missing_totals"
            elif totals["failed"]:
                entry["status"] = "failed"
            elif totals["skipped"] or skips:
                entry["status"] = "passed_with_skips"
        results.append(entry)
        (args.output_dir / "gui-november-summary.json").write_text(
            json.dumps(results, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(json.dumps(entry, ensure_ascii=False), flush=True)
    return 0 if all(item["status"] in {"passed", "passed_with_skips"} for item in results) else 1


if __name__ == "__main__":
    sys.exit(main())
