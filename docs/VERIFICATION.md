# Release verification

The source includes reproducible checks for:

- Dependency checksums, archive path boundaries and source-package exclusions.
- Model rates, missing observations, simulation conservation, news decisions, player-minute reconstruction, rating provenance and market calculations.
- Fresh private MySQL identity, idempotent synthetic imports and complete CSV distributions.
- Ten rendered screens using DirectX WARP without a football API subscription.

## Version 0.2.1 checks

The Windows build, six setup/package tests, core model tests and six simulation-export checks pass. Transactional database tests cover empty leagues, season grouping, zero versus missing counts, partial lineups, 11 starters per team, player-field denominators and exclusion of future/out-of-window matches. Test records are rolled back.

Player-browser checks use the same application methods as the GUI to verify automatic loading, league switching and preservation of the selected fixture across a database reload. The graphics suite now includes database coverage and requires the player page to load without a manual test-only reload. All ten WARP screenshots passed locally.

## Verified locally on 6 November 2026

A source ZIP was extracted to a separate Windows directory containing spaces. It contained no database, credentials, vendor tree, installed toolchain, object files or application binary. Setup extracted verified dependency archives and compiled the app there; the large archives were reused from a download cache, and remaining pinned source/license files were fetched by the installer.

| Check | Result |
| --- | --- |
| Fresh dependency extraction and C++ build | Passed |
| Port 3317 deliberately occupied | Setup selected port 3318 |
| Fresh private MySQL and encrypted credentials | Passed |
| Synthetic data | 240 completed fixtures, one upcoming fixture, four example players and one example news item |
| Setup/archive/package unit tests | Five passed during clean setup; a sixth documentation-link check was added and passed during the final source audit |
| Core model and feature checks | Passed |
| Repeated demo import | Fixture counts preserved |
| Private database directory/port identity | Passed |
| Six distribution CSVs | 10,000 draws conserved in each |
| DirectX WARP screenshots | All nine screens exported |
| Second setup using only the verified offline cache | Passed; existing credentials and data preserved |
| Moving the installed folder on the same account | Startup regenerated paths; database-location and export checks passed |

The tests exposed and corrected a PowerShell 5.1 scalar-splatting issue in the Python launcher probe. Build/test scripts also restore their caller's working directory. The README screenshot comes from the synthetic installation and contains no private dataset or local user path.

This was a separate installation on the development Windows host, not a second physical PC or a pristine Windows VM. Prerequisites were already installed. The GitHub workflow is prepared for Windows Server 2022 runners; a remote CI result is available only after pushing and running it.

Current support target: Windows 10/11 x64 with documented prerequisites. Linux, macOS, ARM-native builds and non-ASCII installation directories are not validated. Authenticated API-Football integration requires a separate live subscription check. Tests do not establish out-of-sample betting accuracy.
