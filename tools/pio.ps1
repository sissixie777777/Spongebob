$projectRoot = Split-Path -Parent $PSScriptRoot
$env:PYTHONPATH = Join-Path $projectRoot '.tools/python'
$env:PLATFORMIO_CORE_DIR = Join-Path $projectRoot '.tools/platformio'
python -m platformio @args
exit $LASTEXITCODE
