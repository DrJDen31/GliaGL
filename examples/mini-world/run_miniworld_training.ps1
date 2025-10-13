Param(
    [string]$DataRoot = "examples/mini-world/data/run1",
    [string]$NetPath = "examples/mini-world/mini_world_newnet.net",
    [string]$BuildDir = "src/train/build",
    [double]$TrainFraction = 0.8,
    [int]$Epochs = 10,
    [int]$Batch = 4,
    [double]$LR = 0.01,
    [double]$Lambda = 0.95,
    [double]$WeightDecay = 0.0001,
    [int]$Warmup = 20,
    [int]$Window = 80,
    [double]$Alpha = 0.05,
    [double]$Threshold = 0.01,
    [string]$DefaultId = "O0",
    [int]$Seed = 123456
)

$ErrorActionPreference = 'Stop'

function Ensure-Dir {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path)) { New-Item -ItemType Directory -Force -Path $Path | Out-Null }
}

$ExeRelease = Join-Path $BuildDir 'Release/glia_miniworld.exe'
$ExeDebug   = Join-Path $BuildDir 'Debug/glia_miniworld.exe'
if (Test-Path -LiteralPath $ExeRelease) { $Exe = $ExeRelease }
elseif (Test-Path -LiteralPath $ExeDebug) { $Exe = $ExeDebug }
else {
    Write-Error "Could not find glia_miniworld.exe in '$BuildDir/Release' or '$BuildDir/Debug'. Build the target 'glia_miniworld' first."
}

$CfgDir = 'examples/mini-world/configs'
$LogDir = 'examples/mini-world/logs'
Ensure-Dir $CfgDir
Ensure-Dir $LogDir

$ConfigPath = Join-Path $CfgDir 'train_miniworld.json'
$SaveNet    = Join-Path $LogDir 'trained_miniworld.net'
$MetricsOut = Join-Path $LogDir 'train_metrics.json'

$cfg = [ordered]@{
    net_path = $NetPath
    data_root = $DataRoot
    train_fraction = [double]$TrainFraction
    max_class = 4
    warmup = [int]$Warmup
    window = [int]$Window
    detector = @{ alpha = [double]$Alpha; threshold = [double]$Threshold; default_id = $DefaultId }
    train = $true
    epochs = [int]$Epochs
    batch = [int]$Batch
    shuffle = $true
    seed = [int]$Seed
    lr = [double]$LR
    lambda = [double]$Lambda
    weight_decay = [double]$WeightDecay
    margin = 0.05
    reward_mode = 'softplus_margin'
    update_gating = 'none'
    reward_gain = 1.0
    reward_min = -1.0
    reward_max = 1.0
    reward_pos = 1.2
    reward_neg = -0.8
    r_target = 0.05
    rate_alpha = 0.05
    elig_post_use_rate = $true
    no_update_if_satisfied = $true
    use_advantage_baseline = $true
    baseline_beta = 0.1
    weight_clip = 0.0
    save_net = $SaveNet
    train_metrics_json = $MetricsOut
}

$cfg | ConvertTo-Json -Depth 4 | Out-File -FilePath $ConfigPath -Encoding UTF8 -Force

Write-Host "Running: $Exe --config $ConfigPath" -ForegroundColor Cyan
& $Exe --config $ConfigPath

