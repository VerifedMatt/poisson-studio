param([switch]$Integration, [switch]$Graphics)
. "$PSScriptRoot\scripts\common.ps1"
Push-Location -LiteralPath $PSScriptRoot
try {
$python = Get-PythonCommand
$pythonArguments = @($python.Prefix) + @('-m','unittest','discover','-s','tests','-p','test_*.py')
& $python.Executable @pythonArguments
if ($LASTEXITCODE -ne 0) { throw 'Setup/package tests failed.' }
Invoke-Poisson -Arguments @('--self-test') -Name 'core'
if ($Integration -or $Graphics) {
    Invoke-Poisson -Arguments @('--seed-demo') -Name 'demo-first'
    $before = @(Invoke-Poisson -Arguments @('--db-check') -Name 'database-before')[0]
    Invoke-Poisson -Arguments @('--seed-demo') -Name 'demo-repeat'
    $after = @(Invoke-Poisson -Arguments @('--db-check') -Name 'database-after')[0]
    if ($before -ne $after) { throw 'Repeated demo import changed fixture counts.' }
    Invoke-Poisson -Arguments @('--integration-check') -Name 'integration'
    foreach ($metric in 0..5) {
        $rows = Import-Csv -LiteralPath "exports\demo_$metric.csv"
        if (($rows | Measure-Object -Property occurrences -Sum).Sum -ne 10000) { throw "Incomplete distribution $metric" }
    }
}
if ($Graphics) {
    $previous = $env:POISSON_SOFTWARE_RENDERER
    try { $env:POISSON_SOFTWARE_RENDERER = '1'; Invoke-Poisson -Arguments @('--smoke') -Name 'graphics' }
    finally { $env:POISSON_SOFTWARE_RENDERER = $previous }
    foreach ($index in 0..8) { if ((Get-Item -LiteralPath "exports\smoke_$index.png").Length -lt 1000) { throw "Empty screenshot $index" } }
}
Write-Host 'All requested checks passed.'
} finally { Pop-Location }
