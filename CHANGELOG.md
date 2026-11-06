# Changelog

## 0.2.1

- Database coverage page with completion bars, missing counts, league/season breakdowns, recorded date ranges and the connected database location.
- Separate coverage for any player records, both starting XIs, player minutes, ratings and shots on target; attempted empty imports do not count as completed lineups.
- Players & lineups automatically loads stored data after league selection/imports, preserves the selected fixture on refresh and provides its own competition selector, team/date search and populated-fixture filter.
- Startup selects the competition with the most stored player appearances instead of defaulting to an unrelated empty competition.
- Stored historical players remain browseable outside the model's five-year window; empty selections clear stale rows and explain how to find/import records.
- Coverage/lineup regression checks and a tenth graphics screenshot in Windows verification.

## 0.2.0

- Reproducible Windows setup with SHA-256-pinned downloads, a local compiler and fresh MySQL credentials.
- Automatic free-port selection, configurable private ports, regenerated server paths after moving folders, and clean-install documentation.
- Offline synthetic demo, deterministic integration/graphics checks, GitHub Actions and source-only release packaging.
- Exact score, total goals, corners, cards and shots-on-target heatmaps with PNG/CSV exports.
- Event-derived player minutes, separate provider/Studio ratings and shots-on-target player columns.
- Automatic availability headline selection with Yes/No review, persistent exclusions and an accuracy warning.
- Inter font, refreshed colors and betting suggestions with model probabilities, fair odds and entered-price expected returns.
