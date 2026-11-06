Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:ProjectRoot = Split-Path -Parent $PSScriptRoot

function ConvertTo-NativeArgument([string]$Value) {
    # Windows CRT quoting; values are never evaluated by a shell.
    $escaped = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
    $escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
    return '"' + $escaped + '"'
}
function Invoke-Poisson {
    param([string[]]$Arguments, [string]$Name = 'app', [int]$TimeoutSeconds = 300)
    $outputDir = Join-Path $script:ProjectRoot 'test-output'
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
    $stdout = Join-Path $outputDir "$Name.stdout.log"
    $stderr = Join-Path $outputDir "$Name.stderr.log"
    $commandLine = ($Arguments | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' '
    $run = Start-Process -FilePath (Join-Path $script:ProjectRoot 'PoissonStudio.exe') -WorkingDirectory $script:ProjectRoot -ArgumentList $commandLine -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    [void]$run.Handle
    if (-not $run.WaitForExit($TimeoutSeconds * 1000)) { $run.Kill(); throw "$Name timed out. See test-output and data/server-console.log." }
    $run.Refresh()
    if ($run.ExitCode -ne 0) { $message = Get-Content -LiteralPath $stderr -Raw -ErrorAction SilentlyContinue; throw "$Name failed ($($run.ExitCode)): $message" }
    if (Test-Path -LiteralPath $stdout) { Get-Content -LiteralPath $stdout }
}
function Find-PrivatePort([int]$Requested = 0) {
    $candidates = if ($Requested -gt 0) { @($Requested) } else { 3317..3399 }
    foreach ($candidate in $candidates) {
        $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, $candidate)
        $listener.Server.ExclusiveAddressUse = $true
        try { $listener.Start(); return $candidate }
        catch { if ($Requested -gt 0) { throw "Port $Requested is in use; select another -Port." } }
        finally { $listener.Stop() }
    }
    throw 'No free localhost port found between 3317 and 3399.'
}
function Get-PythonCommand {
    foreach ($name in @('py', 'python', 'python3')) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command) {
            [string[]]$prefix = @()
            if ($name -eq 'py') { $prefix = @('-3') }
            $probeArguments = @($prefix) + @('-c','import sys; sys.exit(0 if sys.version_info >= (3,10) and sys.maxsize > 2**32 else 1)')
            & $command.Source @probeArguments 2>$null
            if ($LASTEXITCODE -eq 0) { return @{ Executable = $command.Source; Prefix = $prefix } }
        }
    }
    throw 'Install 64-bit Python 3.10+ from https://www.python.org/downloads/windows/ and rerun setup. No pip packages are needed.'
}
