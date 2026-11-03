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
on the card does nothing. Without a kameo.toml the NRO writes a short one with the player's options and runs on the built-in
settings: the same picture settings, with no log, no profiler and no diagnostics. This package's kameo.toml is renut.toml below.

In configs/, each to be copied over kameo.toml:
renut.toml        the shipped settings: Renut on deko3d, 720 lines handheld and 1080 docked, every exact enhancement on,
                  the items measured SLOWER ON THE CONSOLE in banjo-nx off, logs and diagnostics on.
renut_play.toml   the same with the render-thread diagnostics off (for playing).
renut_ab.toml     the same with an in-run A/B list, 30 s halves, target 1 as the do-nothing control (a Vulkan-only key).
                  Stand still in one place for 10+ minutes; the log's "[renut A/B]" lines give each target.
The others        test configs and options that change the picture, one key away from renut.toml each, for you to decide on.

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

Console profile
---------------
The ELF (kameo-nx.elf) and the source snapshot (source_<commit>.tgz) are the ones this NRO was built from. A console
profile (logs/rex/rex_profile.log, switch_profile_stacks = true, on in configs/renut.toml) maps to source lines with them; send
that file, the log and the build's name.
