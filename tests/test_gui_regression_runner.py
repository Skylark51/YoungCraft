"""Non-GUI regression tests for remote Qt result collection."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("gui_runner", Path(__file__).parents[1] / "tools" / "run_gui_regressions.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class GuiRunnerTests(unittest.TestCase):
    def test_skip_rows_and_totals_are_preserved(self):
        log = "SKIP   : SmokeTest::external(alpha beta) Real server results not supplied\nTotals: 2 passed, 0 failed, 1 skipped, 0 blacklisted, 10ms\n"
        skips, totals = runner.parse_log(log)
        self.assertEqual(skips, [{"case": "SmokeTest::external(alpha beta)", "reason": "Real server results not supplied"}])
        self.assertEqual(totals, {"passed": 2, "failed": 0, "skipped": 1})

    def test_discovery_ignores_data_and_diagnostics(self):
        with patch.object(runner.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "Qt diagnostic\nfirst()\nrows_data()\nsecond()\n", "")):
            self.assertEqual(runner.discover_tests(Path("test_smoke.exe"), 1), ["first", "second"])

    def test_no_discovered_cases_is_an_error(self):
        with patch.object(runner.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "", "")):
            with self.assertRaises(RuntimeError):
                runner.discover_tests(Path("test_smoke.exe"), 1)

    def test_per_case_skip_failure_timeout_and_stale_log(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            (out / "gui-november-timeout.txt").write_text("Totals: 3 passed, 0 failed, 0 skipped", encoding="utf-8")
            def execute(command, **kwargs):
                case = command[1]
                if case == "timeout":
                    raise subprocess.TimeoutExpired(command, 1)
                log = Path(command[3].removesuffix(",txt"))
                body = {
                    "ok": "Totals: 3 passed, 0 failed, 0 skipped",
                    "optional": "SKIP   : SmokeTest::optional() Missing integration input\nTotals: 2 passed, 0 failed, 1 skipped",
                    "bad": "Totals: 2 passed, 1 failed, 0 skipped",
                    "empty": "",
                }[case]
                log.write_text(body, encoding="utf-8")
                return subprocess.CompletedProcess(command, 1 if case == "bad" else 0)
            with patch.object(runner.subprocess, "run", side_effect=execute), patch("builtins.print"):
                status = runner.main(["--exe", "unused.exe", "--output-dir", directory, "ok", "optional", "bad", "timeout", "empty"])
            self.assertEqual(status, 1)
            results = json.loads((out / "gui-november-summary.json").read_text(encoding="utf-8"))
            self.assertEqual([item["status"] for item in results], ["passed", "passed_with_skips", "failed", "timeout", "missing_totals"])
            self.assertEqual(results[1]["totals"]["skipped"], 1)
            self.assertEqual(results[3]["exit_code"], None)
            self.assertEqual(results[3]["skips"], [])
            self.assertEqual(results[3]["totals"], None)


if __name__ == "__main__":
    unittest.main()
