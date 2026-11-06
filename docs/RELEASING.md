# GitHub setup and source releases

## First push

The prepared repository contains source, documentation and licensed assets. Do not initialize Git around the entire Windows home directory. From the project directory:

```powershell
git status --short
git add .
git diff --cached --stat
git commit -m "Prepare portable Windows release"
git branch -M main
git remote add origin https://github.com/YOUR-USER/poisson-studio.git
git push -u origin main
```

Create the empty destination repository in your GitHub account first and replace `YOUR-USER`. No destination or credentials are embedded in this project. No push, repository creation, public release or external publication is performed by setup.

Enable Actions if required by your repository policy. The Windows workflow should pass before tagging a release. Branch protection and required checks can reference **Windows build and verification / windows**.

## Package a release

1. Update `VERSION` and `CHANGELOG.md`.
2. Build and run `verify.ps1 -Integration -Graphics` from a fresh setup.
3. Run `python scripts/package_source.py`.
4. Inspect the generated ZIP and its SHA-256 sidecar under `dist/`.
5. Extract the ZIP into a different writable directory and run `setup.ps1 -Demo` there before publishing.
6. Commit/tag the reviewed source, then attach the source ZIP and checksum to a GitHub release.

The packager includes only declared root files, `src`, scripts, tests, docs, GitHub configuration and the Inter font/license. It refuses missing required files, files over 5 MB and links escaping the project. It never walks `data`, `downloads`, `tools`, `vendor`, `exports` or user configuration into the ZIP. `.gitignore` protects normal Git staging, while the separate packaging allowlist protects release artifacts.

## License and distribution scope

This release is **source-first**: it does not bundle executable builds, the MySQL server/client DLLs, the compiler or downloaded football data. Recipients obtain pinned dependencies from the original providers during setup. This avoids making a prebuilt dependency bundle part of the project's release artifact.

Project code is GPL-2.0-only; third-party components retain their licenses. If you introduce binary distributions later, include corresponding source and license material required by the libraries and runtime components you ship, and review that release's contents separately. Do not assume that including this app's source alone fulfills obligations for separately bundled GPL dependencies.

Do not publish your initialized installation folder. `config.json`, `secrets.bin`, `data/`, database logs, API keys and fetched datasets belong to an installation, not to a source release.
