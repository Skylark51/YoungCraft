"""Fetch public validation data into a separate directory and record SHA256 provenance."""
import argparse
import hashlib
import json
from pathlib import Path
import urllib.request

parser = argparse.ArgumentParser()
parser.add_argument("destination", type=Path)
args = parser.parse_args()
sources = Path(__file__).resolve().parents[1] / "tests/data/november-corpus-sources.json"
entries = json.loads(sources.read_text())
for entry in entries:
    target = args.destination / entry["path"]
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists():
        request = urllib.request.Request(entry["url"], headers={"User-Agent": "Youngcraft-validation/1.0"})
        with urllib.request.urlopen(request, timeout=60) as response:
            data = response.read()
        temporary = target.with_suffix(target.suffix + ".download")
        temporary.write_bytes(data)
        temporary.replace(target)
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    if digest != entry["sha256"]:
        raise RuntimeError(f"Corpus checksum mismatch: {entry['path']}")
    entry["bytes"] = target.stat().st_size
    print(entry["path"], entry["bytes"], flush=True)
(args.destination / "provenance.json").write_text(json.dumps(entries, indent=2) + "\n")
