# Example: .\Scripts\BuildEditor.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
param(
    [Parameter(Mandatory = $true)]
    [string]$EngineRoot
)

$ErrorActionPreference = 'Stop'
$BuildTool = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$ProjectFile = Join-Path $PSScriptRoot '../ManOfMettle.uproject'
if (-not (Test-Path $BuildTool)) { throw "Unreal build tool not found: $BuildTool" }
$ResolvedProject = (Resolve-Path $ProjectFile).Path
Write-Host "Building ManOfMettleEditor (all three modules) with $EngineRoot"
& $BuildTool 'ManOfMettleEditor' 'Win64' 'Development' "-Project=$ResolvedProject" '-WaitMutex'
if ($LASTEXITCODE -ne 0) { throw "Editor build failed with exit code $LASTEXITCODE. Preserve the build output for fixes." }
Write-Host 'Editor target build completed. PIE and asset import checks are separate.'
