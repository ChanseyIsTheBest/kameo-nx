# katamari-nx, kameo-nx: the Switch package (the request's step 7), as banjo-nx's builds/ folders:
#   builds/<date>_<time>_<name>/
#     COPY_TO_SD_switch_kameo-nx/   kameo-nx.nro, kameo.toml (the shipped renut.toml), configs/ (renut.toml, renut_play.toml,
#                                      renut_ab.toml and the rest), deko3d_shaders.bin (the prebuilt DKSH cache),
#                                      game_root/ (your game files)
#     COPY_TO_SD_ROOT/atmosphere/contents/05D6EA1333487100/exefs/main.npdm   (the four-core forwarder NPDM; only with
#                                      -FourCore since rc6: the user runs on three cores)
#     kameo-nx.elf                  the unstripped ELF of this NRO (profiles map to it)
#     source_<commit>.tgz              the source snapshot (git archive of the commit; no game data, no generated code)
#     README.txt, PACKAGE_NOTES.txt
# Nothing of the game's data goes in by default (the NRO holds the recompiled code; the DKSH cache is compiled shader
# code). katamari-nx (2026-10-31, the user's request, for their own SD card): -GameRoot copies the extracted disc into
# game_root and -Dlc the DLC packages into dlc (installed at the first start). Never into git, never uploaded.
# kameo-nx (2026-11-03, the user's request): -Ready names the folder switch\kameo-nx, to copy to the card's root as it is, and
# makes the game's files hard links (no disk space here; Explorer copies them as ordinary files). -Caches copies the DKSH
# cache and the pipeline list from a folder, normally the emulator's copy of this NRO's.
#   powershell -File tools/package_switch.ps1 -Name pgo_lto [-Build app\out\switch-pgo-use] [-Notes <file>]
#       [-GameRoot D:\kameo\game] [-Update D:\kameo\update] [-Dlc <folder>] [-Caches <folder>] [-Ready]
param([string]$Name = "package", [string]$Build = "app\out\switch", [string]$Notes = "", [string]$GameRoot = "",
      [string]$Dlc = "", [string]$Update = "", [string]$Caches = "", [switch]$FourCore, [switch]$Ready)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$free = (Get-PSDrive D).Free / 1GB
if ($free -lt 2) { throw ("D: has {0:N1} GB free: not packaging (the request: stop and say so)" -f $free) }
$b = Join-Path $root $Build
$nro = Join-Path $b "renut.nro"
$elf = Join-Path $b "renut"
if (-not (Test-Path $nro)) { throw "no NRO at $nro" }
$stamp = Get-Date -Format "yyyy-MM-dd_HHmm"
$out = Join-Path (Split-Path -Parent $root) "builds\${stamp}_$Name"
$sd = Join-Path $out $(if ($Ready) { "switch\kameo-nx" } else { "COPY_TO_SD_switch_kameo-nx" })
# A tree of the game's data, without the emulator's DLC list.
function Copy-Data([string]$From, [string]$To) {
    if ($Ready) {
        $from_full = (Resolve-Path $From).Path.TrimEnd('\')
        Get-ChildItem $from_full -Recurse -File | Where-Object { $_.Name -ne "installed.txt" } | ForEach-Object {
            $dest = Join-Path $To $_.FullName.Substring($from_full.Length + 1)
            New-Item -ItemType Directory -Force (Split-Path -Parent $dest) | Out-Null
            New-Item -ItemType HardLink -Path $dest -Target $_.FullName | Out-Null
        }
    } else {
        & robocopy $From $To /E /XF installed.txt /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) { throw "robocopy of $From failed ($LASTEXITCODE)" }
    }
}
New-Item -ItemType Directory -Force $sd, (Join-Path $sd "configs"), (Join-Path $sd "game_root") | Out-Null
Copy-Item $nro (Join-Path $sd "kameo-nx.nro")
$dist = Join-Path $root "dist\switch\kameo-nx"
# The NRO reads kameo.toml over its built-in settings (src/renut_app.h OnConfigurePaths), never renut.toml.
Copy-Item (Join-Path $dist "renut.toml") (Join-Path $sd "kameo.toml")
Copy-Item (Join-Path $dist "renut.toml"), (Join-Path $dist "renut_play.toml"), (Join-Path $dist "renut_ab.toml") (Join-Path $sd "configs")
Copy-Item (Join-Path $dist "configs\*.toml") (Join-Path $sd "configs")
$cache = Join-Path $root "out\deko3d_shaders\deko3d_shaders.bin"
if (Test-Path $cache) { Copy-Item $cache, ([IO.Path]::ChangeExtension($cache, ".idx")) $sd }
# kameo-nx (2026-11-03): the pipeline list from a console or emulator folder; its DKSH cache only when no prebuilt one exists.
if ($Caches) {
    Copy-Item (Join-Path $Caches "pipeline_list.bin") $sd
    if (-not (Test-Path $cache)) { Copy-Item (Join-Path $Caches "deko3d_shaders.bin") $sd }
}
if ($GameRoot) {
    $need = ((Get-ChildItem $GameRoot -Recurse -File | Measure-Object -Sum Length).Sum + 1GB) / 1GB
    if ((Get-PSDrive D).Free / 1GB -lt $need) { throw ("D: has too little space for the game files ({0:N1} GB needed)" -f $need) }
    Copy-Data $GameRoot (Join-Path $sd "game_root")
} else {
    Set-Content -Path (Join-Path $sd "game_root\PUT_YOUR_GAME_FILES_HERE.txt") -Encoding ascii -Value `
        "Extract your own Kameo: Elements of Power disc here: default.xex, title update 2's default.xexp next to it, and the rest (README.txt)."
}
New-Item -ItemType Directory -Force (Join-Path $sd "update") | Out-Null
if ($Update) {
    Copy-Data $Update (Join-Path $sd "update")
} else {
    Set-Content -Path (Join-Path $sd "update\PUT_THE_UPDATE_FOLDERS_HERE.txt") -Encoding ascii -Value `
        "Title update 2's language folders here (English, French ...; README.txt)."
}
New-Item -ItemType Directory -Force (Join-Path $sd "dlc") | Out-Null
if ($Dlc) {
    Copy-Data $Dlc (Join-Path $sd "dlc")
} else {
    Set-Content -Path (Join-Path $sd "dlc\PUT_THE_DLC_PACKAGES_HERE.txt") -Encoding ascii -Value `
        "Optional: Kameo's DLC packages (Xbox 360 LIVE files, content type 2) here; installed at the first start."
}
if ($FourCore) {
    $npdm = Join-Path $out "COPY_TO_SD_ROOT\atmosphere\contents\05D6EA1333487100\exefs"
    New-Item -ItemType Directory -Force $npdm | Out-Null
    Copy-Item (Join-Path $root "tools\forwarder\kameo_main.npdm") (Join-Path $npdm "main.npdm")
}
if (Test-Path $elf) { Copy-Item $elf (Join-Path $out "kameo-nx.elf") }
Copy-Item (Join-Path $root "dist\switch\README.txt") $out
if ($Notes) { Copy-Item $Notes (Join-Path $out "PACKAGE_NOTES.txt") }
Push-Location $root
try {
    $sha = (& git rev-parse --short=12 HEAD).Trim()
    & git archive --format=tar.gz -o (Join-Path $out "source_$sha.tgz") HEAD
} finally { Pop-Location }
$md5 = (Get-FileHash -Algorithm MD5 (Join-Path $sd "kameo-nx.nro")).Hash.ToLower()
Write-Output "package: $out"
Write-Output ("NRO {0:N1} MB, md5 {1}; commit {2}" -f ((Get-Item (Join-Path $sd "kameo-nx.nro")).Length / 1MB), $md5, $sha)
Get-ChildItem -Recurse $out | Where-Object { -not $_.PSIsContainer } | Select-Object FullName, Length | Format-Table -AutoSize
