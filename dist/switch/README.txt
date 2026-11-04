kameo-nx: Kameo: Elements of Power (Xbox 360, title 4D5307D2, title update 2) on the Nintendo Switch
======================================================================================================

A ReXGlue static recompilation, built from katamari-nx and banjo-nx: their SDK fork, their post-codegen CPU passes and
reNut's Direct3D-level renderer on plume's deko3d back end (no NVK, no PM4 ring, no EDRAM model). Your own game files are
needed; nothing of the game is in this package except the recompiled code and the compiled shader cache.

Credits: reNut's renderer by masterspike52 (its author permits its reuse in these ports); Kameo: RePowered by MaxDeadBear
and BiRabittoh (MIT: its code generation findings and the parts credited in docs/kameo-notes.md); katamari-nx, banjo-nx,
sa2-nx (input, CPU items), NetherSX2_nx by NaGaa95 (the four-core layout, MIT), battd_nx (the GPU arena), bejeweled3-nx and
the zuma ports (the hot-function method, the version 0x0E shader containers). GPL-3.0 where the sources are.

SD card layout
--------------
sdmc:/switch/kameo-nx/kameo-nx.nro      the program
sdmc:/switch/kameo-nx/kameo.toml        settings, the only toml the NRO reads (copy one of the tomls below over it)
sdmc:/switch/kameo-nx/game_root/        the disc's files, and title update 2's default.xexp next to default.xex
                                        (extract your ISO; the ISO itself is not read)
sdmc:/switch/kameo-nx/update/           title update 2's language folders (English, French ...)
sdmc:/switch/kameo-nx/dlc/              the DLC packages (Xbox 360 LIVE files, content type 2), optional, installed at
                                        the first start (dlc/installed.txt lists them)

The tomls
---------
The NRO has its settings built in and reads one file over them, key by key: kameo.toml next to it. A file named renut.toml
on the card does nothing. This package's kameo.toml holds only the player's options: language, anti-aliasing, resolution,
frame rate, four-core mode, logs and controls. Delete it and the NRO writes the same file again. Only crash logs are written
until you remove the # in front of log_level.

In configs/, each to be copied over kameo.toml:
renut.toml                 the same file as kameo.toml.
renut_play.toml            the same with the log on, for a bug report.
renut_profile.toml         the console profile: the log, diagnostics and the CPU profiler, at a fixed resolution with no frame cap.
renut_ab.toml              an in-run A/B list, 30 s halves, target 1 as the do-nothing control (a Vulkan-only key), logs and
                           diagnostics on. Stand still in one place for 10+ minutes; the log's "[renut A/B]" lines give each target.
renut_no_dynamic.toml      a fixed 3D height: 720 handheld, 900 docked.
renut_smaa.toml            SMAA on. It changes the picture.
renut_648_864.toml, renut_720_docked.toml, renut_verify.toml, renut_pgo.toml, renut_nan_flush_off.toml, renut_no_gpu_round.toml,
renut_audit_off.toml       test configs.

Cores
-----
A launch from the homebrew menu gets three cores, and the game runs on them. The fourth core is on by default, but only a
forwarder can be granted it: in Sphaira, press X on the NRO, Install Forwarder, Address Space 39-bit, Create Forwarder. Start
that forwarder twice: the first start writes its four-core NPDM (NetherSX2_nx's layout, CPU 0-3) to
sdmc:/atmosphere/contents/<id>/exefs/main.npdm, and the second has CPU 0-3. For sdmc:/switch/kameo-nx/kameo-nx.nro the id
is 05E750DF55751000. The log's "[banjo] system core:" line says whether the fourth core was granted, and rex_profile.log's
"system core 3: idle N%" how much of it is left.

On the fourth core: only the Audio Worker and the shader compiler, the compiler held to 10 % of it. Everything else stays on
the other three. Delete that main.npdm to go back to three cores.

Cheat codes (from Kameo: RePowered)

On any pause or options page, eight presses. A sound plays when it works, and an Extras line then appears at the end of Game Options for good.
  Level Select             LB RB LB RB X Y X Y
  Unlock All Extras        X X Y Y LB LB RB RB
  All Warriors & Upgrades  Y X Y X RB LB RB LB
  Collectibles (tracker)   X Y LB RB X Y LB RB
The two cheats are toggles on the Extras page. What you unlock and turn on is kept in kameo_codes.txt next to the NRO, so a new package does not lose it.

Console profile
---------------
The ELF (kameo-nx.elf) and the source snapshot (source_<commit>.tgz) are the ones this NRO was built from. A console
profile (logs/rex/rex_profile.log, from configs/renut_profile.toml) maps to source lines with them; send
that file, the log and the build's name.
