Param(
    [string]$NetPath = "examples/newnet/trained_3class_from_newnet_v4.net",
    [double]$Noise = 0.08,
    [int]$Warmup = 20,
    [int]$Window = 180,
    [double]$Alpha = 0.05,
    [double]$Threshold = 0.001,
    [int]$TimingJitter = 2,
    [int]$Seeds = 50,
    [int]$SeedStart = 1000,
    [int]$TestsPerSeed = 10
)

$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot "../../src/train/build/Release/glia_eval.exe"
if (-not (Test-Path $exe)) { throw "Could not find trainer binary at $exe. Build it first." }
if (-not (Test-Path $NetPath)) { throw "Net file not found: $NetPath" }

$logsDir = Join-Path $PSScriptRoot "logs"
New-Item -ItemType Directory -Force -Path $logsDir | Out-Null

$accs = @()
$classTotals = @(0,0,0)
$classCorrect = @(0,0,0)
for ($i = 0; $i -lt $Seeds; $i++) {
    $seed = $SeedStart + $i
    $seedTotals = @(0,0,0)
    $seedCorrect = @(0,0,0)
    for ($t = 0; $t -lt $TestsPerSeed; $t++) {
        $subseed = $seed + $t
        $metrics = Join-Path $logsDir ("validate_3class_seed_{0}_t{1}.json" -f $seed, $t)
        & $exe --scenario 3class --net $NetPath `
            --noise $Noise `
            --warmup $Warmup `
            --window $Window `
            --alpha $Alpha `
            --threshold $Threshold `
            --timing_jitter $TimingJitter `
            --seed $subseed `
            --metrics_json $metrics | Out-Null
        if (-not (Test-Path $metrics)) { Write-Warning "Missing metrics for seed $seed test $t"; continue }
        $j = Get-Content -Raw $metrics | ConvertFrom-Json
        if ($j.details) {
            for ($idx = 0; $idx -lt $j.details.Count; $idx++) {
                $expected = "O$idx"
                $winner = [string]$j.details[$idx].winner
                $seedTotals[$idx] += 1
                $classTotals[$idx] += 1
                if ($winner -eq $expected) {
                    $seedCorrect[$idx] += 1
                    $classCorrect[$idx] += 1
                }
            }
        }
    }
    $seedTotalAll = ($seedTotals | Measure-Object -Sum).Sum
    if ($seedTotalAll -gt 0) {
        $seedCorrectAll = ($seedCorrect | Measure-Object -Sum).Sum
        $seedAcc = [double]$seedCorrectAll / [double]$seedTotalAll
        $accs += $seedAcc
        $o0 = if ($seedTotals[0] -gt 0) { [double]$seedCorrect[0]/$seedTotals[0] } else { 0 }
        $o1 = if ($seedTotals[1] -gt 0) { [double]$seedCorrect[1]/$seedTotals[1] } else { 0 }
        $o2 = if ($seedTotals[2] -gt 0) { [double]$seedCorrect[2]/$seedTotals[2] } else { 0 }
        Write-Host ("Seed {0}: overall={1:P1}  per-class: O0={2:P1} O1={3:P1} O2={4:P1}" -f $seed, $seedAcc, $o0, $o1, $o2)
    } else {
        Write-Warning "Seed $seed collected zero tests."
    }
}

if ($accs.Count -eq 0) { Write-Error "No validation results collected."; exit 1 }
$mean = ($accs | Measure-Object -Average).Average
$min  = ($accs | Measure-Object -Minimum).Minimum
$max  = ($accs | Measure-Object -Maximum).Maximum
$g0 = if ($classTotals[0] -gt 0) { [double]$classCorrect[0]/$classTotals[0] } else { 0 }
$g1 = if ($classTotals[1] -gt 0) { [double]$classCorrect[1]/$classTotals[1] } else { 0 }
$g2 = if ($classTotals[2] -gt 0) { [double]$classCorrect[2]/$classTotals[2] } else { 0 }
Write-Host ("\nValidated over {0} seeds x {1} tests/seed (noise={2}, jitter={3}, window={4}, thr={5})." -f $accs.Count, $TestsPerSeed, $Noise, $TimingJitter, $Window, $Threshold)
Write-Host ("Accuracy: mean={0:P1}, min={1:P1}, max={2:P1}" -f $mean, $min, $max)
Write-Host ("Per-class accuracy: O0={0:P1} O1={1:P1} O2={2:P1}" -f $g0, $g1, $g2)
