# Setup and troubleshooting

## Supported environment

- Windows 10 or 11, x64; PowerShell 5.1 or newer, running as a 64-bit process.
- Python 3.10+ x64 for dependency setup, packaging and tests. No pip packages.
- Microsoft Visual C++ 2015–2022 x64 Redistributable for the official MySQL DLLs/server.
- DirectX 11 hardware or Windows WARP software rendering. A 1500×970 window is convenient; smaller windows scroll.
- About 4 GB free disk space before the first setup. Network access to GitHub and MySQL is needed for dependencies; live football/news imports need their respective providers.

Linux, macOS, native ARM builds and non-ASCII installation paths are not currently validated. Use a short, writable ASCII directory such as `D:\Apps\Poisson Studio`. Spaces are supported. Running from a protected installation directory or network share is not supported.

Official prerequisite downloads:

- Python: https://www.python.org/downloads/windows/
- VC++ runtime: https://aka.ms/vs/17/release/vc_redist.x64.exe

## First installation

Extract the source ZIP or clone the repository, then run **Setup.cmd**. Its PowerShell equivalent is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\setup.ps1 -Demo
```

`-Demo` creates a clearly labeled synthetic competition with enough observations for every count model. It includes example player and news records; none are real-world predictions. Omit `-Demo` for an empty database. A demo can also be added from **Data imports**.

Setup does the following:

1. Checks Python, x64 PowerShell and the VC++ runtime.
2. Downloads exact versions listed in `dependencies.lock.json`, verifies SHA-256 and extracts tools into the project.
3. Compiles and links the app using the local w64devkit compiler.
4. Finds a free loopback port between 3317 and 3399, or uses `-Port 3340` when specified.
5. Initializes a private MySQL instance under `data/mysql`, with fresh random credentials protected using Windows DPAPI.
6. Optionally seeds the synthetic demo. No real football data are copied or downloaded by this step.

Then open **Launch.cmd**. On startup the app starts its private database; on normal exit it stops an instance it started. No Windows service or global PATH entry is installed. Existing installations are checked without replacing their database or credentials. Close the app before rebuilding.

## Options and offline setup

```powershell
.\setup.ps1 -Demo -Port 3340
.\setup.ps1 -SkipDatabase                 # Build only; useful for an external server
.\setup.ps1 -CacheDirectory 'D:\Cache\Poisson'
.\setup.ps1 -Offline -CacheDirectory 'D:\Cache\Poisson' -Demo
```

A complete dependency cache can be copied between machines. Every file is still hash-checked before use. Copy only cached dependency downloads, not `data`, `config.json` or `secrets.bin`. `--offline` fails if any required cache file is absent or corrupt. To rebuild dependencies from scratch, use a fresh source directory or remove the generated `.bootstrap` state after closing the app.

The downloader does not silently accept a changed upstream file. A checksum failure requires investigation or a deliberate lock-file update; it never executes the mismatched toolchain archive.

## Import real data

Open **Data imports**, select a public competition and click **Import / update selected public league**. This fetches rolling five-year match history and upcoming fixtures. Optional player backfill runs separately with a request budget. See [provider coverage](DATA.md).

Public CSV and ESPN backfills need no API key. API-Football is optional, configured in **Settings**, and may require a paid historical-data plan. No account or subscription is created automatically.

## Updates and moving folders

Close the app, update source, and run `setup.ps1` again. Existing `data`, `config.json` and `secrets.bin` are retained. Schema updates add missing tables/columns rather than replacing fixtures.

You can move the entire installed folder on the **same Windows account and PC** after closing the app/database. Private MySQL paths are regenerated on the next startup. A port conflict can be resolved in Settings or by editing the non-secret `port` in `config.json` while the app is closed.

For another PC or Windows account, start with the source package and run setup there. DPAPI credentials are account/device-bound and are not a transferable setup file. This version has no automatic cross-account database migration tool; reimport provider data on the new installation. Do not copy the old encrypted secrets into the new install.

For a same-account backup, close the application and ensure its private server has stopped before copying `data`, `config.json` and `secrets.bin` together. Keep backups private. Copying live InnoDB files is not a supported backup method.

## External MySQL

`-SkipDatabase` builds without creating a server. Launch the app, dismiss the initial connection message, and enter an existing MySQL 8.4 database/account in **Settings**. Disable **Start private localhost MySQL automatically**. The account needs privileges to create/alter the application's tables and read/write their data. The database itself must already exist. Private-instance integration checks deliberately reject a different database directory.

## Troubleshooting

| Symptom | Action |
| --- | --- |
| Python not found or wrong architecture | Install Python x64, open a new shell and rerun setup. |
| `VCRUNTIME140`, `VCRUNTIME140_1` or `MSVCP140` missing | Install the official VC++ x64 runtime linked above. Do not copy arbitrary DLLs from another PC. |
| PowerShell blocks a downloaded script | Use the shown process-scoped `-ExecutionPolicy Bypass` command after reviewing the source. No permanent policy change is needed. |
| Download/certificate/proxy failure | Check HTTPS access and corporate certificate policy; use a prefilled verified cache where necessary. |
| Checksum mismatch | Remove only the named corrupt cache file and retry; if the upstream version changed, update the lock deliberately. |
| Port already in use | Use `setup.ps1 -Port <free port>` for a fresh setup; update Settings for an existing one. |
| MySQL initialization/startup fails | Inspect `data/initialize.log` and `data/server-console.log`. A partial database is not erased automatically; preserve logs and retry in a fresh folder. |
| Credentials cannot be decrypted | Use the original Windows account/PC, or perform a fresh installation. |
| Linker cannot replace the executable | Close the running app and rerun `build.ps1`. |
| Empty league / unavailable prediction | Import observations. Each metric needs 30 league matches and five for each selected team. Blank stats do not become zero. |
| DirectX initialization fails | Update graphics drivers or set `$env:POISSON_SOFTWARE_RENDERER='1'` before launch to use WARP. |
| Player rating has `*` | It is a Studio heuristic because the provider supplied no published rating. Hover to see its formula. |

Script output is in `test-output/`; runtime/database messages are under `data/`. Neither directory belongs in a GitHub issue attachment without review.
