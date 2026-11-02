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
sdmc:/switch/kameo-nx/renut.toml        settings (copy one of the tomls below over it)
sdmc:/switch/kameo-nx/game_root/        the disc's files, and title update 2's default.xexp next to default.xex
                                        (extract your ISO; the ISO itself is not read)
sdmc:/switch/kameo-nx/update/           title update 2's language folders (English, French ...)
sdmc:/switch/kameo-nx/dlc/              the DLC packages (Xbox 360 LIVE files, content type 2), optional, installed at
                                        the first start (dlc/installed.txt lists them)

The tomls
---------
renut.toml        the shipped settings: Renut on deko3d, 720 lines handheld and 1080 docked, every exact enhancement on,
                  the items measured SLOWER ON THE CONSOLE in banjo-nx off, logs and diagnostics on.
renut_play.toml   the same with the render-thread diagnostics off (for playing).
renut_ab.toml     the same with an in-run A/B list, 30 s halves, target 1 as the do-nothing control (a Vulkan-only key).
                  Stand still in one place for 10+ minutes; the log's "[renut A/B]" lines give each target.
configs/          test configs and options that change the picture, one key away from renut.toml each, for you to decide on.

Cores
-----
The port runs on the three cores a homebrew launch gets. Four cores are optional and off: with
banjo_forwarder_program_id in renut.toml set to your forwarder's id, the NRO writes that forwarder's four-core NPDM
(NetherSX2_nx's layout, CPU 0-3) to sdmc:/atmosphere/contents/<id>/exefs/main.npdm, and the next launch has CPU 0-3
(the log's "[banjo] system core:" line says whether the fourth core was granted). Delete that main.npdm to go back to
three cores.

Console profile
---------------
The ELF (kameo-nx.elf) and the source snapshot (source_<commit>.tgz) are the ones this NRO was built from. A console
profile (logs/rex/rex_profile.log, switch_profile_stacks = true, on in renut.toml) maps to source lines with them; send
that file, the log and the build's name.
