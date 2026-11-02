# kameo-nx: one PC run with RenderDoc 1.46's Vulkan layer in this process's environment only (tools/run_ryujinx_rdc.ps1's
# method), asking for a capture -CaptureAt seconds after the first game frame (banjo_renderdoc_game_s): 3 game frames, written
# to app/out/pc/capturas/rdc and moved to out/pc_runs/<name>. -Emulated runs the SDK's emulated GPU (the reference).
#   powershell -File tools/run_pc_rdc.ps1 -Seconds 150 -Name rdc1 -CaptureAt 110 -Script "45:START,55:A" [-Emulated]
param([int]$Seconds = 150, [string]$Name = "rdc", [int]$CaptureAt = 110, [string]$Script = "", [string]$Extra = "", [switch]$Emulated)
$root = Split-Path -Parent $PSScriptRoot
$rdcDir = "D:\Banjo kazooie port\tools\RenderDoc_1.46_64"
$env:VK_LAYER_PATH = $rdcDir
$env:VK_INSTANCE_LAYERS = "VK_LAYER_RENDERDOC_Capture"
$env:ENABLE_VULKAN_RENDERDOC_CAPTURE = "1"
$a = "--banjo_renderdoc_game_s=$CaptureAt --banjo_show_fps=false --update_data_root=D:/kameo/update"
if ($Script) { $a += " --input_script=$Script" }
$a += " $Extra"
if ($Emulated) {
    $a += " --banjo_renderer=emulated --gpu_plugin=xenos --banjo_noncpu_enhancements=false --gpu_backend=vulkan --vulkan_device=1 --license_mask=1"
} else {
    $a += " --banjo_renderer=renut --renut_gpu_backend=vulkan --license_mask=1 --masseffect_renderer_native=false"
}
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root "tools\run_pc.ps1") -Seconds $Seconds -Name $Name -Every 15 -Visible -Extra $a
Remove-Item Env:\VK_LAYER_PATH, Env:\VK_INSTANCE_LAYERS, Env:\ENABLE_VULKAN_RENDERDOC_CAPTURE
$rdc = Join-Path $root "app\out\pc\capturas\rdc"
if (Test-Path $rdc) { Get-ChildItem $rdc -Filter *.rdc | Move-Item -Destination (Join-Path $root "out\pc_runs\$Name") -Force }
Get-ChildItem (Join-Path $root "out\pc_runs\$Name") -Filter *.rdc | Select-Object Name, Length
