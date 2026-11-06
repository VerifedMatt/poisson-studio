param([string]$LeagueCode = '', [ValidateRange(10,20000)][int]$RequestBudget = 100)
. "$PSScriptRoot\scripts\common.ps1"
Invoke-Poisson -Arguments @('--import-cache') -Name 'refresh-cache'
if ($LeagueCode) { Invoke-Poisson -Arguments @('--refresh-lineups', $LeagueCode, "$RequestBudget") -Name 'refresh-lineups' -TimeoutSeconds 7200 }
Invoke-Poisson -Arguments @('--refresh-news-selection') -Name 'refresh-news'
Invoke-Poisson -Arguments @('--coverage-v2') -Name 'coverage'
