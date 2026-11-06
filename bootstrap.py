"""Install verified project-local dependencies. Python 3.10+, no pip packages."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import time
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parent

def inside(root: Path, relative: str) -> Path:
    """Reject manifest/archive paths escaping their declared destination."""
    if not relative or ":" in relative or relative.startswith(("/", "\\")):
        raise ValueError(f"Unsafe relative path: {relative!r}")
    path = (root / relative.replace("\\", "/")).resolve()
    if not path.is_relative_to(root.resolve()):
        raise ValueError(f"Path escapes destination: {relative!r}")
    return path

def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()

def extract_zip(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            inside(destination, entry.filename)
            if stat.S_ISLNK(entry.external_attr >> 16):
                raise ValueError(f"Archive symlink is not allowed: {entry.filename}")
        for entry in source.infolist():
            target = inside(destination, entry.filename)
            if entry.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.open(entry) as src, target.open("wb") as dst:
                    shutil.copyfileobj(src, dst)

def fetch(entry: dict, cache: Path, offline: bool) -> Path:
    path = inside(cache, entry["cache"])
    expected = entry["sha256"]
    if path.is_file():
        if sha256(path) != expected:
            raise RuntimeError(f"Checksum mismatch: {path}. Remove that cache file and retry; it was not executed.")
        return path
    if offline:
        raise RuntimeError(f"Offline cache is missing {entry['cache']}")
    if not entry["url"].startswith("https://"):
        raise ValueError("Dependency downloads must use HTTPS")
    partial = path.with_name(path.name + ".part")
    path.parent.mkdir(parents=True, exist_ok=True)
    for attempt in range(3):
        try:
            request = urllib.request.Request(entry["url"], headers={"User-Agent": "PoissonStudio-Setup/0.2"})
            with urllib.request.urlopen(request, timeout=90) as response, partial.open("wb") as out:
                if not response.geturl().startswith("https://"):
                    raise RuntimeError("Insecure dependency redirect")
                shutil.copyfileobj(response, out, 1024 * 1024)
            break
        except (OSError, TimeoutError):
            if attempt == 2:
                raise
            time.sleep(attempt + 1)
    if sha256(partial) != expected:
        partial.unlink(missing_ok=True)
        raise RuntimeError(f"Downloaded checksum mismatch for {entry['name']}; installation stopped")
    partial.replace(path)
    return path

def install(cache: Path, offline: bool = False) -> None:
    manifest = json.loads((ROOT / "dependencies.lock.json").read_text(encoding="utf-8"))
    if manifest.get("schema") != 1:
        raise ValueError("Unsupported dependency lock format")
    state_path = ROOT / ".bootstrap" / "state.json"
    state = json.loads(state_path.read_text()) if state_path.exists() else {}
    for entry in manifest["dependencies"]:
        print(f"Checking {entry['name']} {entry['version']}", flush=True)
        archive = fetch(entry, cache, offline)
        destination = inside(ROOT, entry["destination"])
        if entry["kind"] == "file":
            if not destination.exists() or sha256(destination) != entry["sha256"]:
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(archive, destination)
        else:
            required = [inside(ROOT, p) for p in entry["required"]]
            if state.get(entry["name"]) == entry["sha256"] and all(p.exists() for p in required):
                continue
            print(f"Extracting {entry['name']}...", flush=True)
            if entry["kind"] == "zip":
                extract_zip(archive, destination)
            elif entry["kind"] == "sfx":
                destination.mkdir(parents=True, exist_ok=True)
                # Execute the official self-extractor only after SHA-256 verification.
                subprocess.run([str(archive), "-y", "-o" + str(destination)], check=True,
                               creationflags=subprocess.CREATE_NO_WINDOW, timeout=180)
            else:
                raise ValueError(f"Unknown dependency kind: {entry['kind']}")
            if not all(p.exists() for p in required):
                raise RuntimeError(f"Incomplete extraction for {entry['name']}")
        state[entry["name"]] = entry["sha256"]
        state_path.parent.mkdir(exist_ok=True)
        state_path.write_text(json.dumps(state, indent=2), encoding="utf-8")
    print("Dependencies ready. No database or user credentials were copied.", flush=True)

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache-dir", type=Path, default=ROOT / "downloads")
    parser.add_argument("--offline", action="store_true", help="Use verified cache files only")
    args = parser.parse_args()
    if sys.version_info < (3, 10) or os.name != "nt" or sys.maxsize < 2**32:
        parser.error("Use 64-bit Python 3.10+ on 64-bit Windows")
    try:
        install(args.cache_dir.resolve(), args.offline)
        return 0
    except Exception as error:
        print(f"Setup failed: {error}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
