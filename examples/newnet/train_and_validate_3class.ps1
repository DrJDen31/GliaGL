Param(
    [string]$ConfigPath = "examples/newnet/configs/train_newnet_3class.json",
    [string]$NetOut = "examples/newnet/trained_3class_from_newnet_v5.net",
    [string]$TrainMetricsOut = "examples/newnet/train_metrics_v5.json",
    [switch]$SuppressFinalEval = $true,
    [int]$ValSeeds = 10,
    [int]$ValSeedStart = 91000,
    [int]$ValTestsPerSeed = 5,
    [double]$Noise = 0.08,
    [int]$Warmup = 20,
    [int]$Window = 180,
    [double]$Alpha = 0.05,
    [double]$Threshold = 0.001,
    [int]$TimingJitter = 2
)

$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot "../../src/train/build/Release/glia_eval.exe"
if (-not (Test-Path $exe)) { throw "Could not find trainer binary at $exe. Build it first." }
if (-not (Test-Path $ConfigPath)) { throw "Config file not found: $ConfigPath" }

# Train
Write-Host "=== Training (config: $ConfigPath) ==="
$trainArgs = @('--config', $ConfigPath, '--save_net', $NetOut, '--train_metrics_json', $TrainMetricsOut)
if ($SuppressFinalEval) {
    $stop = $false
    & $exe @trainArgs 2>&1 | ForEach-Object {
        $line = $_.ToString()
        if ($line -match '^=== Evaluating ') { $stop = $true; return }
        if (-not $stop) { Write-Host $line }
    }
} else {
    & $exe @trainArgs
}

if (-not (Test-Path $NetOut)) { throw "Training did not produce net file: $NetOut" }

# Small validation (matching training settings)
Write-Host "`n=== Validating trained net (small sweep) ==="
$valScript = Join-Path $PSScriptRoot "run_validate_3class.ps1"
if (-not (Test-Path $valScript)) { throw "Missing validation script: $valScript" }

& $valScript -NetPath $NetOut -Noise $Noise -Warmup $Warmup -Window $Window -Alpha $Alpha -Threshold $Threshold -TimingJitter $TimingJitter -Seeds $ValSeeds -SeedStart $ValSeedStart -TestsPerSeed $ValTestsPerSeed

