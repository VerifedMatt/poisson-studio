import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
import re

ROOT = Path(__file__).resolve().parents[1]
def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result
bootstrap = module(ROOT / 'bootstrap.py', 'bootstrap')
packager = module(ROOT / 'scripts/package_source.py', 'packager')

class SetupTests(unittest.TestCase):
    def test_archive_escape_rejected_before_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for attack in ('../escaped.txt', '..\\escaped.txt', 'C:/escaped.txt', '/absolute.txt'):
                archive = root / 'bad.zip'
                with zipfile.ZipFile(archive, 'w') as z:
                    z.writestr('safe.txt', 'safe')
                    z.writestr(attack, 'bad')
                with self.assertRaises(ValueError):
                    bootstrap.extract_zip(archive, root / 'output')
                self.assertFalse((root / 'output/safe.txt').exists())
    def test_checksum_mismatch_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            (cache / 'dependency.exe').write_bytes(b'wrong')
            with self.assertRaisesRegex(RuntimeError, 'Checksum mismatch'):
                bootstrap.fetch({'cache': 'dependency.exe', 'sha256': '0'*64}, cache, True)
    def test_lock_uses_pinned_https_downloads(self):
        lock = json.loads((ROOT / 'dependencies.lock.json').read_text())
        for entry in lock['dependencies']:
            self.assertTrue(entry['url'].startswith('https://'))
            self.assertNotIn('/master/', entry['url'])
            self.assertNotIn('/main/', entry['url'])
            self.assertRegex(entry['sha256'], r'^[0-9a-f]{64}$')
    def test_private_files_cannot_enter_source_package(self):
        names = [p.relative_to(ROOT).as_posix() for p in packager.source_files(ROOT)]
        self.assertIn('src/platform.cpp', names)
        self.assertIn('Setup.cmd', names)
        for name in names:
            self.assertFalse(name.startswith(('data/','exports/','tools/','vendor/','downloads/','test-output/')))
            self.assertNotIn(Path(name).name, ('config.json','secrets.bin','PoissonStudio.exe'))
    def test_source_archive_has_no_machine_user_path(self):
        for path in packager.source_files(ROOT):
            if path.suffix in ('.cpp','.hpp','.py','.ps1','.md','.json','.yml','.cmd'):
                self.assertIsNone(re.search(r'[A-Za-z]:[\\/]+Users[\\/]+', path.read_text(encoding='utf-8')), str(path))
    def test_relative_documentation_links_exist(self):
        for path in packager.source_files(ROOT):
            if path.suffix == '.md':
                for target in re.findall(r'\[[^\]]*\]\(([^)]+)\)', path.read_text(encoding='utf-8')):
                    if target.startswith(('https://', 'http://', '#')):
                        continue
                    self.assertTrue((path.parent / target.split('#')[0]).is_file(), f'{path}: {target}')

if __name__ == '__main__':
    unittest.main()
