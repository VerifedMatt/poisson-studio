# Using Poisson Studio

Complete the [setup guide](SETUP.md), then open Launch.cmd. A new setup can include a clearly labeled synthetic demo; import real matches before treating results as football predictions.

## Use the model

1. Open **Model & heatmaps** and choose a competition and two different teams, or an upcoming fixture.
2. Set the number of simulations (100–100,000,000), random seed, recency half-life and shrinkage.
3. Optionally enable news adjustments, acknowledge the accuracy warning, and confirm or reject automatically selected stories in the News tab.
4. Click **Run simulations**. Work runs in a background thread with progress and cancellation.
5. Switch among **Exact score**, **Total goals**, **Corners**, **Cards**, **Shots on target**, and **Bet suggestions**. Cards supports yellow, red, or yellow plus red counts.
6. Hover a cell for its full simulated frequency. **Export PNG** saves the rendered chart; **Export CSV** saves the full, untruncated count distribution. Exports include a JSON metadata file. Files go to `exports/`.

Score matrices use home counts as rows and away counts as columns. The final bin groups all larger outcomes, so no probability mass is silently dropped. Total goals are summed from the same simulated goals; the total-goals menu includes over/under probabilities. Corner and card heatmaps also show the most frequent count pairs.

## Import observations

Use **Data imports** for public competitions, custom CSVs or an optional API-Football account. Import match history first, then use a request budget for player/lineup backfill. Coverage counts show recorded observations for each metric. See [Data providers](DATA.md) for league codes, fields and limitations.

## Check database coverage

Open **Database coverage** to see completion bars and missing counts for goals, corners, yellow/red cards, shots on target, player records and both starting XIs. Select a competition to inspect its seasons, recorded date range and player-field coverage. Counts refresh after an import ends, including partial progress saved before cancellation or an error; **Refresh coverage** also picks up changes from another process.

Match percentages use stored finished fixtures in the rolling five-year window (through today). A metric requires both teams' values. Scheduled fixtures and older records are excluded. **Any player records** can mean a partial roster; **Both starting XIs** requires 11 identified starters for each team. Minutes, ratings and player shots-on-target percentages use stored appearances in that window. Ratings include provider/Studio ratings, and unused substitutes can legitimately have no rating. An attempted detail import with an empty response is not a completed lineup.

These percentages measure completeness of imported records. The total number of historical fixtures available from a source is unknown, so 100% does not certify that every real match or all five years were imported. The page also identifies the connected database folder/port and shows all-time database totals; those totals can exceed the five-year counters.

## News and lineups

**News & availability** searches public Google News RSS or parses a user-specified club RSS feed. Injury and suspension keywords help categorize results, but are not evidence that a player is definitely unavailable. Each item retains its title, publication information and source URL. You can add a manually verified note.

Enabling the news toggle opens an accuracy warning and offers to find news for both selected teams. Headlines naming the team and matching injury/suspension keywords are automatically selected. Recovery, unrelated sport and injury-time headlines are excluded where recognized. The **News review** queue asks **Yes, relevant** or **No, irrelevant**. Decisions persist across searches; a separate inclusion checkbox controls whether each item affects the model. Rejected or expired items are excluded. Selected items awaiting review are included while the news toggle is on, with their pending count shown beside the forecast.

Automatic selections start with editable scenario effects: -3% team goals, -3% team shots on target and +2% opponent goals per story. These small defaults are assumptions, not measured player effects. You can also set corner and card changes. Save effects and rerun the simulation. News expires seven days after its publication date; unknown dates require manual review. Identical headlines for the same team are counted once in the model, and aggregate rate multipliers are bounded to 0.25–2.0. Keyword relevance can be wrong, and competition-specific suspensions still need review.

**Players & lineups** displays minutes, goals, assists, shots, shots on target, cards and ratings. Minutes are taken from the source when available; otherwise completed matches use substitution/dismissal events and the nominal 90/120-minute duration, excluding added time. Derived minutes carry an asterisk. Incomplete event timelines remain unavailable. Unused substitutes have zero minutes and no local rating.

Choose **Competition** directly in this tab. On startup, the app selects the competition with the most stored player appearances. The newest fixture with stored player records loads automatically; **Only fixtures with player data** is enabled by default. Search by a team name or date, choose a fixture, or use **Newer fixture / Older fixture**. Turn off the filter to inspect a scheduled or unpopulated fixture. Player browsing includes older stored records even when they fall outside the model's five-year window. **Reload stored players** reads the local database; it does not fetch new data. Imports and database refreshes preserve the selected fixture where possible and reload its rows automatically.

If the table is empty, check the competition and team/date filter. Match CSV imports do not contain player appearances: use **Backfill / resume public player data** in Data imports for the same competition, or the optional API provider. Separate extracted installations normally have separate databases; the Database coverage page shows which one is connected.

Published ratings support numeric and string formats and take precedence. When the public feed supplies no rating, **Rating*** shows a separately stored **Studio score**: `6 + 0.85*goals + 0.55*assists + 0.12*shots_on_target - 0.30*yellow - 1.20*red - 0.70*own_goals`, with goalkeepers additionally receiving `+0.08*saves - 0.25*goals_conceded`. Scores are bounded to 1–10. This is an explicit local performance heuristic, not a provider rating or calibrated measure of player quality. Hover a rating or minute value to inspect its source. **Refresh imported public lineups** updates existing records instead of skipping their import checkpoints.

The count model remains fitted to team match totals. Player ratings do not automatically change scoring rates; use the availability scenarios to investigate those effects.

## Bet suggestions

After running a simulation, open **Bet suggestions** for winner, goal totals, corner totals and the selected card-count definition. Choose half-goal/count lines. The highlighted pick in each market has the highest simulated probability when no odds are entered. Enter current decimal odds to rank the priced picks by `probability * odds - 1`, the model's expected net return per unit stake. The screen reports when none of the entered prices has a positive model return. Fair odds are `1 / probability`; they do not include bookmaker margin or commission. Missing metrics produce no suggestion. Probabilities are model estimates, and related picks are not independent.

## Model and storage details

See [Model](MODEL.md) for fitting assumptions and [Architecture](ARCHITECTURE.md) for database layout, missing-value handling and local state.
