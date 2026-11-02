# Progress log

All times AEST, 2026-11-02 / 03.

- 20:06 Start. Docs read; `kameo-nx` created from katamari-nx-public's files.
- 20:10 Disc extracted to `D:\kameo\game` (5.4 GB); update unpacked (`default.xexp`, nine string tables to `D:\kameo\update`).
- 20:15 Katamari's parts taken out, as PAC-MAN's first commit did.
- 20:20 First codegen on the patched image: RePowered's hints checked, one wrong (`0x8251F320`), dropped. Pointer and gap sweeps, label check, wide scan, jump tables, register diet audits: clean.
- 20:25 First PC build started (emulated GPU, Renut compiled out).
- 21:27 The PC build: emulated GPU reaches the title (first stop: C000008F, an FP exception on a host thread; RePowered's guard).
- 21:43 Renut on Vulkan draws the title: the game's device at XDK 2.0.2135's layout, 4,616 shaders from the executable and the CAFF archives.
- 22:17 Renut on Vulkan reaches Thorn's Castle gameplay, stable for 300 s (index endian, vertex fetch, resolve homes).
- 22:34 Switch shaders: 4,616 compiled by uam, 0 failed. Package 4.9 MB. Ryujinx data folder made from katamari's system files.
- 22:36 Kameo's settings, package script, README and notices done; title update code built in. First Switch build started.
- 22:40 Reference reaches gameplay (`emu_gpu7`, longer button holds). First differences: midtones darker (the game's gamma ramp), sky blown out through the window.
- 22:55 First NRO, 46 MB, deko3d only. 22:56 first Ryujinx run: title and menus draw, no crash, verify 0 differ, 100 % fast draws. False colours from my test toml (gamma on, old offset in that binary).
- 22:58 The user's report through the Pac-Man session: `vpkuwus` / `vpkuhus` aliasing; time limit moved to 11:00. Fixed in the SDK; Kameo uses neither. `vpkd3d128` has the same bug and Kameo does hit it at `0x82551A34` (float-to-half helper loses the sign). Fixed too.
- 23:03 PC rebuild with new codegen: pack fixes, render guard, gamma at `+0x36EC`, Game precision by default.
