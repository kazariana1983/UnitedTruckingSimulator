param(
    [Parameter(Mandatory = $true)]
    [string]$UnrealRoot
)

$ErrorActionPreference = 'Stop'
$simulatorRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $simulatorRoot 'UnitedTruckingSimulator.uproject'
$buildTool = Join-Path $UnrealRoot 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $buildTool -PathType Leaf)) {
    throw "Build.bat not found under UnrealRoot: $UnrealRoot"
}
if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
    throw "Project file not found: $projectFile"
}

# Builds the editor target only. This does not cook, package, or launch the app.
& $buildTool 'UnitedTruckingSimulatorEditor' 'Win64' 'Development' "-Project=$projectFile" '-WaitMutex'
if ($LASTEXITCODE -ne 0) {
    throw "Unreal editor build failed with exit code $LASTEXITCODE."
}
Write-Host 'Editor target built. Open the .uproject and follow docs/UNREAL_SESSION_SHELL_TEST.md.'
