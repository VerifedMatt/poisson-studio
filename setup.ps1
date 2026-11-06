param([switch]$Demo, [switch]$SkipDatabase, [switch]$Offline, [string]$CacheDirectory = '', [ValidateRange(0,65535)][int]$Port = 0)
. "$PSScriptRoot\scripts\common.ps1"
if (-not [Environment]::Is64BitOperatingSystem -or -not [Environment]::Is64BitProcess) { throw 'Run 64-bit PowerShell on 64-bit Windows.' }
if ($Port -gt 0 -and $Port -lt 1024) { throw 'Choose a database port from 1024 to 65535.' }
foreach ($dll in @('vcruntime140.dll','vcruntime140_1.dll','msvcp140.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path ([Environment]::SystemDirectory) $dll))) { throw 'Install the Microsoft Visual C++ 2015-2022 x64 runtime required by MySQL: https://aka.ms/vs/17/release/vc_redist.x64.exe . Then rerun setup.' }
}
$python = Get-PythonCommand
$arguments = @((Join-Path $PSScriptRoot 'bootstrap.py'))
if ($CacheDirectory) { $arguments += @('--cache-dir', $CacheDirectory) }
if ($Offline) { $arguments += '--offline' }
$pythonArguments = @($python.Prefix) + $arguments
& $python.Executable @pythonArguments
if ($LASTEXITCODE -ne 0) { throw 'Dependency installation failed.' }
& "$PSScriptRoot\build.ps1"
if (-not $SkipDatabase) {
    if (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'data\mysql\mysql')) {
        Write-Host 'Existing database found; verifying without replacing credentials or data.'
        Invoke-Poisson -Arguments @('--db-check') -Name 'setup-existing'
    } else {
        $chosenPort = Find-PrivatePort $Port
        Invoke-Poisson -Arguments @('--setup-db', "$chosenPort") -Name 'setup-database' -TimeoutSeconds 240
    }
    if ($Demo) { Invoke-Poisson -Arguments @('--seed-demo') -Name 'setup-demo' }
}
Write-Host 'Setup complete. Open Launch.cmd. Demo data, when enabled, are synthetic; import real matches from Data imports.'
