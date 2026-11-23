"""Validate real portable manifest paths across Windows long/8.3 aliases."""
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


@unittest.skipUnless(os.name == 'nt', 'Windows path aliases required')
class ManifestPaths(unittest.TestCase):
    def test_long_and_short_roots_in_windows_powershell_and_pwsh(self):
        helper = Path(__file__).resolve().parents[2] / 'tools/PortableManifest.ps1'
        with tempfile.TemporaryDirectory(prefix='Youngcraft manifest path ') as temporary:
            root = Path(temporary) / '한글 staging directory'
            root.mkdir()
            expected = {'Youngcraft.exe': b'native fixture', 'nested/한글.txt': b'payload fixture'}
            for relative, payload in expected.items():
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(payload)
            buffer = ctypes.create_unicode_buffer(32768)
            self.assertTrue(ctypes.windll.kernel32.GetShortPathNameW(str(root), buffer, len(buffer)))
            self.assertNotEqual(str(root).casefold(), buffer.value.casefold(), '8.3 aliases unavailable')
            script = Path(temporary) / 'read-manifest.ps1'
            script.write_text(
                'param([string]$Helper,[string]$Root)\n'
                "$ErrorActionPreference = 'Stop'\n"
                '[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding\n'
                '. $Helper\n'
                '@(Get-YoungcraftPortableFileRecords -Root $Root) | ConvertTo-Json -Depth 5\n',
                encoding='utf-8-sig')
            for shell in ('powershell.exe', 'pwsh.exe'):
                executable = shutil.which(shell)
                self.assertIsNotNone(executable, shell)
                for alias in (str(root), buffer.value):
                    with self.subTest(shell=shell, root=alias):
                        environment = {key: value for key, value in os.environ.items()
                                       if key.lower() != 'psmodulepath'}
                        # Let each edition build its own standard module search path.
                        result = subprocess.run(
                            [executable, '-NoProfile', '-NonInteractive', '-File', str(script),
                             '-Helper', str(helper), '-Root', alias], env=environment,
                            capture_output=True, text=True, encoding='utf-8', errors='replace',
                            creationflags=subprocess.CREATE_NO_WINDOW, timeout=30)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        records = json.loads(result.stdout.lstrip('\ufeff'))
                        self.assertEqual({x['path'] for x in records}, set(expected))
                        for record in records:
                            payload = expected[record['path']]
                            self.assertEqual(record['sha256'].lower(), hashlib.sha256(payload).hexdigest())
                            self.assertEqual(record['size'], len(payload))


if __name__ == '__main__':
    unittest.main()
