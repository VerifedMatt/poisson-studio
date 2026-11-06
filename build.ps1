param([switch]$DebugBuild)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Push-Location -LiteralPath $PSScriptRoot
try {
$lock = Get-Content -LiteralPath 'dependencies.lock.json' -Raw | ConvertFrom-Json
$mysqlVersion = ($lock.dependencies | Where-Object name -eq 'mysql').version
$imguiVersion = ($lock.dependencies | Where-Object name -eq 'imgui').version
$compiler = Join-Path $PSScriptRoot 'tools\w64devkit\bin\g++.exe'
$mysqlDir = Join-Path $PSScriptRoot "tools\mysql-$mysqlVersion-winx64"
$imguiDir = Join-Path $PSScriptRoot "vendor\imgui-$imguiVersion"
foreach ($needed in @($compiler,"$mysqlDir\lib\libmysql.lib","$imguiDir\imgui.cpp",'vendor\json.hpp','vendor\pugixml.cpp','vendor\stb_image_write.h')) {
    if (-not (Test-Path -LiteralPath $needed)) { throw "Missing dependency: $needed. Run Setup.cmd or bootstrap.py first." }
}
New-Item -ItemType Directory -Force -Path 'build' | Out-Null
$sources = @('src/core.cpp','src/features.cpp','src/database.cpp','src/demo.cpp','src/platform.cpp','src/providers.cpp','src/espn.cpp','src/ui.cpp','src/main.cpp','vendor/pugixml.cpp',
 "vendor/imgui-$imguiVersion/imgui.cpp","vendor/imgui-$imguiVersion/imgui_draw.cpp","vendor/imgui-$imguiVersion/imgui_tables.cpp","vendor/imgui-$imguiVersion/imgui_widgets.cpp",
 "vendor/imgui-$imguiVersion/backends/imgui_impl_win32.cpp","vendor/imgui-$imguiVersion/backends/imgui_impl_dx11.cpp")
$flags = @('-std=c++23','-DUNICODE','-D_UNICODE','-DNOMINMAX','-DWIN32_LEAN_AND_MEAN','-Isrc','-Ivendor',"-I$imguiDir","-I$mysqlDir\include")
if ($DebugBuild) { $flags += @('-O0','-g') } else { $flags += @('-O2','-DNDEBUG') }
$signature = (Get-FileHash 'dependencies.lock.json' -Algorithm SHA256).Hash + ($flags -join '|')
$savedSignature = if (Test-Path 'build\configuration.txt') { Get-Content 'build\configuration.txt' -Raw } else { '' }
$rebuild = $savedSignature.Trim() -ne $signature
$headerTime = (Get-ChildItem src -Filter *.hpp | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).LastWriteTimeUtc
$objects = @()
foreach ($source in $sources) {
    $object = Join-Path $PSScriptRoot ('build\' + ($source -replace '[/\\.]','_') + '.o')
    $objects += $object
    $sourceFile = Get-Item -LiteralPath $source
    $objectFile = Get-Item -LiteralPath $object -ErrorAction SilentlyContinue
    if (-not $rebuild -and $objectFile -and $objectFile.LastWriteTimeUtc -gt $sourceFile.LastWriteTimeUtc -and $objectFile.LastWriteTimeUtc -gt $headerTime) { continue }
    Write-Host "Compiling $source"
    & $compiler @flags -c $source -o $object
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $source" }
}
Write-Host 'Linking PoissonStudio.exe'
& $compiler -mwindows -static-libgcc -static-libstdc++ $objects "$mysqlDir\lib\libmysql.lib" -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -lwinhttp -lcrypt32 -lbcrypt -lshell32 -limm32 -lole32 -luuid -lws2_32 -o (Join-Path $PSScriptRoot 'PoissonStudio.exe')
if ($LASTEXITCODE -ne 0) { throw 'Link failed. Close the running app before rebuilding.' }
Copy-Item -LiteralPath "$mysqlDir\lib\libmysql.dll" -Destination $PSScriptRoot -Force
foreach ($dll in @('libcrypto-3-x64.dll','libssl-3-x64.dll')) { Copy-Item -LiteralPath "$mysqlDir\bin\$dll" -Destination $PSScriptRoot -Force }
Set-Content -LiteralPath 'build\configuration.txt' -Value $signature -Encoding ASCII
Write-Host 'Build complete.'
} finally { Pop-Location }
