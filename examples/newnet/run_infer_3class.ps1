Param(
    [string]$NetPath = "examples/newnet/trained_3class_from_newnet_v4.net",
    [double]$Noise = 0.08,
    [int]$Runs = 3,
    [int]$Warmup = 20,
    [int]$Window = 180,
    [double]$Alpha = 0.05,
    [double]$Threshold = 0.001
)

$ErrorActionPreference = 'Stop'

$exe = Join-Path $PSScriptRoot "../../src/train/build/Release/glia_eval.exe"
if (-not (Test-Path $exe)) {
    throw "Could not find trainer binary at $exe. Build it first."
}

if (-not (Test-Path $NetPath)) {
    throw "Net file not found: $NetPath"
}

$logsDir = Join-Path $PSScriptRoot "logs"
New-Item -ItemType Directory -Force -Path $logsDir | Out-Null

$accs = @()
for ($i = 1; $i -le $Runs; $i++) {
    $metrics = Join-Path $logsDir ("infer_3class_run_{0}.json" -f $i)
    & $exe --scenario 3class --net $NetPath `
        --noise $Noise `
        --warmup $Warmup `
        --window $Window `
        --alpha $Alpha `
        --threshold $Threshold `
        --metrics_json $metrics | Out-Host
    if (-not (Test-Path $metrics)) {
        Write-Warning "Metrics file missing: $metrics"
        continue
    }
    $j = Get-Content -Raw $metrics | ConvertFrom-Json
    $accs += [double]$j.accuracy
    Write-Host ("Run {0}: accuracy={1:P1}" -f $i, $j.accuracy)
    if ($j.details) {
        foreach ($d in $j.details) {
            Write-Host ("  idx={0} winner={1} margin={2}" -f $d.index, $d.winner, $d.margin)
        }
    }
}

if ($accs.Count -gt 0) {
    $mean = ($accs | Measure-Object -Average).Average
    Write-Host ("\nMean accuracy over {0} runs: {1:P1}" -f $accs.Count, $mean)
}
