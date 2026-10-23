$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $projectRoot '.tools/platformio/packages/toolchain-gccmingw32/bin/g++.exe'
$env:PATH = (Split-Path -Parent $compiler) + ';' + $env:PATH
if (!(Test-Path -LiteralPath $compiler)) {
  throw 'Install compiler: .\tools\pio.ps1 pkg install --global --tool platformio/toolchain-gccmingw32'
}
New-Item -ItemType Directory -Path (Join-Path $projectRoot 'build') -Force | Out-Null
$testExe = Join-Path $projectRoot 'build/controller_test.exe'
& $compiler -std=c++11 -Wall -Wextra -Werror -static (Join-Path $projectRoot 'tests/controller_test.cpp') -o $testExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $testExe
exit $LASTEXITCODE
