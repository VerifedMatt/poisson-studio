# Data sources and imports

The repository ships no real football dataset. Importers obtain data at runtime and retain structured observations in MySQL. Availability and permitted reuse are controlled by the source; public access does not imply redistribution rights.

## Public match history

Source: https://www.football-data.co.uk/data.php

| Country | Competition codes |
| --- | --- |
| England | E0 Premier League; E1 Championship; E2 League One; E3 League Two; EC National League |
| Scotland | SC0 Premiership; SC1 Championship; SC2 League One; SC3 League Two |
| Spain | SP1 La Liga; SP2 Segunda Division |
| Italy | I1 Serie A; I2 Serie B |
| France | F1 Ligue 1; F2 Ligue 2 |
| Germany | D1 Bundesliga; D2 2. Bundesliga |
| Netherlands | N1 Eredivisie |
| Belgium | B1 Pro League |
| Portugal | P1 Primeira Liga |
| Turkey | T1 Super Lig |
| Greece | G1 Super League |

Season files covering the rolling five-year window are downloaded, then rows older than the cutoff are excluded. Upcoming fixtures are stored separately from completed observations. Goals, corners, yellow/red cards and shots on target are imported where present. Missing values remain SQL NULL. Raw public CSV snapshots are retained locally in `data/public_cache`; content hashes skip unchanged downloads.

## Public players and lineups

ESPN public scoreboard/summary feeds supply rosters, starter flags, available player statistics, team statistics and key events. Records are linked only after matching the date and both team names. Unknown or ambiguous matches are reported rather than guessed. The endpoint is not a contracted API, and five-year coverage is not guaranteed.

Backfills work backwards from recent matches with a request budget and delay between requests. Completed summaries are checkpointed. **Refresh imported public lineups** explicitly revisits previously imported summaries. Player minutes can be reconstructed from completed match events; local ratings are stored separately from any provider rating.

This process takes a very long time, once populated entirely you have freedom to do anything very accurately! Anyways, if you only want to use poisson model without player lineups affecting the weights you still can.
## API-Football

Documentation: https://www.api-football.com/documentation-v3

Enter your own key in Settings. The adapter supports league discovery, five-year fixtures, team statistics, lineups, appearances, events and injuries. Provider coverage and subscription quotas apply. Detail imports are budgeted and resumable; future lineup/injury requests remain refreshable. Live authenticated API operation is not part of automated CI and has not been verified with a subscription in this release.

## Custom CSV

UTF-8 CSV accepts quoted commas/doubled quotes and a BOM. Dates may be `YYYY-MM-DD` or `DD/MM/YYYY` (two-digit years are interpreted as 20xx).

| Columns | Meaning |
| --- | --- |
| Date, HomeTeam, AwayTeam | Required fixture identity |
| FTHG, FTAG | Full-time home/away goals |
| HC, AC | Home/away corners |
| HY, AY | Home/away yellow cards |
| HR, AR | Home/away red cards |
| HST, AST | Home/away shots on target |

Example of the format (illustrative data only):

```csv
Date,HomeTeam,AwayTeam,FTHG,FTAG,HC,AC,HY,AY,HR,AR,HST,AST
2026-10-01,Northbridge FC,Riverside United,2,1,6,4,2,3,0,0,5,3
2026-10-02,Eastford City,Westhaven Athletic,0,0,,,1,2,0,0,2,1
```

Leave unknown fields empty; a missing corner count is not zero. A small format example is insufficient for a fitted prediction. The synthetic demo generator supplies a complete test dataset with dates relative to installation time.

## News

News search uses Google News RSS or a supplied HTTPS RSS feed. Titles, dates and source links are retained. Automatic selection uses team/availability keywords, excludes recognized unrelated contexts and requires a current publication date. It is not a full article-understanding system. Users confirm or reject relevance and can edit scenario effects. Rejected items remain excluded across searches. Review a suspension's competition before applying it.

## Storage and coverage

Shared integer IDs, nullable count fields, composite keys and dictionary-based player statistics avoid repeated raw JSON. Reimports update existing observations instead of creating duplicate fixtures. Coverage counts in the app describe what has actually been recorded. They do not imply every fixture, player, league or historical season exists in a source.

The **Database coverage** page groups stored finished fixtures by competition and provider season within the rolling five-year window. FT/AET/PEN statuses count as finished; shots-on-target coverage requires FT to match the model's regulation-time filtering. Each metric requires observations for both teams. Player counts are aggregated before joining fixture counts so a roster cannot inflate the match denominator. A successful detail request with no roster remains incomplete. No expected league/season fixture total is invented for the completion percentages.
