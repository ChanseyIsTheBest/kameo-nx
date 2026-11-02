# katamari-nx, kameo-nx: a PC verify run on Renut: every CPU enhancement checks itself on every use (banjo_cpu_verify), the
# guard on each thread's first 4000 calls of every hook and then one call in -Period (docs/optimising-functions.md, "Checking").
# -Period 1 guards every call, and so does banjo_cpu_verify whatever the period, which held 446 copies under 1 fps from the movies
# on, the Bink decoder's among them (verify3 and verify4 never reached gameplay, so most copies went unexercised). A -Period above
# 1 leaves banjo_cpu_verify off, so that it counts. For a new game, move the save out of out/pc_user first (it changes the menu
# path; run_pc.ps1's --user_data_root comes first and wins over a second one).
#   powershell -File tools/run_pc_verify.ps1 [-Seconds 300] [-Name verify1] [-Every 15] [-Script "20:START,24:A"] [-Sticks 240] [-Period 64]
param([int]$Seconds = 300, [string]$Name = "verify1", [int]$Every = 15, [string]$Script = "", [int]$Sticks = 0, [int]$Period = 1)
$root = Split-Path -Parent $PSScriptRoot
$self = if ($Period -gt 1) { "false" } else { "true" }
$extra = "--banjo_cpu_verify=$self --banjo_hot_guard_calls=4000 --banjo_hot_guard_period=$Period " +
         "--update_data_root=D:/kameo/update --headless=true --input_script_polls=60 --input_script=$Script"
if ($Sticks -gt 0) { $extra += " --input_script_sticks=$Sticks" }
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root "tools\run_pc.ps1") -Seconds $Seconds -Name $Name -Every $Every -Visible -Extra $extra
