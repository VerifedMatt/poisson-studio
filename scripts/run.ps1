param([Parameter(ValueFromRemainingArguments = $true)][string[]]$AppArguments)
. "$PSScriptRoot\common.ps1"
if (-not $AppArguments) { $AppArguments = @('--help') }
Invoke-Poisson -Arguments $AppArguments -Name 'command' -TimeoutSeconds 7200
