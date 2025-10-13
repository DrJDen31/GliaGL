Param()
$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootWin = Resolve-Path (Join-Path $ScriptDir '..\..')
$RootWsl = & wsl wslpath -a "$RootWin"
& wsl bash -lc "bash '$RootWsl/examples/3class/run_from_biased_baseline_wsl.sh'"

