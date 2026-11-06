# Development, CLI and verification

## Build

Run `setup.ps1 -Demo` once, then use `build.ps1`. For symbols and assertions use `build.ps1 -DebugBuild`. The local w64devkit compiler targets x64 Windows and C++23. Changes to the dependency lock or build flags invalidate object-file reuse. Keep the app closed while replacing the executable.

Dependencies are pinned by version/commit and SHA-256 in `dependencies.lock.json`. The MySQL version in the lock also determines the runtime server directory. To update dependencies, fetch official artifacts, review source/license changes, update their hashes/versions, rerun bootstrap and perform a clean verification. Never change a checksum just to silence an unexplained mismatch.

The app is a Windows GUI-subsystem executable: direct execution from PowerShell can return before the process exits. Use the wrapper, which handles native argument quoting, exit codes, timeouts and logs:

```powershell
.\scripts\run.ps1 -AppArguments @('--help')
.\scripts\run.ps1 -AppArguments @('--import-league', 'E0')
.\scripts\run.ps1 -AppArguments @('--simulate', 'E0', 'Arsenal', 'Chelsea', '100000')
```

## Command reference

| Command | Behavior |
| --- | --- |
| `--help`, `--version` | Prints information without opening the database |
| `--setup-db [PORT]` | Creates a new private database; refuses an existing or partial data directory |
| `--seed-demo` | Adds synthetic fixtures/players/news, leaving real leagues untouched; repeat calls are idempotent |
| `--self-test` | Core/model/feature tests without MySQL or network |
| `--integration-check` | Checks private database identity, synthetic observations, fits and exports |
| `--db-check`, `--coverage-v2` | Database and metric coverage |
| `--coverage` | Connected database identity and five-year lineup coverage by competition |
| `--lineups-check` | Reads stored leagues and checks player auto-loading, league switching and selection preservation on refresh; no imports |
| `--smoke` | Renders ten demo screens, including database coverage, and exits; run `--seed-demo` first |
| `--import-league CODE` | Updates one public competition |
| `--import-all`, `--import-cache` | All public imports, or reimport locally cached CSVs |
| `--import-csv PATH LEAGUE_NAME` | Custom CSV import |
| `--enrich CODE REQUESTS` | Resumable public player backfill |
| `--refresh-lineups CODE REQUESTS` | Revisit imported public summaries |
| `--search-news CODE TEAM` | Search public RSS for a stored public-league team |
| `--refresh-news-selection` | Reclassify pending news while preserving explicit relevance decisions |
| `--simulate CODE HOME AWAY DRAWS` | Forecast a public-league matchup and export distributions |
| `--shutdown-db` | Explicitly shut down the configured database; use only for the intended private instance |

CLI provider codes refer to the public competitions listed in [DATA.md](DATA.md). The GUI also supports custom and API-Football leagues. Import failures and coverage gaps may be reported in the command's summary; inspect it rather than assuming every historical request succeeded.

## Tests

```powershell
.\verify.ps1
.\verify.ps1 -Integration
.\verify.ps1 -Integration -Graphics
```

Default checks cover setup/archive boundaries, checksum failures, package exclusions, CSV/date handling, known Poisson rates, missing observations, probability conservation, shots-on-target fitting, news effects/exclusions, minutes/ratings and fair-odds calculations.

Integration checks add the synthetic demo, verify idempotent seeding, verify that MySQL is actually using this installation's data directory/port, fit all six distributions and check CSV draw totals. Temporary coverage fixtures test missing versus zero counts, empty leagues, partial rosters, both starting XIs, season grouping and exclusion of old/future fixtures; their transaction is rolled back. The player browser check verifies automatic loading and preservation on refresh. Graphics checks force the Windows WARP software renderer and export exact score, total goals, corners, cards, shots on target, suggestions, players, news, coverage and the warning modal. The player screenshot must load automatically without a test-only manual load. These checks do not call live football or news endpoints. They are intended for a disposable fresh installation; `-Integration` explicitly adds a demo league to the configured private database.

Logs are under `test-output/`; demo distributions/screenshots are under `exports/`. No secrets are expected in ordinary test logs, but neither entire `data` nor entire project directories should be attached as CI artifacts.

## GitHub Actions

`.github/workflows/windows.yml` builds on `windows-2022` with Python 3.12, caches only hash-verified dependency downloads, sets up a fresh private database, and runs the full tests. It uploads the source package and selected test logs/images. Workflow permissions are read-only; no repository secrets or API subscriptions are needed. The workflow is prepared locally; its first actual GitHub run occurs after pushing the repository.
