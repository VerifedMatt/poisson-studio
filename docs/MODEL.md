# Model and interpretation

Separate regularized Poisson regressions estimate goals, corners, yellow cards, red cards and shots on target. For each metric:

```text
log(home mean) = intercept + home advantage + home attack + away defensive weakness
log(away mean) = intercept + away attack + home defensive weakness
weight(age) = exp(-log(2) * age_days / half_life_days)
```

Training uses available observations in the rolling five-year window before the current forecast date. A ridge penalty shrinks team coefficients; a small intercept prior stabilizes rare events. Fits require 30 league matches and five observations for each selected team. A Pearson dispersion diagnostic reports excess historical variance. Missing values are excluded separately for each metric.

The simulator uses `std::mt19937_64` and Poisson draws with an adjustable seed and 100–100,000,000 iterations. It stores aggregate counts, not every draw. Exact-score grids group the tail into a final `N+` bin; exports retain the untruncated count distribution. Total goals sum the same simulated home/away goals. Combined cards sum simulated yellow and red counts. Shots on target are a separate count model, not a joint constraint on simulated goals.

Seeds are reproducible within this build; the C++ standard does not require different standard-library implementations to produce identical Poisson sequences. More simulations reduce Monte Carlo error without fixing model misspecification.

## Limitations

- Goals refer to regulation/full-time results. Extra-time API statistics are not treated as regulation-only observations.
- Models do not simulate tactical changes, red-card feedback, score dependence, fitted player effects or bookmaker settlement exceptions.
- Corners/cards/shots may be overdispersed and dependent. Tail probabilities from independent Poisson models are approximate.
- There is no demonstrated out-of-sample calibration or guaranteed betting edge. Synthetic tests verify implementation behavior, not forecasting profitability.

## News scenarios

The news toggle opens an accuracy warning. Automatically selected pending stories are included when enabled, until confirmed, excluded or rejected. Defaults are -3% team goals, -3% team shots on target and +2% opponent goals per story. These are editable scenario assumptions, not estimates learned from player absences. Identical headlines for one team are counted once; aggregate multipliers are bounded between 0.25 and 2.0. Other related articles can still describe the same situation, so review duplicates and relevance.

Expired, rejected or unselected items do not affect the next simulation. The forecast displays the number of selected stories still awaiting review. Saving news changes requires rerunning the simulation.

## Minutes and ratings

Provider minutes take priority. Otherwise a completed match's substitution/dismissal timeline supplies nominal minutes over 90/120 minutes, excluding added time. Incomplete timelines remain unavailable. Derived minutes carry `*`.

Published provider ratings take priority. When absent and enough played statistics exist, **Rating*** displays a separate Studio score:

```text
6 + 0.85*goals + 0.55*assists + 0.12*shots_on_target
  - 0.30*yellow - 1.20*red - 0.70*own_goals
Goalkeepers: add 0.08*saves - 0.25*goals_conceded
Bound to [1, 10], round to one decimal
```

This heuristic is not a published provider rating or calibrated player-quality measure. Unused substitutes are unrated. Ratings do not automatically change forecast rates.

## Betting suggestions

For a selected fixture, the app compares 1X2 winner probabilities and over/under probabilities for chosen half-count goal, corner and card lines. Without entered odds it highlights the most probable outcome in each market. With decimal odds it ranks priced outcomes by:

```text
fair odds = 1 / probability
expected net return per unit stake = probability * decimal_odds - 1
```

The app identifies markets with no positive modeled return among entered prices. Commission and bookmaker margin are not included. Cards use the displayed count convention, not booking points. Related bets must not be treated as independent when forming multiples.
