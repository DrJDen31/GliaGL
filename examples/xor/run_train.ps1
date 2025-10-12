Param()
$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootWin = Resolve-Path (Join-Path $ScriptDir '..\..')
$RootWsl = & wsl wslpath -a "$RootWin"
& wsl bash -lc "bash '$RootWsl/examples/xor/run_train_wsl.sh'"
