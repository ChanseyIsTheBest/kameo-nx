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
- 23:43 Magenta contours traced to my own leftover: the Swap hook wrote N&B's fence tag at `0x3A44`, inside the gamma ramp. Removed; `renut11` shows 0 such pixels.
- 23:49 `emu3`: the Switch build reaches Thorn's Castle in Ryujinx on deko3d, right colours, verify 0 differ. Runaway render targets found (768 MB).
- 00:16 Surface headers read on the game thread; stream cache 256 MB. `emu5prof`: the stale-header cause proven by its own log line, targets 169 MB, gameplay a steady 12-19 fps in the emulator.
- 00:32 First hot list merged (247 functions). Malloc per `BeginVertices` and per-draw thread-local calls removed from the drawing thread. Copies building.
- 01:20 Ring census: every draw packet the game writes itself is D3D's own pipe drain; no draw of the game's lost in the path covered.
- 01:52 DLC installed on both builds; 1,245 more shaders, 5,861 in all. Deferred chain on (one device), 36.3 million draws checked, 0 differ.
- 04:01 Docked at 1080 lines reaches gameplay in the emulator.
- 05:09 `verify5`: 426 of 446 copies reached, 18.5 million calls compared, 0 differences. 110 without a compared call held out.
- 05:44 `emu14`: the NRO that ships reaches gameplay, every verify line 0 differ.
- 05:50 Time limit moved to 18:00 by the user. Tested package made.
- 06:02 The user asked for a document on making Renut accurate, shared between ports: https://claude.ai/artifact/Ek4W2j42vrHqTWTmcB3HeC. Read Ridge Racer 6, Halo 3; asked Viva Pinata, Sonic and Pac-Man.
- 06:15 Chains closed: not the renderer. The game submits fewer rings (318 vertices against 60), and our own renderer on deko3d at low frame rates is as sparse as the reference. Their spacing, and the player's height on the wall, follow the frame rate. Sky: `e20049` ruled out; the reference's window varies from (16,12,6) to (255,255,203), so a long idle run of ours (`idle_long`) tests the phase. Gamma census (`kameo_gamma_census`) written, not yet built.
