"""Windows-only launcher regressions; no native GUI, network or admin required.

Run: python tests/packaging/test_standalone.py
All fixtures, caches and probe executables are confined to a unique temp folder.
Builds deliberately use Windows PowerShell 5, including replacing existing EXEs.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]
BUILDER = ROOT / "tools" / "create_standalone.ps1"
HIDDEN_PROCESS = {"creationflags": subprocess.CREATE_NO_WINDOW, "stdin": subprocess.DEVNULL} if os.name == "nt" else {}
CSC = Path(os.environ.get("WINDIR", "C:/Windows")) / "Microsoft.NET/Framework64/v4.0.30319/csc.exe"


@unittest.skipUnless(os.name == "nt", "Windows .NET Framework launcher")
class StandaloneTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="yc-sfx-")
        cls.base = Path(cls.temporary.name)
        probe_source = cls.base / "Probe.cs"
        probe_source.write_text(r'''
using System;
using System.IO;
using System.Text;
using System.Web.Script.Serialization;
class Probe {
    static void Main(string[] args) {
        var record = new { args = args, cwd = Environment.CurrentDirectory,
            relative = args.Length > 0 ? Path.GetFullPath(args[0]) : "" };
        File.WriteAllText(Environment.GetEnvironmentVariable("YOUNGCRAFT_TEST_REPORT"),
            new JavaScriptSerializer().Serialize(record), new UTF8Encoding(false));
    }
}
''', encoding="utf-8")
        cls.probe = cls.base / "Youngcraft.exe"
        subprocess.run([str(CSC), "/nologo", "/target:winexe", "/reference:System.Web.Extensions.dll",
                        "/out:" + str(cls.probe), str(probe_source)], check=True, capture_output=True, **HIDDEN_PROCESS)
        cls.native_bytes = cls.probe.read_bytes()
        cls.zip1, cls.exe1 = cls.build("version1")
        cls.zip2, cls.exe2 = cls.build("version2", updates={
            "youngcraft-server.json": b'{"host":"new-default"}',
            "lab_fragments/edited.cjson": b'{"bundled":"new"}',
            "lab_fragments/unchanged.cjson": b'{"bundled":"new"}',
        })

    @classmethod
    def tearDownClass(cls):
        # TemporaryDirectory only removes this test's known, unique directory.
        cls.temporary.cleanup()

    @classmethod
    def build(cls, name, updates=None, extras=None, mutate_manifest=None, wrong_hash=False):
        files = {
            "Youngcraft.exe": cls.native_bytes,
            "youngcraft-server.json": b'{"host":"old-default"}',
            "lab_fragments/edited.cjson": b'{"bundled":"old"}',
            "lab_fragments/unchanged.cjson": b'{"bundled":"old"}',
            "xtb_runtime/gfnff/xtb.exe": b"fixture-runtime-not-executed",
            "nested/data.bin": bytes(range(256)) * 8192,
        }
        files.update(updates or {})
        records = [{"path": path, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                   for path, data in files.items()]
        if wrong_hash:
            records[-1]["sha256"] = "0" * 64
        if mutate_manifest:
            mutate_manifest(records)
        manifest = json.dumps({"schema": 1, "files": records}).encode("utf-8-sig")
        package = cls.base / (name + ".zip")
        with zipfile.ZipFile(package, "w", zipfile.ZIP_DEFLATED) as archive:
            for path, data in files.items():
                # Exercise Compress-Archive's Windows backslash ZIP paths.
                archive.writestr(path.replace("/", "\\"), data)
            for path, data in extras or []:
                archive.writestr(path, data)
            archive.writestr("package-manifest.json", manifest)
        executable = cls.base / (name + ".exe")
        cls.compile(package, executable)
        return package, executable

    @staticmethod
    def compile(package, executable, check=True):
        env = {key: value for key, value in os.environ.items() if key.lower() != "psmodulepath"}
        # PowerShell 7's inherited module search path hides PS5 modules.
        result = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-WindowStyle", "Hidden", "-ExecutionPolicy", "Bypass", "-File",
                                 str(BUILDER), "-PackageZip", str(package), "-OutputPath", str(executable)],
                                capture_output=True, timeout=60, env=env, **HIDDEN_PROCESS)
        if check and result.returncode:
            raise AssertionError(result.stdout.decode(errors="replace") + result.stderr.decode(errors="replace"))
        return result

    def root(self, name):
        directory = self.base / hashlib.sha256(self._testMethodName.encode()).hexdigest()[:8] / name
        directory.mkdir(parents=True, exist_ok=True)
        return directory

    def invoke(self, executable, root, *options):
        return subprocess.run([str(executable), "--youngcraft-cache-root", str(root), *options],
                              capture_output=True, encoding="utf-8", timeout=30, **HIDDEN_PROCESS)

    def extract(self, executable, root):
        result = self.invoke(executable, root, "--youngcraft-extract-only")
        self.assertEqual(0, result.returncode, result.stderr)
        return Path(result.stdout.strip())

    def test_extract_repeat_verify_and_user_cache_preserved(self):
        root = self.root("cache 한글 space 🧪")
        cache = self.extract(self.exe1, root)
        self.assertEqual(hashlib.sha256(self.zip1.read_bytes()).hexdigest(), cache.name)
        self.assertEqual(self.native_bytes, (cache / "Youngcraft.exe").read_bytes())
        times = {p.relative_to(cache): p.stat().st_mtime_ns for p in cache.rglob("*") if p.is_file()}
        user_file = cache / "personal.cache"
        user_file.write_text("keep", encoding="utf-8")
        self.assertEqual(cache, self.extract(self.exe1, root))
        self.assertEqual(times, {p: (cache / p).stat().st_mtime_ns for p in times})
        result = self.invoke(self.exe1, root, "--youngcraft-verify-only")
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual("keep", user_file.read_text())
        self.assertFalse(list(root.glob(".extracting-*")))

    def test_verify_only_missing_cache_and_bad_options(self):
        root = self.root("missing")
        result = self.invoke(self.exe1, root, "--youngcraft-verify-only")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("not been extracted", result.stderr)
        result = self.invoke(self.exe1, root, "--youngcraft-extract-only", "--youngcraft-cache-root")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("requires a directory", result.stderr)

    def test_corrupt_cached_code_and_manifest_preserved_and_rejected(self):
        for relative in ("Youngcraft.exe", "package-manifest.json"):
            with self.subTest(relative=relative):
                root = self.root(relative)
                cache = self.extract(self.exe1, root)
                target = cache / relative
                target.write_bytes(b"corrupt")
                result = self.invoke(self.exe1, root, "--youngcraft-extract-only")
                self.assertNotEqual(0, result.returncode)
                self.assertIn("SHA256 mismatch", result.stderr)
                self.assertEqual(b"corrupt", target.read_bytes())

    def test_same_size_tampering_with_original_timestamp_is_rejected(self):
        root = self.root("same-size")
        cache = self.extract(self.exe1, root)
        target = cache / "nested/data.bin"
        original = target.stat()
        data = bytearray(target.read_bytes())
        data[len(data) // 2] ^= 1
        target.write_bytes(data)
        os.utime(target, ns=(original.st_atime_ns, original.st_mtime_ns))
        result = self.invoke(self.exe1, root, "--youngcraft-verify-only")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("SHA256 mismatch", result.stderr)
        self.assertEqual(data, target.read_bytes())

    def test_corrupt_embedded_zip_hash_rejected(self):
        damaged = self.root("damaged") / "damaged.exe"
        content = bytearray(self.exe1.read_bytes())
        payload = self.zip1.read_bytes()
        offset = content.find(payload)
        self.assertGreaterEqual(offset, 0, "Embedded ZIP must be present verbatim")
        content[offset + len(payload) // 2] ^= 1
        damaged.write_bytes(content)
        result = self.invoke(damaged, self.root("cache"), "--youngcraft-extract-only")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("ZIP SHA256 mismatch", result.stderr)

    def test_manifest_checksum_rejected_without_publishing_cache(self):
        package, executable = self.build("bad-checksum", wrong_hash=True)
        root = self.root("bad")
        result = self.invoke(executable, root, "--youngcraft-extract-only")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("SHA256 mismatch", result.stderr)
        self.assertFalse((root / hashlib.sha256(package.read_bytes()).hexdigest()).exists())

    def test_unsafe_zip_paths_and_duplicate_aliases_rejected(self):
        bad_paths = ["../escaped.txt", "..\\escaped.txt", "/absolute.txt", "C:/outside.txt",
                     "nested/file:stream", "nested/NUL.txt", "nested/trailing. ", "youngcraft.EXE"]
        for number, path in enumerate(bad_paths):
            with self.subTest(path=path):
                _, executable = self.build("unsafe" + str(number), extras=[(path, b"bad")])
                root = self.root(str(number))
                result = self.invoke(executable, root, "--youngcraft-extract-only")
                self.assertNotEqual(0, result.returncode)
                self.assertFalse(list(root.glob(".extracting-*")), "Unsafe paths must be rejected before extraction")
        self.assertFalse((self.base / "escaped.txt").exists())

    def test_unsafe_manifest_path_rejected(self):
        _, executable = self.build("unsafe-manifest", mutate_manifest=lambda records: records.append(
            {"path": "../escaped.txt", "size": 0, "sha256": hashlib.sha256(b"").hexdigest()}))
        result = self.invoke(executable, self.root("bad"), "--youngcraft-extract-only")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("Unsafe payload path", result.stderr)

    def test_concurrent_first_launch_publishes_one_cache(self):
        root = self.root("concurrent")
        commands = [str(self.exe1), "--youngcraft-cache-root", str(root), "--youngcraft-extract-only"]
        children = [subprocess.Popen(commands, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **HIDDEN_PROCESS) for _ in range(8)]
        outcomes = [child.communicate(timeout=30) for child in children]
        for child, (output, error) in zip(children, outcomes):
            self.assertEqual(0, child.returncode, error)
        self.assertEqual(1, len({output.strip() for output, _ in outcomes}))
        self.assertEqual(1, len([p for p in root.iterdir() if p.is_dir()]))
        self.assertFalse(list(root.glob(".extracting-*")))

    def test_argument_forwarding_and_original_cwd(self):
        root = self.root("cache")
        caller = self.root("invoking 한글 folder")
        report = caller / "report.json"
        arguments = ["상대 분자.xyz", "", "space and 한글 🧪", "C:\\folder space\\", "trailing\\\\",
                     'literal"quote', 'slashes\\\\"quote', "--new-window", "--", "--youngcraft-extract-only"]
        env = dict(os.environ, YOUNGCRAFT_TEST_REPORT=str(report))
        result = subprocess.run([str(self.exe1), "--youngcraft-cache-root", str(root), *arguments],
                                cwd=caller, env=env, capture_output=True, timeout=30, **HIDDEN_PROCESS)
        self.assertEqual(0, result.returncode, result.stderr)
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            try:
                observed = json.loads(report.read_text(encoding="utf-8"))
                break
            except (FileNotFoundError, json.JSONDecodeError):
                time.sleep(0.05)
        else:
            self.fail("Native argument probe did not report")
        self.assertEqual(arguments, observed["args"])
        self.assertEqual(str(caller), observed["cwd"])
        self.assertEqual(str(caller / arguments[0]), observed["relative"])

    def test_custom_data_migrates_and_new_bundles_win_for_unchanged_files(self):
        root = self.root("upgrade")
        old = self.extract(self.exe1, root)
        (old / "youngcraft-server.json").write_text('{"host":"custom"}', encoding="utf-8")
        (old / "lab_fragments/edited.cjson").write_text("custom geometry", encoding="utf-8")
        (old / "lab_fragments/personal.cjson").write_text("personal", encoding="utf-8")
        new = self.extract(self.exe2, root)
        self.assertNotEqual(old, new)
        self.assertEqual('{"host":"custom"}', (new / "youngcraft-server.json").read_text())
        self.assertEqual("custom geometry", (new / "lab_fragments/edited.cjson").read_text())
        self.assertEqual("personal", (new / "lab_fragments/personal.cjson").read_text())
        self.assertEqual('{"bundled":"new"}', (new / "lab_fragments/unchanged.cjson").read_text())
        self.assertEqual(0, self.invoke(self.exe2, root, "--youngcraft-verify-only").returncode)
        modified = new / "lab_fragments/edited.cjson"
        modified.write_text("latest user geometry", encoding="utf-8")
        # Deterministic timestamp ordering independent of filesystem clock resolution.
        timestamp = time.time() + 2
        os.utime(modified, (timestamp, timestamp))
        self.extract(self.exe1, root)
        self.assertEqual("latest user geometry", (old / "lab_fragments/edited.cjson").read_text())
        self.assertEqual("latest user geometry", modified.read_text())
        self.assertTrue((old / "Youngcraft.exe").exists())

    def test_powershell5_repeated_build_and_native_output_guard(self):
        output = self.root("build output 한글") / "Youngcraft-Standalone.exe"
        self.compile(self.zip1, output)
        self.compile(self.zip2, output)
        cache = self.extract(output, self.root("rebuilt-cache"))
        self.assertEqual(hashlib.sha256(self.zip2.read_bytes()).hexdigest(), cache.name)
        native = output.parent / "Youngcraft.exe"
        native.write_bytes(b"native must survive")
        rejected = self.compile(self.zip1, native, check=False)
        self.assertNotEqual(0, rejected.returncode)
        self.assertEqual(b"native must survive", native.read_bytes())


if __name__ == "__main__":
    unittest.main(verbosity=2)
