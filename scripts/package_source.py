"""Build a GitHub-ready source ZIP from an explicit allowlist, never a directory dump."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = {
    '.gitignore', '.gitattributes', 'README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md',
    'CONTRIBUTING.md', 'SECURITY.md', 'CHANGELOG.md', 'VERSION',
    'dependencies.lock.json', 'bootstrap.py', 'build.ps1', 'setup.ps1',
    'verify.ps1', 'Launch.cmd', 'Setup.cmd', 'refresh_data.ps1',
}
TREES = {'src': {'.cpp', '.hpp'}, 'scripts': {'.py', '.ps1'},
         'tests': {'.py'}, 'docs': {'.md', '.png', '.csv'}, '.github': {'.yml', '.md'}}
ASSETS = {'assets/Inter.ttf', 'assets/Inter-LICENSE.txt'}

def source_files(root: Path) -> list[Path]:
    files = []
    for name in sorted(ROOT_FILES | ASSETS):
        path = root / name
        if not path.is_file():
            raise ValueError(f'Missing source-package file: {name}')
        files.append(path)
    for directory, extensions in TREES.items():
        for path in (root / directory).rglob('*'):
            if path.is_file() and path.suffix in extensions and '__pycache__' not in path.parts:
                files.append(path)
    for path in files:
        if path.is_symlink() or not path.resolve().is_relative_to(root.resolve()):
            raise ValueError(f'Source package cannot contain links outside the project: {path}')
        if path.stat().st_size > 5 * 1024 * 1024:
            raise ValueError(f'Unexpected large source asset: {path.name}')
    return sorted(set(files))

def package(root: Path, output: Path) -> Path:
    version = (root / 'VERSION').read_text().strip()
    if not re.fullmatch(r'\d+\.\d+\.\d+(?:-[a-zA-Z0-9.-]+)?', version):
        raise ValueError('Invalid VERSION')
    prefix = f'PoissonStudio-{version}-source'
    output.mkdir(parents=True, exist_ok=True)
    archive = output / (prefix + '.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path in source_files(root):
            name = prefix + '/' + path.relative_to(root).as_posix()
            entry = zipfile.ZipInfo(name, date_time=(2000, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            z.writestr(entry, path.read_bytes())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_suffix('.zip.sha256').write_text(f'{digest}  {archive.name}\n', encoding='ascii')
    return archive

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    print(package(ROOT, args.output.resolve()))
