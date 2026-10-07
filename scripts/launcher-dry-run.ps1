param(
    [string]$BuildDir = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot '..')).Path 'build')
)

$testExecutable = Join-Path (Resolve-Path $BuildDir).Path 'qemu-launcher-core-tests.exe'
if (-not (Test-Path -LiteralPath $testExecutable)) {
    throw "Core test executable not found: $testExecutable. Run cmake --build first."
}

$output = & $testExecutable 2>&1
$exitCode = $LASTEXITCODE
$output | Write-Output
if ($exitCode -ne 0) {
    exit $exitCode
}
if (($output -join "`n") -match 'GPU_EXECUTION_COMPLETE|GPU execution complete') {
    throw 'Dry-run must not claim GPU execution without guest and GPU Engine evidence.'
}
Write-Output 'DRY_RUN: core checks passed; GPU execution still requires real evidence.'
