# kameo-nx: what else the machine is doing during a test run, every 30 s, so a slow run can be read (the emulator's frame rate
# means little when other ports are building): CPU load, free commit memory, Ryujinx and compiler processes.
#   powershell -File tools/load_log.ps1 -Seconds 620 -Out out/emu10_load.txt
param([int]$Seconds = 600, [string]$Out = "out/load.txt")
$end = (Get-Date).AddSeconds($Seconds)
Set-Content -Path $Out -Value "time cpu% freeCommitGB ryujinx compilers renut" -Encoding ascii
while ((Get-Date) -lt $end) {
    $cpu = (Get-CimInstance Win32_Processor | Measure-Object LoadPercentage -Average).Average
    $free = [math]::Round((Get-CimInstance Win32_OperatingSystem).FreeVirtualMemory / 1MB, 1)
    $ryu = @(Get-Process -Name Ryujinx -ErrorAction SilentlyContinue).Count
    $cc = @(Get-Process -Name cc1plus, clang, clang++, ld, lld, ninja -ErrorAction SilentlyContinue).Count
    $renut = @(Get-Process -Name renut -ErrorAction SilentlyContinue).Count
    Add-Content -Path $Out -Value ("{0} {1} {2} {3} {4} {5}" -f (Get-Date -Format HH:mm:ss), $cpu, $free, $ryu, $cc, $renut) -Encoding ascii
    Start-Sleep -Seconds 30
}
