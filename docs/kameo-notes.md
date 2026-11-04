# Kameo: Elements of Power port: notes

This file records the port: what each step found, with its evidence. The project starts from a copy of katamari-nx
(`D:\beautifulkatamari\katamari-nx-public`, commit `f6c0efcd`, its files and not its history), itself from banjo-nx
(`D:\Banjo kazooie port\banjo-nx`). Kameo: RePowered (MaxDeadBear, BiRabittoh; MIT; `D:\kameo\KameoRePowered-native-rendering`,
read-only) is the source of the codegen work and of the visual grafts. Comments that say "banjo-nx", "katamari-nx",
"zuma-nx", "pacman-nx" or "RePowered" record findings from those projects. Where docs differ, katamari-nx's win, then banjo-nx's.

The request (2026-11-02, from 20:06 AEST): port Kameo to the PC and the Switch with ReXGlue, the Switch the main goal; work
uninterrupted until 06:00 AEST without asking; a package for the console by then.

## Decisions (made without asking, as the request said)

| What | Decision | Why |
|---|---|---|
| Game files | `D:\kameo\game` (the disc, `tools/extract_iso.py`), `default.xexp` next to `default.xex`, `D:\kameo\update` (the update's string tables) | the request |
| `app/assets` | a junction to `D:\kameo\game` | no second copy; git-ignored |
| Internal project name | `renut`, as banjo-nx and katamari-nx | its CMake, post-codegen passes and glue name `renut_*`; the NRO, its title and the SD folder are kameo-nx |
| Bootstrap generator | katamari-nx's built `rexglue.exe` and `rexruntime.dll` copied to `out/` for the first codegen runs | the build needs a generated tree first; the build then uses the generator it builds from `sdk/` |
| What Katamari owned | taken out as PAC-MAN's first commit did (`331c5ee1`), applied by script; `title_update.cpp` kept and brought back in | "Stale mentions" in `docs/optimising-functions.md` |
| Hand-written natives | none, the header holds only the common code | the request: only from a console profile |
| RePowered's CRT table | checked and recorded, not switched on (see "Code generation") | the base keeps the guest's CRT so every store goes through the store cards; a host `memcpy` would leave Renut's whole-buffer copies stale |
| Where the D3D state lives | **the game's own device, in the game's layout**; the game's own setters run (see "Renut on an inlined D3D") | Kameo links `D3D9LTCG`: most setters are compiled into the game, so hooks cannot be the only way state reaches the device |

## The game (2026-11-02)

- `default.xex`: title **4D5307D2**, version 0.0.0.1, base `0x82000000`, base image `0xBF0000`, entry `0x8251F320`; AES + LZX.
- Base XDK: every static library **2.0.2099** (D3D9**LTCG**, XAUDLTCG, XACTLTCG, X3DAUDL, D3DX9, XGRAPHC, XAPILIB, XRTLLIB, XONLINE, LIBCMT, XBOXKRNL, XHV, XMP),
  kernel imports 2.0.2099 min 2.0.1861.
- Title update 2 (`TU_16L61UI_...`, content type 000B0000, "Kameo Title Update #2"): `default.xexp` 1.5 MB and one
  `titleUpdatekameoStringsstripped.str` per language. Patched version **0.0.2.1**, image `0xC90000` (`82000000+C90000`).
  D3D's build qword at `0x827AB840` is `0x0002000008574000`: **XDK 2.0.2135**, the same D3D as Zuma's (zuma-nx), and the
  oldest in the "XDK versions of our Xbox 360 ports" doc (which listed Zuma's 2.0.2135 as its oldest; the base game's 2.0.2099
  is older still).
- Data, 5.4 GB: 59 `.lvl`, 13 `.mdl`, `audio/*.xwb`, `fmv/*.bik` (Bink, not WMV), one folder per language with its own `.lvl`, `.xwb` and `.plf`, `spa.bin`.
- DLC: four LIVE packages of type 2, 85 MB; the Winter Warrior folder's second copy of the Add-On Pack is the same file name, one is used.

## Time limit

A message from the Pac-Man port's session, quoting the user (2026-11-02, 22:58), moved the limit from 06:00 to 11:00 AEST. The user then moved it to 18:00 AEST on 2026-11-03 directly. A tested console package still goes out around 06:00 as first asked; the extra hours go on what is left after it.

## Code generation (2026-11-02)

**`vpkuhus` / `vpkuwus` aliasing (the user's report via the Pac-Man session, 2026-11-02).** `build_vpkuhus` and `build_vpkuwus` (`sdk/src/codegen/builders/vector.cpp`) wrote their result piece by piece into the destination, so `vpkuwus128 v63,v63,v62` overwrote source values before reading them. Both now build in `v_temp` and copy at the end. Kameo uses neither: a scan of the patched image's code finds 0 of `vpkuhus`, `vpkuwus` and their 128 forms (the only pack instructions of those groups are 2 `vpkswss`), and the generated code agrees (2 `packs_epi32`, nothing else).

**The same bug in `vpkd3d128`, which Kameo does hit.** The other piece-by-piece builders were read. `vsrb`, `vsrab`, `vrlw`, `vsraw`, `vsrw`, `vsubsws` write only the element they read, and `vupkd3d128`'s in-place cases read each source element before the piece that would overwrite it. `vpkd3d128`'s FLOAT16_2, NORMSHORT4 and FLOAT16_4 cases do not: they write pieces of vD while still reading vB. The image has 3 `vpkd3d128`, and one has `vD == vB`: `0x82551A34`, `vpkd3d128 v0,v0` (FLOAT16_4, mask 2, shift 2) in the float-to-half stream helper `sub_825519F8`, called by its wrapper at `0x82551C0C`. It splats the float, packs, then `vsplth` takes guest halfword 0, which is host `u16[7]`: the piece whose sign bit the old code read back from the half it had just written, so **every negative value converted through it came out positive**. Those cases now read a scoped copy of the source when `vD == vB`. Both the reference and Renut ran the old code, so this is no difference between them; what in the game uses the helper has not been traced, and no visible change has been seen yet.


Config: `app/renut_config.toml`, `app/config/renut_funcs.toml`, `app/overrides.toml`. The generator applies `default.xexp`
itself ("XEX patch applied successfully: base version: 0.0.0.0, new version: 0.0.2.1").

**What came from RePowered's `config/tu/`, each checked against the patched image (`tools/ppc_dis.py`, `tools/check_func_hints.py`):**

| Item | Address | Check |
|---|---|---|
| setjmp | `0x82704060` | hook word check at `0x82BCFF6C`, then `stfd f14,0(r3)` ... as katamari-nx's: right |
| longjmp | `0x82703F40` | right (reads the buffer through r3 / r4, unwinding checks first) |
| memcpy | `0x8255D400` | `std r3,-8(r1)` first: right |
| memmove | `0x8255BDA0` | `cmpw r3,r4`, forward case branches to memcpy: right |
| memset | `0x8255D970` | `stb` loop: right |
| memcpy_s, memmove_s | `0x8255FA08`, `0x8255F940` | prologues, right shape |
| XMemCpy | `0x8255D820` | byte loop head: right |
| Render guard | `0x8228AF0C` | `lwzx r8,r9,r26; mtctr r8; bctrl`, the effects table dispatch: right; its bare-`blr` target must be the patched image's, not their `0x823BB178` |
| 14 function starts | `config/renut_funcs.toml` | each after padding, each referenced (bl / b / data / lis + addi) |
| **`0x8251F320`** | **dropped** | on their list as "indirect-call target (present in vanilla; missed in the patched image)". It is the base game's entry point. In the patched image the word before runs into it (`lfs f0,-20604(r10)`), nothing references it, and `tools/read_before_write.py` says it reads six registers it never sets: a label in the middle of a function |

The CRT table is recorded here and not active: `[rexcrt]` strips the originals for host routines, which do not mark the store cards Renut's whole-buffer copies rely on; hand natives wait for a console profile.

**The base's passes on top:**

- First run with their list: 217 files, 108 MB, no "target not in any function", no unresolved branch.
- **Pointer sweep** (`tools/pointer_fixpoint.py`): 7,990 gaps, 46 candidates, 39 accepted; fixpoint in pass 2.
- **Gap sweep** (`tools/gap_sweep.py`): 7,972 gaps, 480 candidates, 20 accepted; fixpoint in pass 2.
- **Five of those are not functions** and are removed (`tools/override_labels_check.py`): `0x82558E24` (SEH scope label, `mr r8,r8`), `0x82553A1C`, `0x8256C7E8`, `0x825702A4`, `0x8260A458` (an instruction runs into each). 54 entries remain.
- **Wide pointer scan** (`tools/wide_pointer_scan.py`, 15,570 functions): 3 hits, none a function: `0x823C30D4` is a jump table's first word (built at `0x823C30C0`), `0x821C0004` / `0x821C0008` are an epilogue's lines.
- **Jump tables** (`tools/switch_check.py`): 729 `bctr` (348 absolute, 293 indirect, 44 byte, 44 half), 479 switches; truncated 0, wrong targets 0, missed 0, switches at a call through a function table 0. Five with no bound in the code (`sub_821D90B8`, `sub_82562B18`, `sub_825B59C0`, `sub_82607008` twice), to look at if one stops a run. Two odd shapes `0x826DDD78`, `0x826DFBB0` (Bink, generated as indirect calls).

### Codegen options: the register diet

All on. `tools/non_argument_audit.py app/generated --noreturn=82703F40`: ENTRY 457 (the save / restore helpers kept by name, MSVC unwind funclets `addi r31,r12,-N`, the stack probe), RET 1 (`sub_8256F688` with `sub_8256F750`), HANDOVER 0. `tools/diet_keep_list.py`: 239 + 2 functions.

`tools/read_before_write.py` on code generated WITH the options: 20 functions, none a split piece: 15 with a `mflr` / save-helper prologue that save the registers they read on paths a state machine never takes (0x8264-0x826B, the image decoders); `sub_82631DE0`, `sub_82631FF0`, `sub_826A8050`, `sub_826A8620` are setjmp callers (`env.*`, handled by the generator); setjmp itself; `sub_82571744`, a data word (`0x82571770`) in front of `sub_8257174C`, never run. **No `share_registers` needed.** `--with-option`: the two left (`sub_8264C150`, `sub_8264C2C0`) read r11 on `--volatile`, in the same decoders. `non_volatile_as_local_keep = ["v64-v127"]` from the start (katamari-nx's rc3 finding).

## Renut on an inlined D3D (the plan, 2026-11-02)

The base's Renut is a native re-implementation of N&B's D3D: hooks on ~320 functions write the device. zuma-nx (the same XDK 2.0.2135) keeps that model with an adapter, because PopCap's game calls D3D's functions. **Kameo cannot**: `D3D9LTCG` compiles D3D into the game (its `BeginIndexedVertices` inside `drawTube`, packet writers inside `xfix_sky_renderStars`, `Draw_Bink_textures`, `IconPlayerDrawBars`, `_FluidPoints_coreQuadRender`), and these write the device's fields and tags directly. RePowered's renderer works by letting the game's D3D run on its real device and capturing at the draws. So:

- the device is the game's, created by the game's `Direct3D_CreateDevice`, at XDK 2135's layout (plus the update's 16 bytes from `0x34D8`);
- the game's setters stay, unhooked; Renut's draw, clear, resolve and present hooks read the device;
- `d3d.h` and `off::` move to this layout, and resource headers to XDK 2135's (common and fence 12 bytes; fetch constants at +16 in textures, +12 in vertex buffers; index address at +12; declaration count at +8, elements at +36);
- the ring is stubbed as RePowered stubs it (`KickOff`, `RingBufferDeviceAllocate`, `BlockOnPrimaryRange`), and the GPU waits are answered by the render thread.

This keeps every part of our renderer that matters here (the record path, fast draw path, pipelines by content, pushed constants, whole-buffer copies, store cards, deko3d back end). What it does not use are the setter natives and setter marks: they were how N&B's recorded buffers avoid comparing state, and this game records none.

## What the PC run needed (2026-11-02)

Each stopped or broke a run; each has its own commit.

| Symptom | Cause | Fix |
|---|---|---|
| Exit `C000008F` on a host thread | a guest thread's MXCSR has the exception masks clear; a vector computation faults on the host | `app/src/kameo_fp_guard.cpp`, RePowered's `fp_guard` (MIT, credited) |
| "Draw dropped: declaration" | the declaration pointer is the game's, Renut wants its own id | content-hashed registry on the game thread, `KameoDeclarationById` at both lookup sites |
| All shaders "missing" | almost all are in the CAFF archives, not the executable | `tools/kameo_files.py`: 147 hits in the image plus 4,477 distinct in 81 archives, 4,616 in all |
| Shards in the 3D menus | index endian read from Common bits, which this XDK does not use for it | the endian comes from the index width (16-bit: 1, 32-bit: 2) |
| Vertex data wrong | the game builds vertex fetch constants only in its own flush (`sub_820D5EA8`) | the draw hooks build them: `w0 = GpuAddress(vb+12 + offset)`, `w1 = ((vb+16 - offset) & ~0x30000000) | 0x10000000`, at `VertexFetch[17-n]` |
| Video memory exhausted, crash (3,367 resolve targets, 11.8 GB) | the game makes a temporary header per resolve, so a target was made per header; keying by page alone was not enough, because depth and colour alternate on one page | `g_resolve_homes` keyed by `desc[1] << 32 | desc[2]`: 32 targets, 58 MB |
| "New Quest" appears to hang in tests | it opens `XamShowKeyboardUI`, an ImGui dialog that the captures do not show | tests take "Start without saving" (DOWN, A, confirm A) |
| Reference run stuck at Play while the machine was busy | the scripted press held 0.2 s, and at a low frame rate no poll fell inside it | the script's `A@1.5` holds; only needed for emulated-GPU runs |

## Census: what the game decides (porting-guide-renderer.md section 2)

PC, Thorn's Castle, run `renut8`, ~40 fps, 10 s logs. Figures for the console come later.

| Item | Kameo | Consequence |
|---|---|---|
| Where draws come from | 100 % immediate, no command buffers; ~186,000 draws/s (~4,640 a frame) | the record-time items (A/B 45-50) have nothing to act on |
| Where resources come from | the game's own headers, with temporary headers per resolve | resolve homes by content (above); objects at bind time |
| How state is set | the game's own inlined setters (D3D9LTCG), on its real device | no setter marks or natives: the device is compared at the draw |
| Memexport | to be measured in the emulator's Switch run | |
| Shadow passes | 235,946 depth-only draws per 10 s (~590 a frame); map sizes to be measured | |
| Threads | to be measured in `rex_profile.log` | |
| Where shaders live | 147 in the executable, 4,477 in 81 CAFF archives; containers version 0x0E | the Switch cache carries all 4,616; 0 failed in uam |
| Tiling | no RePowered mentions; fields exist at `0x28C0` bit `0x10` and `0x32A8..`; to be confirmed | |

`fast draws` 0 % and `whole buffers` 0 on the PC are expected: `FastDrawLocked` returns at `!IsDeko3D()`. They are read on the Switch build.

## Visual differences against the reference (from 2026-11-02 22:40)

Reference: the SDK's emulated GPU (`--banjo_renderer=emulated --gpu_plugin=xenos`), run `out/pc_runs/emu_gpu7`. Ours: Renut on Vulkan, runs `out/pc_runs/renut8` and later. Captures stay in `out/` (game imagery is never committed). Idle view of Thorn's Castle: reference `captura_015_0226s.png`, ours `captura_012_0180s.png`.

| # | Difference | Cause found | State |
|---|---|---|---|
| 1 | Our picture is darker in the midtones everywhere (menus and gameplay) | the display's gamma ramp: `SetGammaRamp` (`sub_820C48B8`) stores the converted ramp at device `+0x36EC`; our Swap still read N&B's offset, and `banjo_renut_game_gamma` was off. RePowered records the same difference (`kameo_gfx_gamma`: median 30 against the reference's 55) | **fixed** (`renut10`, `emu2`): our own key, on by default, no graft |
| 2 | The sky through the castle window is blown out to white with a wide halo; in the reference it is a muted orange | the PC default kept bloom buffers at 16 bits, the console's are 8: `renut_post_process_precision` now defaults to `"Game"` | halo gone in `renut10`; the window's colour still pale grey against the reference's cream, to be checked with the same sky |
| 3 | Rings ("chains") above the player: continuous in ours, sparse in the reference. Enlarged crops, `renut11/captura_012_0180s.png` against `emu_gpu7/captura_015_0226s.png` | **Not the renderer.** The game submits fewer: the drawing call is 318 vertices in ours (`rdc4` e17471) and 60 in the reference (`rdc_ref2` e23042), same 128x128 BC2 texture. They trail the player as it descends, and their spacing follows the frame rate of that descent: deko3d in the emulator at low frame rates (`emu14/captura_027_0426s.png`, 1-8 fps in its reports) is as sparse as the reference, on our renderer. Alpha test, alpha-to-mask and mips are not involved. I did not read the code that emits them, so "one ring a frame" is inferred from the spacing | closed: frame-rate dependent game behaviour. The console's spacing will follow its own frame rate |
| 5 | **FIXED on the PC, see "Difference 5, the cause" below.** **Sky through the window: orange in the reference, grey-yellow in ours.** Pixel (12,200) of the idle view, camera at rest: ours 22 captures in `idle_ours`, (90,82,65) to (186,176,126), G/R 0.90-0.95, B/R 0.68-0.77; reference 10 captures in `emu_gpu7` and `idle_ref`, (165,116,67) to (177,109,51), G/R 0.58-0.75, B/R 0.25-0.45. Brightness drifts in both (the sky is animated) but the hue never overlaps. An earlier note here called this unproven, from reference samples taken after the camera had moved: those were not the same view | Pixel history in `rdc4`: the orange gradient **is drawn** (event 4751: 0.253, 0.132, 0.063), then the **sun** covers it (event 14360: a 4-vertex quad, a 128x128 BC3 sun disc, alpha blended, opaque at its centre), then cloud quads over that (1016x868 BC3). So the gradient is right, and the open question is the sun's place or size on screen. The idle views differ too: on the wall, reference = ours + (-3, +52) in two pairs of captures, yet at those matching places the upper band is still orange in the reference and grey in ours, (160,115,74) against (81,75,61). That alignment may be the wrong yardstick: the sun is at infinity and barely moves with camera translation, and the player may simply hang lower on the wall. Not established: the cause. A reference capture would settle it, and needs its own trigger, since `banjo_renderdoc_game_s` sits in Renut's Swap hook and the emulated GPU never calls it. Affects a small part of this view | open |
| 4 | Thin magenta contours in bright areas (G exactly 0, R about 254, B about 130; title, opening movie, a line at the edge of the gameplay sky). Both back ends. `renut10/captura_008_0120s.png`: 11,907 such pixels; `renut8`: 0; reference: 3 | **our own bug, from fixing difference 1**: the Swap hook still wrote N&B's fence tag at `0x3A44`, inside the ramp at `+0x36EC`: word 428, green entry 172, took its high half (0) and entry 173 its low half. The contour pixels' neighbours worked back to input green 172, and the log showed the ramp "changing" 1,315 times while its printed entries stayed put. Kameo's Swap (`sub_820D1860`) has no such fields, so the three writes are gone | fix built, being checked in `renut11` |

**Deko3d result (`emu15`, 07:28, verify keys): the fix works there.** The log line fires for the same `k_8` and `k_8_8` textures, and the window reaches (171,123,71), G/R 0.72, B/R 0.42, where `emu14` before it never did. 106 verify lines 0 differ, none nonzero. Copies: 335 of 336 reached, about 4.4 million calls compared, differences 0; 66 differed once and not when repeated (races, 65 in `emu14`).

**Difference 6, the cause (2026-11-03 09:05): a bug in Ryujinx's shader translator, not in the port.** `Optimizer.RemoveNode` (Ryujinx.Graphics.Shader) took an operation off its sources' use lists as soon as one of its dests lost its last use, even with other dests in use. A texture sample has four. With `banjo_gpu_output_trim` on, the alpha chain dies inside the translator, so the cube sample stayed but its coordinates were then removed as dead: it read undefined registers, so a fixed cube colour times the plate's mask. That is the mottling.
- **Proof:** the plate shader's exact cached DKSH through the emulator's own translator DLL (scratchpad harness `xlate`, no packages): trim off reads c15, c16, c17 3, 6, 3 times; trim on 0, 3, 0, with `texture(..., intBitsToFloat(temp_337)...)` of unassigned temps, as in the live capture. With the one-line fix: 3, 6, 3 and real coordinates.
- **uam and the cache are clean:** all 12 cached shaders whose cube sample goes through that slot compute the direction; every drawn cube shader's cached variants are byte-identical to fresh compiles; the two variants' instructions are identical up to the sample, only scheduling words differ. Trim stays on. A console runs the Maxwell code directly, so by this reasoning it is unaffected; no console has confirmed that.
- **Fix, with the user's permission:** the check in `RemoveNode`, in `D:\New folder\Ryubing-src\ryubing` (commit b3205d3 on its branch `kameo-nx-shader-fix`, not pushed, listed in its BANJO-NX.md), `build/Ryujinx.Graphics.Shader.dll` swapped with the old one kept as `.bak-20261103`. All port sessions told first. Tools: `tools/rdc_shader.py`, `tools/rdc_slices.py`, `tools/uam_variant.py`.
- **Confirmed in `emu24fix` (900 s, trim on):** 41 gameplay captures at 6.5-10.2 % lilac, at a lower camera (door row 719), against `emu15`'s 16-21 %; the plates look like the reference, also by the user's eye. 166 verify lines 0 differ; copies 336 of 336 reached, 8.28 million calls compared, 0 differences. `emu22fix` and `emu23fix` show nothing: load kept them from the level in time.
- **Earlier readings below that this overtakes:** I told the ports trim was the cause, and corrected that. Runs before the swap under-report nothing else that I know of, but any Ryujinx-only difference seen before it deserves a second look.

**Difference 6, as found in `emu15`: metal plates in the wall's hole are mottled purple-grey on deko3d, smooth orange-brown on the PC and in the reference.** Top right of the idle view. Not caused by the tfetch3D fix: `emu14` has it. Scratchpad `hole_sheet.png`.
- Measure: share of lilac pixels in the box (850,110)-(1090,320), median over idle captures: `emu15` 18.9 %, `emu14` 14.8 %; reference `rdc_ref2` 11.2 %, `idle_ref` 12.2 %, `emu_gpu7` 11.7 %; PC `glow2` 12.2 %, `idle_long` 12.1 %. Steady over frames, so not a moving part's reflection.
- The draw is PS `8CBB5B7DC553E60D`, VS `EAB3CF9A645CA827` (events 4687, 4691 in `rdc5`; 9683 in `rdc_emu2`), the same shader as the walls, which look right. It mixes the lighting towards a cube map by a mask of three textures.
- Ruled out: the textures. Every mip of every layer of the draw's 8 textures is pixel-identical between `rdc5` and `rdc_emu2` (`tools/rdc_slices.py`, 116 levels, 0 differ). Camera height: `rdc_ref2` is at the same height as `emu15` (door row 684 against 687) and is not mottled. The optimiser's folds in this shader read as valid, but that is reading, not a test.
- **An enhancement causes it:** `banjo_noncpu_enhancements=false` (`emu16off`) gives 8.0 % and smooth plates, though at a somewhat different camera height. Bisecting with `tools/emu_toml_with.py`. Neither shader uses bool specialization.
- **Narrowed:** `banjo_gpu_output_trim`, `_blend_skip`, `_tess_grid` and `_bool_spec` off together (`emu17b1`): 6.5-10 % against `emu15`'s 16-21 %, at the same camera height (door row 687), and the plates look like the reference. `_bool_spec` off alone (`emu18b2`): still mottled by eye, at another height (door row 716), so its 14-18 % does not compare. The level has no tessellated draws. So output trim or blend skip; `emu19b3` tests trim alone.
- **A boot freeze, unexplained (`emu19b3`, 08:09):** all output stopped 22 s in. The log ends at `[enhancements] optimised codegen`, before the next line (the instruction forms self-check), and no capture was taken. Before any draw, and the same toml key was off in `emu17b1`, which ran, so the key is an unlikely cause. The first such freeze in this port's emulator runs. The machine is under heavy load from other ports' builds and runs (the user's reminder, 2026-11-03), so host load is the likeliest cause: not treated as a port bug unless it repeats. Re-run as `emu20b3`.
- **Blend skip cannot be it:** `g_latched.blend_skip` also needs `banjo_renut_blend_skip`, false by default and in every toml here, so that key changed nothing in `emu17b1`. That leaves output trim, by elimination, unless the bool specialization result (judged by eye at another height) misled me. `emu20b3` did not settle it: at 306 s it was still on the title screen, with the machine at 84 % CPU and 0.7 GB free RAM. `emu21b3` repeats it with 660 s.
- A lead not yet followed: in `rdc_emu2` blended draws lie over the plates (`e24863`, `e24921`, SrcAlpha/One, 64x64 and 256x256 BC2), which would be affected by either key where the plate draw itself, blend off, is not.

**Difference 5, the cause (2026-11-03 07:15): fixed on the PC and on deko3d.** A shader's `tfetch3D` on a 2D fetch constant read the null texture. XenosRecomp samples `tfetch3D` through the 3D table, and Renut bound a 2D fetch only in the 2D table; the GPU samples whatever the constant describes, as the reference does. The glow is PS `75607CA74F3F7AA3`: blend DstAlpha/One, mask 7, slot 0 a 128x128 `k_8` (swizzle RRR1), colour times texel. The clouds write alpha, so this draw lights them orange near the sun. With the null texture it read alpha 0 and was discarded. PS `1E821314E16E91EA` (additive, 10 quads a frame, 128x128 `k_8_8`) was lost the same way. RenderDoc showed it plainly: our draw's texture was 1x1 where the reference's was 128x128.
- **Evidence:** window at (12,200), `glow2` (135,83,43) to (148,84,38), G/R 0.57-0.64, B/R 0.26-0.36; reference `emu_gpu7` (158,93,44), G/R 0.59, B/R 0.28. Before, 53 idle captures in `idle_long` had none. Crops: scratchpad `glow_sheet.png`, from `idle_long/captura_053_0798s.png`, `glow2/captura_019_0285s.png`, `emu_gpu7/captura_015_0226s.png`.
- **Fix:** `banjo_renut_fetch3d_on_2d`, on, A/B 72. `Fetch3DMask` (`d3d/hooks/shader.cpp`) scans the microcode for the slots read through `tfetch3D`: it finds exactly the 52 of 5,861 shaders whose GLSL declares a `sampler3D`, offline over all the dumped containers. `FillTextures` marks those fetches stacked (`SampleAsStacked`), so they take Renut's existing `tfetchStacked` path; `BindTextureLocked` gives that use its own host texture (key `va | 1`). No shader changes.
- **From RePowered (MIT):** the idea, in `src/gpu/textures/texture_upload.cpp` ("The shader decides the dimension, not the fetch constant"), and the control-flow walk of `ucode_scan.cpp`. Its method makes a one-slice 3D resource; ours reuses the stacked path because that needs no new table on deko3d. Its `scene_replay.cpp` samples a slot with no texture object as zero: Kameo has none of those (0 in the census run).
- **Cost:** two record loads and a compare a draw, and a second small host texture for each texture used this way. Not measured on the A57.

**`loop_locals.py` and switches (from Halo 3's port, 2026-11-03).** A loop region whose last jump back was a case of a switch ended mid-switch, so later cases left it without storing its locals. Both renderers share the code, so comparing them cannot show it. Halo 3's check over `app/generated`: Kameo has the shape once (`sub_825917A8`) and 0 bad jumps, so the current tree is clear. `switch_end` is now in our pass for the next codegen; not yet exercised by a codegen here.

**Tessellation (the user's question, 2026-11-03): not used by the game's data, so banjo's tessellation work is not needed for it.** RePowered's renderer has no tessellation code, but its modkit names a render command 0x2B "draw tessellated", and our `D3DDevice_DrawIndexedTessellatedVertices` hook is an unlinked N&B leftover, which would have lost such draws. A scan of all 143 `.lvl` and `.mdl` files, game and DLC, for packets by the modkit's layout found 0 candidates for 0x2B even with a relaxed rule, against 375,579 ordinary draws (0x01) and 80,424 draw-if-visible (0x30, 0x31) as the control. The three tessellation state setters have no direct callers in the generated code; each has one pointer in the image, in adjacent 12-byte slots (image offsets 0x7A989C, 0x7A98A8, 0x7A98B4), by its shape D3D's render-state table, which proves nothing either way. Limits: a static scan; game code building a tessellated draw another way is not excluded, and only the opening level and menus have run under the ring census (0 such packets). `kameo_ring_census` would report one if a later level did.

**Ring constants (Halo 3's lead, 2026-11-03).** Halo 3's game writes constants straight into the ring past the device's shadow, which Renut does not read. Its `find_pm4.py` over Kameo's generated code: 7 hits, 6 of them `lis` plus `addi ...,11520` forming data addresses (0x82BC2D00 and the like), not packet words, whose high half would be 0xC0xx; the seventh (`sub_821042D0`) is `addis r6,r11,305` plus 11520, adding 20,000,000 to a time difference. So 0 real hits. This is a static check for one way of building the word, not proof; no draw in Kameo is known to take a stale transform.

**Wrong turns on the way, for the record.** I blamed sky phase, `e20049` as a no-op (it only looked so where no cloud had written alpha), and a slot with no texture pointer (0 cases). The first fix did nothing because my scan assumed version 0x11 containers; Kameo's are 0x0E.

**Gamma census (`kameo_gamma_census`, run `census1`).** 50 million draws in the opening level: 0 to a gamma target, 0 with colour exponent bias, 0 with a Gamma-signed slot bound; only colour formats 0 and 2. No exponent-adjust line in the log. So Halo 3's and Sonic's gamma fixes have nothing to act on here, in the scenes covered.

**Difference 5, later findings (2026-11-03 06:15).** Most of the entry above is overtaken. (1) A reference capture now exists (`rdc_ref2`, `StartTimed`). Where the sun covers the pixel, gradient, sun and final colour match ours. (2) The pixels I first compared were not the same: at (5,170) the reference has a wall in front (e15710, z 0.99915) and the sun fails its depth test there. The `(-3,+52)` offset is real: the player's height on the wall follows the frame rate of the descent, as difference 3 shows. (3) Reference draw `e20049`, my last lead, is ruled out: DstAlpha/One onto alpha 0 adds nothing. (4) The reference's window is not one colour. `emu_gpu7` runs from (16,12,6) to (255,255,203) with the wall steady, and `rdc_ref2` holds (255,255,204) for 200 s and then falls to (169,134,86). Its orange has the bare gradient's hue. (5) `idle_ours` ran at about 1.4 fps at its end, so it was a poor sample. Run `idle_long` tests whether ours ever goes orange.

**A correction.** The first entry for difference 4 called the clouds "flat and blurred" in ours and suspected the cloud clusters. Wrong on both counts. RenderDoc shows that scene is a Bink movie (`fmv\Cloud_Flying.bik`: Y at 1280x720, U and V at 640x360, three draws a frame), and its frames come out soft and detailed in ours (`out/rdc/rdc2/pass_002_e76.png`). The soft look at 120 s is the movie's own cross-dissolve, and I had compared frames 15 s apart. Movie frames at the same moment still need comparing.

**Ruled out: auto-exposure from a CPU read of the 32x32 target.** A theory for the sky, since 32x32 resolves alternate between pages `1F850` and `1F851`. `_depthEffects_LuminanceHistogram` (`sub_8228AF70`) does zero 256 bins at `0x828E24F0` and lock a texture, but the bins it then scans are filled from a fixed table at `0x827B71xx`, not from pixels, so Renut not writing resolves back cannot change it.

**Draws the game writes itself (`kameo_ring_census`, off by default).** Our hooks replace the draw functions and stub the ring, so a draw packet written inline goes nowhere. RePowered names such writers. The census parses the ring sink: `census1` counted 1.5 million draw packets, which looked alarming, but they are `C0003600 00010081`: one auto-indexed point, again and again, with register writes between. The writers: `sub_820C63F8`, called by `sub_820CA9E8`, which writes device `+0x28A4` from the VS and PS register counts and looks like `D3DDevice_SetShaderGPRAllocation`; it loads immediate shaders and draws these points, by its shape a pipe drain and not picture. `sub_820C6A50` builds a rectangle (`00030088`). I first guessed these points were the flame's particles: wrong. The census now sorts by initiator; anything else is logged as a real candidate. `census3` (title, menus, movie, the level's opening with the attack buttons pressed): 996,744 draw packets, **all D3D's single points, 0 rectangles, 0 others**. So no draw of the game's is lost in that path. `drawTube` (the water cannon) is the known one, and it needs that weapon on screen: untested.

The reference fakes occlusion query results too (`query_occlusion_fake_sample_count`, as our hook's 1000), so RePowered's real occlusion counts for light glows fix no difference from the reference. They go in the report as a decision.

## Ryujinx runs

| Run | NRO | Result |
|---|---|---|
| `emu1` (22:56, 420 s, verify keys) | first build, 46 MB | deko3d draws the title and menus. No crash. Every verify line 0 differ, plus 3 `fast_draw_verify` notes with the same pipeline key and a different object, each beside a uam "background run-time variant" line (the generic variant while the specialised one builds; katamari-nx and pacman-nx record the same). 100 % fast draws, 4.6 % whole buffers, 50-55 fps at the menus. **False colours**: this test's toml turned on `banjo_renut_game_gamma` while this binary still read N&B's ramp offset, so the present looked up garbage. Script off by one press: it reached Expert, which asks for the Power Pack. Neither is a renderer finding; to be confirmed by the next build. |

`emu2` (23:25, 480 s, verify keys, DLC in place): **the colours are right on deko3d**: title and menus match the PC, the same gamma ramp read (`[64]=307`, `[128]=568`, `[255]=1023`). All four DLC packs installed through hard links in the emulator's `dlc` folder (the installer reads them as they come; nothing copied). 39 verify lines 0 differ, 1 provisional-pipeline note, 100 % fast draws, 46-60 fps at the menus. Three host stalls of 40-60 s with no frames at all (one at 13:27, two at 13:31, while other ports were building); the first ate the START press, so the script went to Leaderboards and never reached gameplay. Fix: `input_script_polls`, the script's clock counts the game's own polls. The magenta contours (difference 4) show on deko3d too.

`emu3` (23:49, 480 s, verify keys, DLC installed, poll-clock script): **reaches gameplay on deko3d**: Rare logo, title, menus, caution, opening movie, Thorn's Castle's tutorial dialog, with the right colours. No crash. 37 verify lines 0 differ, 8 provisional-pipeline notes. 99.9 % fast draws, 80 % whole buffers. Render thread 85-143 ms a frame in gameplay with every verify key on, so that figure is not a timing. GPU memory region peaking at 1,830 MB: see below.

**Runaway render targets on deko3d (`emu3`).** PC: 20 render targets, 52 MB. Emulator: 33 MB in the menus, then 39 targets and 768 MB in gameplay, with `Depth target 42A42250: 797x5410` in the log just before image heap chunks of 156 and 163 MB, and then "42 MB of the GPU memory budget left ... 1 cold host textures released" repeating, which is a likely large part of those 85-143 ms frames. `0x42A42250` is in the game's heap. Likely cause: the render thread reads a surface's header from guest memory when it replays a draw, by when the game may have freed a temporary header and its memory been reused; a slower render thread widens that window, which fits the PC not showing it, and a console could hit it too. Fix: the draw hooks read each bound surface on the game thread (`NoteBoundSurfaces`: three words per bound surface per draw, the rest only on a change), and the render thread takes that copy (`d3d::SurfaceDescOf`). Not a graft, and no change to the picture. A `[kameo] surface ... changed since a draw bound it` log line reports what a late read would have taken, so the next run tests the cause directly. Not yet built or run.

`emu4prof` (00:03, 540 s, no verify keys, sampling profiler): fps 57-60 at the menus; then 0.6-1.6 fps as gameplay starts, render thread at 91 % of its core and 1,139 ms a frame. Its samples: `FlushVertexFillsLocked` → `ConvertVertices` / `ExpandElement<2>` and `memmove`, reached from `QueueKickFlush`. Its counters: **cache 128.0 MB (the limit), "stream cache full 448, index cache full 14367"**, and 889.9 MB through the upload ring in one 10 s window. The PC's cache reached 234.5 MB in the same level, so the base's 128 MB is too small for it. `banjo_renut_stream_cache_mb` goes to 256 (a size, no effect on the picture); `emu5prof` shows why: the cache peaks at 223.5 MB during the level's load and settles at 42-48 MB in play, with no "cache full" line at all. The log stopping 3 minutes before the end was not a hang: the profiler file carries on to the end; this config buffers its log and the run is stopped by force, which most likely dropped the tail. Test tomls now write it unbuffered.

`emu5prof` (00:16, 600 s, no verify keys, sampling profiler; both fixes in): **the cause is proven**: `[kameo] surface 42E72820 changed since a draw bound it: bound as 128x128, its header now reads 1379x2964`, and more of the same (`7905x5968`, `1x1`) up to the 16-line cap. Those are the temporary 128x128 surfaces. No garbage target in the run. Render targets 21 and 169 MB against 39 and 768 MB; GPU region peak 963 MB against 1,830 MB. Gameplay holds 12-19 fps in the emulator with the render thread at 48-77 ms a frame (86.5 % of its core), a game thread at 85.5 %, the main thread at 37 %, XMA 12.9 % and the audio worker 11.9 %. 77-78 % of draws from whole buffers; the rest are the "kind" reason. These are emulator figures, proxies for the A57. Still to explain: 169 MB of targets against the PC's 52 MB.

## Machine copies (2026-11-03)

From `emu5prof`'s samples (ARM, the emulator), `tools/switch_hot_bulk.py --min-samples 2 --merge`: 247 functions in `localized_bulk.txt`.

| Run | Copies reached | Calls compared | Differences |
|---|---|---|---|
| `verify1` | 39 of 243 | 91.8 million | 0. Few reached because I ran `localize_native.py` alone: callers stayed direct (`__imp__sub_X`), and only `direct_calls.py` in `post_codegen.py` sends them through the hooks. Always run the whole of `post_codegen.py` after changing a list |
| `verify2` | 184 of 247 | 115.2 million (19.3 million not comparable) | 0, and 3 calls that differed once and not when repeated |

63 copies were not reached on the PC in that path, so they are not verified yet. No hand-written natives: none without a console profile.

With `emu11prof`'s and `emu13docked`'s samples merged the list is 446. `verify3` (the guard on every call): 278 reached, 140.7 million calls compared, 82.8 million not comparable, 0 differences, 838 that differed once and not when repeated. It never reached gameplay: under 1 fps with every call of 446 copies guarded, so its 226 copies with no compared call say nothing. `verify4`, asked for one call in 64, did no better (152.8 million compared, 0 differences, the same 226) because `banjo_cpu_verify` guards every call whatever the period; `verify2` ran the same way at about 50 fps with 247 copies, and the 199 added include the Bink decoder (`sub_8270E030`), which fits the crawl from the movies on, though I have not isolated it. `verify5` leaves that key off so the period counts (each thread's first 4,000 calls, then one in 64), and gets to gameplay at about 4,500 draws a frame: **426 of 446 reached, 18.5 million calls compared, 5.9 million not comparable, 0 differences**, 101 that differed once and not when repeated (races: the guard restores the inputs and repeats a difference up to three times, and a wrong copy differs every time). 110 have no compared call: 20 never reached, about 90 reached but every call hit something the guard cannot repeat, among them the Bink decoder (`sub_8270E030`) and `sub_820E7F10`. **Those 110 are out of the build**, held in `localized_unverified.txt`: the request is verified before on, and for them there is no evidence. 336 copies remain. Teaching the guard to compare them is the next step on this track. `tools/hot_verified.py` then moves any copy still without a compared call to `localized_unverified.txt`, out of the build.

**Where the drawing thread's `memcpy` goes** (`emu6prof`, callers of leaf samples): `CaptureDelta` 54.6 %, the whole `Capture` 29.7 %, a copy of `sub_8232CFF0` 11.2 %. Each 10 s report: 43,756 draws at 418 bytes each, and 9,401 `Begin*Vertices` draws as whole copies of 12,960 bytes, about 122 MB. `banjo_renut_deferred_chain` puts those in the delta chain. It is off in the base because N&B draws on two devices from two threads (emulator hangs in 4 of 8 runs). Kameo has one device in every run. On the PC, `chain4`: **36.3 million draws checked through gameplay, 0 differ**, and 0 whole copies for those draws. On by default here. The key is read once at start, so it has no in-run A/B target; on and off can only be compared across runs. Its hang history means it needs repeated emulator runs: see "Ryujinx runs".

`emu6prof` (01:07, the copies on, 223 of 247 reached in the emulator, NRO 46 to 51 MB, about 20 KB a copy): gameplay 13-16 fps with the render thread at 64-70 ms, against 12-19 fps and 48-77 ms without them. No gain can be claimed from this: the difference is inside the noise of a shared machine and a JIT, and the console decides.

## DLC (2026-11-03)

All four packs install through the base's installer on both builds (Kameo Add-On Pack, Design-A-Skin Pack, Power Pack, Winter Warrior pack; hard links, so nothing is copied and the originals stay as they came). Their unpacked files hold 73 CAFF archives (65 `.mdl`, 8 `.lvl`). `tools/kameo_files.py` over the installed folder: 1,668 distinct shader containers, **1,245 of them new**, so the set goes from 4,616 to 5,861. Without them DLC content would have missing shaders. SPIRV-Cross failed on 2 of the new ones (`ps_31E80B337E6737C3`, `ps_D8285199D6D8B325`, both the Power Pack's `008B5284_42.mdl`: "Maximum compilation loops"); `tools/deko3d_shaders.py` now retries such a shader with `--force-temporary`, which only changes how the GLSL is written. RePowered's other DLC hooks (`sub_820B0878` the sign-in remount, `sub_820B1268` the licence mask for version-0 packs) are not taken yet: neither pack problem has shown here, and a DLC skin has not yet been selected in a run.

`emu11prof` (03:30, 480 s, the current NRO, chain on, A-only script, captures): **logo, title, menus, opening movie and 4 minutes in Thorn's Castle, no hang.** The game warns about overwriting the save the earlier run made, which is right. Render thread about 89 ms a frame, 10 fps, against 48-77 ms and 12-19 fps in `emu5prof` and `emu6prof`. Whether that is the chain or the machine cannot be told across runs (mean CPU 75 %, up to 9 compiler processes, free commit 6.3 GB), so the chain now has an in-run target, A/B 71.

`emu12ab` (03:41, 18 minutes, `renut_ab.toml`): every target printed. **Correction (2026-11-03): I wrote "no hang" here and it did hang.** Frames stopped at 17:49:31 and the log ran on to 17:55:42 with none, the Main XThread and the workers parked in the job queue's wait; see "The emulator freeze" below. **Nothing could be judged from it.** The list starts at boot, so targets 71, 25, 28, 22, 33 and 42 fell across changing screens (menus, movie, load). The two measured in gameplay: 70, the copies, 88.9 ms on and 87.0 ms off; 1, the do-nothing control, 99.0 ms on and 102.0 ms off. The control's own spread is larger than the copies' difference, with the machine at a mean of 87 % CPU and up to 20 compiler processes from other ports. Neither the copies nor the chain are shown to gain or lose time. The console decides.

## The round after the first console profile (2026-11-03)

**Superseded on copies:** the 4,089 below became 13,855 at the user's request, which crashed at boot on the console (the next section), and then 999, which is what ships. The user's limit is now about 70 MB of NRO, and no more machine copies for now.

The evidence is `docs/console-run-2026-11-03.md`: 27.8 fps without compile waits, 22.7 fps overall, 34.1 s of 252 s blocked on 79 shader compiles, and the game's drawing thread the limit at 94 % of a core. No frame rate below is from a console: this round has not run on one.

**Machine copies: 336 to 4,089.** The 110 held back are in the build at the user's request (below, how many of them the guard could compare), plus 553 sampled on the console and 3,090 they call, followed to a fixed point. 48 compile units instead of 12, because GCC ran out of commit memory on a shared machine. NRO 54.8 to 99.5 MB, about 11 KB a copy. About 10,100 eligible functions are still not copied: none has a sample on any machine, and they would take the NRO to roughly 210 MB. That is the user's decision.

**Hand work, in `app/src/cpu/hot/` and `tools/hand_copies.py`.** The user asked for the top 6 of the drawing thread. By samples they are:

| Function | Share of the thread | What was done |
|---|---|---|
| `CaptureDelta` | 15.3 % | One check for up to 8 lines (`LinesAnyDiffer`), then line by line only in a run that differs. 75 % of its samples were the compare, most of them the horizontal max, the move to a general register and the branch, once a line. |
| `sub_82379E10` | 7.9 % | A hand edit of the copy: rejected elements in a loop of their own, with no float reloads, and a prefetch 8 elements ahead. |
| `sub_82110280` | 5.2 % | **Nothing by hand.** A dispatcher on a state id, 85 % of its samples on loads, none over 6.5 % in one place. It gains only from the memset native inside it. |
| `sub_820E7F10` | 4.7 % | **Machine copy only.** A 49-way command interpreter with 32 real calls. Threaded dispatch would mean a per-file compiler flag, for a gain I could not estimate. |
| `sub_8231C268` | 3.2 % | **Nothing of its own.** No instruction over 3 of its 232 samples. It gains from the memcpy and constant natives inside it. |
| `sub_820C9C38`, `sub_820C9D80` | 2.7 %, and its pair | Native: a plain copy and the dirty mask, instead of emulated unaligned vector loads and a card per vector. |

Also native, because they sit inside those: memcpy (`sub_8255D400`) and memset (`sub_82554238`). Each was read against this game's statements, since XDK 2.0.2135 compiled them differently from N&B's.

`tools/hand_copies.py` is new: an edit of a copy's statements, applied to `f_` only, so `o_` stays the original and the guard compares the two. It stops if the code generation changes the statements it expects.

**Shader compile stalls.** The package never shipped the PC-built cache `tools/deko3d_variant_cache.py --generic` makes, so every shader not yet seen compiled on the console. It now does: 8,379 variants, 33 MB, from every shader's generic variant, the console's pipeline list and its log. All 797 variants compiled on both the PC and the console are byte-identical. `deko3d_shaders.idx` keeps that many records from slowing each boot; a wrong or stale index falls back to the walk. uam itself is no faster. A pooled allocator looked like 30 % of its time, but its results are freed by code using the normal allocator, a memory corruption risk not worth taking for compiles that should now be rare.

**Checked in the emulator** (Ryujinx, a shared machine; correctness only):

| Run | Settings | Result |
|---|---|---|
| `emu30guard` | Guard and every verify key | 4.6 million calls compared, 4.4 million not comparable, **0 differences**, 119 that differed once and not when repeated. 11.87 million draws through the device snapshot check, **0 differ**. 0 compiles, 0 waits. |
| `emu31plain`, `emu33plain`, `emu34plain` | No guard | 0 compiles, 0 waits. Wall 20.7 where the old build had 20.6, lilac about 1 %. |
| `emu32plain` | No guard | **Froze** (below). |

1,919 of 4,084 copies were reached, and 929 had a compared call. Compared: `sub_82379E10`, both constant writers, memcpy, memset, `sub_82110280`, `sub_8231C268`. **Not compared, every call hit a real call: `sub_820E7F10`, `sub_8233F578`, `sub_82524E98`.** So those run on the tool's correctness alone, as the user chose.

### The emulator freeze

`emu32plain` stopped drawing at 13:34 and did not recover in the 6.5 minutes left. The process lived. The Main XThread blocks with an infinite wait in `sub_82264F40`'s queue for the end of the frame, the drawing thread waits on its own queue for its next job, the render thread has nothing to do, and the workers are idle: each waits on the other, so a message or a semaphore signal was lost. **The cause is not known.**

- It is not new: `emu12ab`, a 53 MB build from before this round, did the same (frames stop, log runs on 371 s, the same queue wait). So did `emu8` in part. That is 1 of 5 runs on this build against 1 or 2 of 26 before, which is too few to say the rate changed.
- Every case had heavy host load (`emu32plain`: 100 % CPU, free commit down to 0.5 GB, 20,000 pages a second).
- `thread_signal_skip_idle` is cleared by reading it: the waiter count and the state change under one mutex, so a signaller that sees no waiter is safe.
- It has not been seen on a console. Only one console run exists, 6.5 minutes on the old build.
- The separate collapse in `emu31plain` and `emu8`, under 1 fps with every thread 30-40 times slower at once, recovers by itself and looks like the machine: nothing of the port stood out in the stacks. That is an inference from the stacks, not a test.

If a console freezes this way, the first test is `banjo_cpu_native_bulk = false` in `kameo.toml`, to separate the copies from the rest.

`emu13docked` (04:01, 600 s): **docked at 1080 lines reaches Thorn's Castle with no crash**: render targets 65 MB at x1.5, textures 187 MB, GPU region peak 1,072 MB.

`emu14` (05:34, 560 s, verify keys, **the NRO that ships**, 54.8 MB, 336 copies, all 5,861 shaders, the chain on): logo, title, menus, opening movie, Thorn's Castle. 40 verify lines 0 differ; 3.17 million snapshot draws checked, 0 differ; 335 of 336 copies reached, 2.6 million calls compared, 0 differences; no missing shader, no dropped draw. GPU region 938 MB. Render thread 118-258 ms a frame there with every verify key on, which is not a timing.

Machine load matters to these figures: `tools/load_log.ps1` logs it beside a run. The Game List crawling at 1.5-3 fps in `emu9` was not the save, as I first thought: `emu10prof` holds the same page at 60 fps with a save present, and its slow stretches sit in a period with the CPU at 100 % and 15 compiler processes from another port.

## The round after the second console profile (2026-11-03 evening)

The evidence is `docs/console-run-2-2026-11-03.md`. The stall fix worked (blocked on compiles 34.1 s to 0, worst hitch 7.9 s to 0.44 s, 22.7 to 26.3 fps overall), and windows without waits did not gain (27.8 to 26.3 fps; the scene differs, XMA 4,769 against 3,496 contexts a second, so that comparison separates little).

**Why the 13,855-copy NRO crashed.** Its code spanned 159 MB, past AArch64's 128 MB branch range. `exception_handler_switch.cpp`'s `bl svcReturnFromException` then went through a linker veneer (`adrp x16 / add x16 / br x16`) after the handler had restored the registers, so interrupted code resumed with x16 holding the image address, and the next store card faulted. So 128 MB of code is a hard limit here, and `nm` must show no veneers. 999 copies: 68 MB, 0 veneers.

**The fourth core** (the user's request): on by default, as banjo-nx does it. Only the Audio Worker, the heavier audio thread in both console runs (18.8 % of a core against the XMA Decoder's 16.8 %), and the shader compiler, held to 10 % of the core. The XMA Decoder and the audio output stay on cores 0-2 (`banjo_system_core_xma`, `banjo_system_core_audio_output`, both false). The Audio Worker now samples the core's idle time, since the output thread that did is no longer there. A launch from the homebrew menu still gets three cores: only a forwarder can be granted the fourth. `SphairaForwarderOfThisNro` (from renut-nx) recognises Sphaira's id for this NRO's path, `05E750DF55751000`. Against banjo-nx's measurement that the system takes about half of core 3 and stutters at 41-55 %, about 29 % should fit: that is a reading of two numbers, not a test on Kameo.

**Hand work on the five functions the user listed.** All samples are the second console run's, on the drawing thread. Estimated total: about 3 % of the thread, about 1 ms of its 38 ms. That is an estimate from samples, and my last one (`CaptureDelta`, a predicted 65 % cut that came in at 13 %) was wrong, so the console decides.

| Function | Share | Where its samples are | What was done |
|---|---|---|---|
| `sub_820E7F10` | 10.7 % | A 4x4 matrix multiply for bones, at least 67 of 332. The context hand-over, 60 of 332. | Natives for both matrix multiplies, a prefetch two bones ahead, and registers left in the context (below). |
| `sub_82379E10` | 6.7 % | The skip loop, 77 of 209: 53 on the element load and its prefetch, and about 12 writing cr6 to the stack per element. | No cr6 in the loop, and its values in locals so they stay in registers. |
| `sub_82110280` | 5.0 % | Flat: 88 % on loads, tree walks and object fields, none over 7 %. | **Nothing.** I found nothing to remove. A tree's next node is unknown until the load, and a cache of lookups would need to know when the tree changes, which would not be exact. |
| `sub_8231C268` | 4.2 % | Flat: 115 addresses for 129 samples. | **Nothing of its own.** Keeping f and v in the context was compiled and rejected: 312 more instructions and 82 more context stores. It does gain from the card fix below. |
| `sub_82524E98` | 3.4 % | The same skip-loop shape, 43 of 105: 23 on one load with no prefetch, the rest on four spills and cr6. | A loop of its own with a prefetch 8 elements ahead. |

- **Matrix natives** (`n_820E0DC8`, `n_820E0CE0`, key `banjo_hand_matrix`): the generated code transposes A, takes sixteen dot products at a multiply, two `faddp` and a `dup` each, and shuffles the results together. Read from those statements, each result is `(B[j][3]*A[3][i] + B[j][2]*A[2][i]) + (B[j][1]*A[1][i] + B[j][0]*A[0][i])`. The native does the same multiplies and the same additions in the same pairs, four results at a time, so rounding matches. `sub_820E0DC8` is the transposed form. They write no register. Checked by reading what follows each of the 427 call sites: nothing reads what they leave behind. One known limit: with two NaN operands the result's payload could differ, because GCC treats multiply and add as commutative, in the generated code too.
- **Context registers** (`KEEP_IN_CONTEXT` in `tools/hand_copies.py`): the interpreter's `f_` keeps f1-f13 and r3-r10 in the context, which is the original statements' own form, so it is exact by construction. Its 29 real calls carried 286 stores and 377 loads of f1-f13, because the back edge makes every register "maybe changed", and the handlers use those registers in 22 places. Compiled: 7,399 to 6,774 instructions, 1,090 to 688 stack accesses, 155 to 22 floating-point context accesses.
- **`Card` in `banjo_hot_common.h`**: GCC turned the card loop into a `memset` call, with everything live saved around it, on the hot path of the natives, including the constant uploads. Up to a page now takes one or two direct byte stores.
- **`hand_copies.py` limits**: the skip loops read the count on every element, as the originals do. Hoisting it would save little and need an argument about other threads.

**Clip planes (from Ridge Racer 6's session):** Renut never applies user clip planes (`ClipControl & 0x3F`). Kameo's code stores them too but nothing here shows whether it uses them. A census needs a logging build. Suggested after this console run.

**The guard on the 999-copy build before these edits** (`emu50guard`): 0 differences in 5,564,376 compared calls, 4,223,048 not comparable, 130 that differed once and not when repeated; 875 of 995 reached; 18.6 million snapshot draws, 0 differ; 0 compiles. `emu51plain`: wall 20.6, lilac about 1 %.

**A testing mistake.** `tools/run_ryujinx.ps1` launches the NRO on the emulator's card, and nothing copied new builds there after 18:50. So `emu52guard`, `emu53core` and `emu54gpu` all ran the old rc5 NRO, whatever they were meant to test, and I told the user the hand work had passed the guard when it had not run. `run.nro.txt` in each run's folder shows the size and hash that ran. The script now copies a newer build to the card first. The hand work's first real guard run is `emu55gpu` below. `emu53core` tested nothing of the four-core layout; the third console run shows it working on the hardware.

## The GPU round (2026-11-03, night)

From the third console run: GPU 22 ms a frame, of which colour resolves 3.6 ms, depth resolves 0.95 ms, the present pass 0.86 ms, and the rest draws. **At handheld this does not raise the frame rate yet:** the drawing thread takes about 34 ms and is the limit. It shows in the log's GPU time, and in the frame rate once PGO + LTO lower the CPU side, or docked.

Ten changes, each exact by the argument given, each with its own key, on by default. Keys 1-5, 7, 8 and 10 are also A/B targets 73-79 where listed.

| # | Change | Key | What the emulator's counts show |
|---|---|---|---|
| 1 | A whole colour resolve is left in the target's image; a whole clear gives the image to the texture | `banjo_deko3d_color_handover` (73) | About 2 full-screen copies a frame gone, 0 lost. 9 more a frame are drawn anyway because a draw writes the target next |
| 2 | The frame's last resolve is scanned out from that image | `banjo_deko3d_scanout_handover` (74) | One of those 2 |
| 3 | A depth resolve is sampled from the depth target a draw only tests | `banjo_deko3d_depth_handover_sampled` (75) | "Sampling it with its target bound" from about 2 a frame to 0; about 1 a frame still drawn for a draw that writes depth. So 1 copy gone, not 2 |
| 4 | A sampled depth target stays in the read layout for draws that do not write it | `banjo_deko3d_depth_read_layout` (76) | Fewer barrier flips, part of the fall below |
| 5 | The gamma ramp is copied when it changes, every frame only for the first 300 | `banjo_renut_gamma_upload_once` | Full barriers 1.01 to 0.01 a frame; each copy also cost an L2 invalidate |
| 6 | The gamma ramp as one 32-bit float per channel, not four to a texel: the same floats, a quarter of the bytes | none (built in) | See below: 8-bit levels were tried first and were not exact |
| 7 | No `pow` in the scan-out at a gamma of 1 | `banjo_renut_present_no_pow` (78) | |
| 8 | The scan-out loads the texel at one to one | `banjo_renut_present_texel_load` (79) | |
| 9 | A 32-register cap on four shaders whose code does not grow (`1C9F38A0CCE5CDF6`, `553A76B76E6DB923`, `1CFDF6EE716976B1`, `58C40F6F4EED6C75`), about 0.8 ms of GPU time | `banjo_deko3d_gpr32` (16) | Unmeasured, and could go either way: more threads against spills to memory. The two heaviest shaders (47-48 registers) grow 5-9 % with the cap and stay out |
| 10 | A resolve that draws nothing leaves the targets bound | `banjo_deko3d_resolve_keeps_target` (77) | |

**My estimate was wrong again before the first test.** I expected about 1.4 ms from item 1, from the 12 resolves a frame of the 512x512 render-to-texture loop. The reason counts show those, 16 a frame in all, copy part of the target into a smaller texture, which a swap of images cannot do. Revised estimate for all ten: about 1.5 ms of the 22, not 2.5-3. Fragment barriers fell only from about 63 to 59 a frame, for the same reason.

**The 8-bit ramp was wrong, and the test that found it is worth keeping.** I first stored each ramp entry as the 8-bit level it should round to, having checked that n / 1023 is never nearer an 8-bit tie than 0.0014 of a level. That argument assumed the GPU's float-to-8-bit conversion rounds exactly. It does not at that margin, at least on this PC's GPU under Ryujinx: comparing which output levels each channel ever takes, over all of a run's captures, showed the old build with a gap at 160 and the new one with a gap at 163. Three entries in a row, each close under a tie because the ramp climbs by 4 there, came out one level lower. Now the ramp keeps the floats and the GPU rounds them as before. `ramp_levels` (the set of levels, old run against new) is the only exact test of the scan-out these runs give, because the set does not depend on the scene's animation.

**Deferring the destination's read barrier to its first bind was considered and left out:** it would move a dozen barriers from the small render-to-texture loop into the middle of the main pass.

**Checked** in `emu55gpu` and `emu56gpu` (the right NRO: `run.nro.txt` against the build), guard and every verify key on:
- `emu57gpu`, the build that ships: 0 differences in 4,083,058 compared calls, 4,035,748 not comparable, 156 that differed once and not when repeated; 873 of 993 reached; 0 hand-overs lost. The output levels of every channel match the old build's exactly (`emu54gpu`, which really was old). Lilac 0.97 %.
- `emu56gpu`, with the 8-bit ramp: 0 differences in 3,804,761; its level gap at 163 is what found the ramp's fault.
- **The wall is not settled.** It holds to about 0.01 within a run, and between runs (mean over the box, of 255): old build 19.59 and 19.61, nearly the same new code 19.56, this build 19.50. Runs of almost the same code differ by 0.06, so 0.1 cannot be called a change or noise from these runs. Comparing whole frames does not help: two old-build runs agree on only 7.6 % of pixels. An in-run A/B of the keys is the test, since it has no run-to-run difference.
- `emu55gpu`: one `DIFFERENCE`, `sub_82359AE0`, r3 0x4525E against 0x45260 after 285 compared calls, which switched that copy off for the run. That function holds none of these edits and it did not come back in `emu56gpu`. **The cause is not established.**
- **Not compared by the guard:** `sub_82524E98`, `sub_820E7F10` and `sub_820E5100` are in `hot_unverified.txt`, because every call hits a real call. So the skip loop in the first and the context registers and prefetch in the others rest on the reasoning, and on 80 s of play on the console (rc6). `sub_82379E10` and the two matrix natives were compared.
- **Pixel-identical is not proven by these runs.** The scene animates, so captures of two runs differ everywhere. The only bit-identical capture, at 15 s, is one flat colour and shows nothing. What exists is the arguments above, the wall and lilac measures, and the verify keys. Sampling a depth target while it is bound for testing is the change most worth watching on the console: Ryujinx models neither depth compression nor Zcull.

Ridge Racer 6's session reports a deko3d fault for indexed strips with 0xFFFF restart markers in the range draw path (their `rr6_strip_markers`): huge wedges for a few frames after a mesh first appears. Not in this build. Whether Kameo has it is not tested yet: about 17 % of its draws go through that path.

Checked in `emu63strip` (2026-11-04, `banjo_renut_whole_buffers = false`, rc9's NRO): 502,103 draws, all through the ranges, and no wedges or torn meshes in 31 captures. **That is weak evidence.** They cover only the menus, the opening cutscene and the first wall climb, and the run did not count how many draws carry a marker. Whole buffers stay on by default, so the console takes that path for most draws anyway. Nothing was changed.

## The cheat codes (2026-11-03, night)

The user asked for Kameo: RePowered's four codes. `app/src/kameo_codes.cpp` and `kameo_collectibles.inc`, adapted from its `src/ui/pause_menu/`, `src/ui/text_window.cpp` and `src/engine/` (MIT, `THIRD_PARTY_NOTICES.md`). Eight presses on any pause or options page, a sound, and an "Extras" line appears at the end of Game Options for good. With no code entered no page changes, and no picture changes.

| Code | Presses | What it adds |
|---|---|---|
| Level Select | LB RB LB RB X Y X Y | 13 chapters, each with its tasks; a confirmation with the cursor on Cancel |
| Unlock All Extras | X X Y Y LB LB RB RB | a toggle: events 485-526, the mirror's unlockables, answer done |
| All Warriors & Upgrades | Y X Y X RB LB RB LB | a toggle: the ten warriors in the sack, move events 239-278 answer done |
| Collectibles | X Y LB RB X Y LB RB | the tracker: this area, the whole game, and how many were missed |

Choices live in `kameo_codes.txt` next to the NRO, not in `kameo.toml`, so a new package does not lose them. `kameo_cheat_codes = false` removes it all; `kameo_codes_test_unlock` unlocks for tests without saving.

What differs from RePowered, which is written against the original game:

- **The title update's own code decided every address that mattered**, not the maps. The maps were wrong in three places: the cost table's rows are 16 bytes (12 in the original), the task jump's four words are 828E1F0C-828E1F18, and its last stores are 64-bit.
- **All Warriors exists here though RePowered has not ported it to the update.** `kaiSackPopulate` (821A3FF8) adds each warrior by name after its story event, and the add (821A3C30) refuses a duplicate, so adding all ten after it is safe. The names are the game's own strings.
- **`eventDone` is not hooked.** It is a machine copy, inlined in 23 others. `tools/hand_copies.py` gives the copy one test of a flag instead. **So the two cheats depend on `banjo_cpu_native_bulk`, and a guard run with a cheat on will report differences for that function.** With both off the guard compares as before.
- **The pause closes by the owner's Start press** set in the pressed word for one call of 82278C30, not by writing its state as RePowered does: the update's close path also calls 822AF990, and this way the game's own path runs whatever it holds.
- **Left out:** RePowered's Options pages, tribe shop sales, translations (English labels), and its cheat guard (leaderboard mark, save backup): this port has no leaderboards, and both cheats need a deliberate toggle after the code.

**Tested: not yet.** It compiles, and `nm` finds the eight hooks and the flag in the binary. No run has entered a code: `emu60codes` and `emu61codes` used up their time before the script's first Start (the script's clock runs at about a third of the wall's in gameplay on this loaded machine), and the user chose to test on the console, so `emu62codes` was stopped. The scripted test is in `out/emu/emu_codes.toml` if it is wanted: gameplay is on screen by 160 s of the script's clock, so allow over 20 minutes.

## Point sprites (fixed 2026-11-04, the user's decision)

Reported by the Beautiful Katamari session, then checked here. Kameo's point sprites were wrong for two separate reasons.

1. **The geometry shaders' layout.** XenosRecomp has FOG0 (katamari-nx a0268462), so vertex and pixel shaders put fog at location 23, the point coordinate at 24 and the size at 25. `point_sprite_gs.hlsl` and `rectangle_list_gs.hlsl` still had the coordinate at 23 and the size at 24. So the size came from the vertex shader's coordinate, always 0, which fell back to the render state's size, and a killed point was not dropped. On D3D12 the stages do not link: `out/pc_runs/verify1` and `verify5` log "Pipeline creation failed" for VS 04D58836F1C8402F, PS 84162BFF585D487A, topology 1. Deko3D reports nothing. Fix from vivapinata-nx 952a38c2, the two `.hlsl` files.
2. **The pixel shaders never received the coordinate.** That patch alone does not fix Kameo. The sprite shaders sample at `abs(r).zw`, which came out as `abs(vec4(0.0)).zw`, one fixed texel. XenosRecomp only set up the generated register when `fieldC & 6` was nonzero. These shaders have `fieldC` 0x200 or 0x100: neither low bit, but the register byte names the first register after their interpolators. RePowered's `src/gpu/shaders/particles/README.md` found the same register. Fix: `shader_recompiler.cpp` also takes a nonzero register byte. RePowered instead swaps in its own shaders for three pairs; this keeps ours.

**Exact in scope.** All 4,490 pixel shaders translated by the old and new recompiler: 4 differ, by one line each (`9A16289A84E5A85B`, `84162BFF585D487A`, `E1D5926C5F6332C6`, `D43301A9EF74280E`). Only those four were regenerated, so 12 of 8,475 cache variants are new. Rectangle lists should not change: every vertex shader writes fog and the coordinate as 0, and no pixel shader reads fog.

**Effects.** Dust motes (67EDA3D674722EC6 + 9A16289A84E5A85B), the burst of motes (04D58836F1C8402F + 84162BFF585D487A) and water cannon spray (511A3E3E3F0761D3 + E1D5926C5F6332C6), names from RePowered. Only the first two pairs have drawn in any run here. `D43301A9EF74280E` and vertex shader 519FBEC688E43106 are not in RePowered's list and are unidentified.

**Evidence, all on the emulator's deko3d path.**
- `emu69grad`, a throwaway build with two of the pixel shaders drawing their coordinate as colour: sprites about 5 pixels wide in the opening tower, red rising left to right (34 to 172) and green rising top to bottom (106 to 186). So size and coordinate both arrive, the right way up.
- `emu67sprite`, the real build: the opening tower looks the same as before. These motes are small and dim there, about 7 point draws a frame, 0.10-0.45 % of draws. **So no run has shown the real art changing.** The dense clouds RePowered describes (40,000 motes in what it calls the basin) have not been reached here.
- Not measured: the cost. Sprites now cover their real size, so a scene full of motes draws more pixels than it did.
- The PC build was not rebuilt, so the D3D12 failure is fixed by the source but not re-run. The PC's shader cache and `shader_cache_switch.cpp` were left as they were: the console reads the GLSL package.

## 3 fps in an area of the big level: the vertex cache's limit (rc16 console run, 2026-11-04)

`renut_001 (27).log`, `rex_profile (40).log`. CPU 2397 MHz, GPU 1305 MHz, so the overclock was already at its top:

| Window | fps | Render thread ms a frame | GPU ms a frame | Draws a frame | MB a frame through the upload ring | Stream cache |
|---|---|---|---|---|---|---|
| 12:48:39, battle | 47.9 | 16.6 | 14.1 | 3,687 | 1 | 107 MB |
| 12:49:29, quiet | 60.0 | 1.9 | 2.6 | 460 | 0 | 1 MB |
| 12:48:19, loading | 1.0 | 1005 | 1.4 | 4,787 | about 1,800 | 256 MB (full) |
| 12:50:10, the slow area | 3.1 | 328 | 3.9 | 3,048 | 568 | 256 MB (full) |

The render thread is at 103 %, in `ConvertVertices` under `FlushVertexFillsLocked`. The GPU is nearly idle. `banjo_renut_stream_cache_mb` is 256 in the package, and at that limit a draw can't keep a converted copy, so it converts its vertices again at every kick: 23,000-25,000 draws in 10 s for streams and about 20,000 for indices, against 0 in the battle. The cache fills in the first seconds of the area, and one 60-frame pass refused 153,100 copies. At 3 fps, that pass runs only once every 20 s. The heap had 913 MB never handed out.

**This probably also explains the level-load crash:** the same level's load in this run (12:48:19) shows `stream cache full 17102` with 18 GB through the ring in 10 s. That points to vertex data refused at this limit rather than bone constants. rc12's crash log has no 10 s report from the load itself, so it can't confirm this. The limit isn't the only heavy source, though: rc11's battle on this level put 2-3 GB through the ring per 10 s with the cache at 45-137 MB and no refusals, so `kind` draws (fans, quads, inline vertices, skinned once) cost a lot on their own.

**A bigger limit did not help.** The user ran `banjo_renut_stream_cache_mb = 768` and reported no improvement. That run's renderer log is missing: the `renut_001` sent was byte-identical to the 256 MB run's, and its `rex_profile` stops about 40 s in, at 3.3 fps with 1,781 MB/s through the ring. So the cache's state at 768 MB wasn't seen.

**The cause: range copies lock out the whole copies that would replace them.** The first slowdown in the same run shows it:

| Window | Frames | Refused, dropped by the idle pass | Cache | Whole copies made | Ranges replaced |
|---|---|---|---|---|---|
| 12:48:09 | 189 | 0 | 256 MB | 21.4 MB | 0.5 MB |
| 12:48:19 | 10 | 0 | 256 MB | 0 | 0 |
| 12:48:29 | 245 | 112,939, 208.5 MB | 105.9 MB | 33.0 MB | 197.5 MB |

Once the idle pass made room, 33 MB of whole copies replaced 197.5 MB of ranges, and 1 fps became 48. In the second slowdown the pass at 60 frames found only 1.3 MB idle. It needs copies unread for `kPressureIdleFrames` (150), which is 50 s at 3 fps, and ranges still being drawn never go idle at all. A range copy needs only `kStableFlushes` (3) kicks, which a split frame gets through at once, while a buffer the load just wrote can't have a whole copy yet, so the ranges always arrive first.

**Fix, rc17:** `WholeCopyFits` lets a whole copy pass the limit by `banjo_renut_whole_over_cap_mb` (default 256). Range copies stay refused at the limit. `DropReplacedRanges` runs every frame while the cache is over the limit, instead of every 16 frames. Draws of kinds that can't go whole (fans, quads, inline vertices) stay on ranges, so some cost may remain.

## Crash at the Microsoft logo (rc13, 2026-11-04)

The console's `rex_crash (17).log`: a read at guest address -8 in `n_82557A68`, the game's heap allocator, on the main thread during a file callback. It was walking a free list (head 0x40100180) and a block's next pointer was 0. So the heap was damaged earlier, and the log doesn't say by what. First time in the crash logs we have, and it has not been reproduced. The log has no `long frame split` line, so rc13's split did not run on that boot.

What the boot does: `sub_82107270` is the game's file thread. It mounts the four DLC packs as `dlc0`-`dlc3`, closes them, and mounts them again as `dlc4`-`dlc7`, while requests for the old roots are still in its queue. All 5 console logs that mount DLC have a failed open of `dlc2:\audio\M_DLCsfx.xwb`, and in 2 of them `dlcCues.ssf` fails too. Which ones fail depends on the timing of the close. 6 of 50 emulator boots with DLC also have at least one such failed open. This crash came 0.6 s after the failed open, but 2 console boots with the same order survived, and nothing shows the failure damaged the heap.

Two bugs of ours found on the way, both fixed in rc14. Neither is proven to be the cause:

- **Closing a pack took its files with it.** `ContentManager::CloseContent` released the guest handle of every file open in the pack (`CloseOpenedFilesFromContent`), then destroyed the device. A later read on that handle fails. The game's read failure goes through flag 412 of the request to `sub_82107628`, then `sub_820BA680`, unless the request has its own failure handling (416, 420). Now content of type 2 (`content_keep_closed_dlc`, default true) only stops counting as open: its root and files keep working, up to 8 packs, and a root name used again drops the kept one. Save containers unmount as before.
- **Completion was reported first.** `CompleteOverlappedEx` wrote the result, then the extended error and the length, then read the event and the routine. A game polling the result can read a stale length or free the structure while we are still writing to it. A length written into a freed block would land on its back link. The window is a few stores, so this is unlikely to be what happened here. Now the result is written last, with nothing of the structure touched after it. This is shared code, so the other ports may want it.

## Why the DLC scan ran twice, and the disc message (2026-11-04)

The disc message reached the console on rc14. `emu74disc1` reproduced it on its first boot, with no failed read or open, and the caller log names `sub_82313330`, at its end-of-file check of the error flag.

- `sub_82313330` parses a `.kad` file (`XGS:`, `XWB:`, `XSB:`, `CUE:` lines). Its error flag is set by an entry too long, a failed allocation, or a cue that won't go into the table at `0x82BD09E0 + 24` (`sub_8231EF78` returns 0 for a key already there). The log doesn't say which of these fired. A duplicate cue is the one that fits the rest of this, and the test below supports it.
- `sub_820B0878` polls the listener the DLC scan creates (`sub_820B1A10`). A sign-in notice counts as a change if the word at `0x82BCFE94` says nobody was signed in, and that word starts at 0. `KernelState::RegisterNotifyListener` sends `XN_SYS_SIGNINCHANGED` at startup (a Xenia workaround for another game), so it counted, during the scan.
- At the end of the scan, `sub_820B0D70` sees the extra count and calls `sub_820B1B30`: close each pack in reverse order, then `sub_82315AF0` unloads DLC audio and the scan starts again.
- `sub_82315AF0` removes the cues only of banks marked loaded (`44(r28) == 1`). A bank still loading keeps its cues, so the second scan adds the same names.
- The close also explains the failed `dlc2:\audio\M_DLCsfx.xwb` open on every console boot with DLC. Audio requests from the first scan were still queued behind it.

**Fix, rc16:** `renut::dlc::SeedSignInState` writes the sign-in state to `0x82BCFE94` before the guest starts (`kameo_dlc_no_rescan`, default on). The notice is then no change, and the scan runs once. The notice itself still arrives, so nothing else that listens for it changes.

**Why Banjo and Futari don't hit this:** their file code is the same as ours (Banjo's `host_path_*`, `xfile.cpp` and the VFS are identical), and they get the same notice. Kameo's DLC scan is what restarts on it. Futari has one fix we don't: an overlapped `XEnumerate` that fails completes with `ERROR_FUNCTION_FAILED` (1627), with the real error as the extended error, as Xenia does. Kameo's DLC enumeration treats both the same, so it isn't part of this. Its other two enumerations (`sub_822CAD78`, `sub_822EA4C8`) haven't been checked.

**wmw_nx and battd_nx on Horizon's file limits:** a read at or past the end fails with EIO where POSIX returns 0. A file open for write can't be opened or stat'ed again. wmw's open/close race on newlib's handle table isn't in this toolchain as written, because `__alloc_handle` and `__release_handle` take a lock, though `__get_handle` still returns a raw pointer with no reference count (battd's 1.2.0 guard). None of these was the cause here, but failed reads are now logged, because the layers above turned them into a silent end of file.

**The crash at the Microsoft logo** came on a boot with the same double scan, 0.6 s after the failed open. A link is likely, not proven: nothing shows what wrote over the free list.

**A correction to the next paragraph:** "no file error in the log" is wrong for `emu58ab`. My first reading, that a failed read raises this message, was also wrong for the case tested: `sub_82313330` raised it with no failed read.

Sometimes the game boots to its own page, "There's a problem with the disc you're using", and stays there. Seen by screenshot in `emu58ab` (rc8) and `emu65sprite` (the sprite fix), so the fix did not cause it; the next two boots of that same build were fine. Both stop just after the last DLC pack mounts, with no file error in the log, and never reach the save list. Files and hard links were checked and are intact. The sign in a log is no `list type 00000001` line: 13 of 47 runs have that, but many are short, and most of their captures are gone, so **that is an upper bound, not a count**. Never reported on the console. It may be a race in loading: the two failing logs differ in order from a good one, which proves little because good boots vary too.

**Check for it in any long run.** A run stuck here still logs 60 fps and no errors, and looks healthy.

**A correction: `emu58ab` measured nothing.** It sat on this page for its 25 minutes, and compared 0 calls. I reported "no crash and 0 differences while toggling". That was wrong: it says nothing about the GPU changes in gameplay. `emu57gpu`'s guard result (4,083,058 calls) stands: that run reached gameplay. The ten GPU changes have still not been toggled in gameplay anywhere.

**Another one, in the census diagnostic:** `emu67sprite` with `banjo_renut_diag_fast_census = true` died at 5:41 with a read of address 0, in a block starting at `FastCensusDraw`. `emu69grad` with the census off ran its full 16 minutes in the same scene. So the census is the suspect, though nothing in that function's first lines reads address 0. The key is off by default and not fixed. Do not use it for long gameplay runs.

## Audit against RePowered's renderer (2026-11-04, the user's request)

From the user's console run of rc10: a corrupted dialogue portrait, and glowing dots on walls that real hardware does not have. Then "audit for any issues we may have for the rendering like repowered". I read the comments and options of its `src/gpu` (draws, surfaces, resolves, textures, vertices, shader mods, hooks), not every line, and checked each candidate in ours by reading. Its stereo, HUD layout, movie replacement and texture pack code are features, not fixes, and were left out.

**Fixed, each with a key, on by default.** All change the picture, towards the original. On the console rc11's occlusion fix removed the stray lights (77,824 queries, 38 % 0 samples, 42 timed out). The others have no result either way yet.

| Key | What was wrong |
|---|---|
| `banjo_renut_real_occlusion` | Every occlusion query answered 1000 samples, so a light glow behind a wall drew anyway. Now deko3d's samples-passed counter, in order with the draws (`render/occlusion.cpp`, a new hook on D3DQuery_Issue 820D2668). The emulated reference fakes it too, which is why no comparison with it caught this. The glows and the sun are the only two users |
| `banjo_renut_poly_offset` | Nothing read the game's polygon offset at all, so decals could z-fight. Set per draw from the device (0x2E90, 0x2E94, enable bit 11 of ModeControl), front values for both faces as RePowered |
| `banjo_renut_cull_both` | ModeControl with both cull bits drew both faces. Now a real cull of both on deko3d, so the vertex shader still runs. RePowered drops the draw |
| `banjo_renut_last_pixel_shader` | A colour draw with no pixel shader wrote depth and no colour. The GPU keeps the last shader unless the EDRAM is depth-only (5). RePowered says fading models draw this way, such as an absorbed baby warrior. Not with no colour target, nor inside an occlusion query. My guess that this was the portrait's cause was wrong: rc11 did not fix it |

**The portrait and the exploded far-away goblins: a resolve's depth clear was dropped (`banjo_renut_resolve_clears_both`).** The imposter renderer (82102280) sets up its scratch pad and clears it once per batch. Then for each model: draw, resolve depth (flags 4), resolve colour with 0x300, "clear colour and depth for the next one". Our ResolveStateLocked cleared only the surface resolved, so the depth stayed, and each model was tested against the last one's: only what was nearer drew. The trace agrees: 156 of 466 resolves in `emu57gpu` are colour resolves of the 512x512 pad with 0x300. RePowered's `capture_commands.cpp` notes the same flags. **Read from the code and the trace, and not run yet.** It also clears the scene's depth at the 1280x720 resolves with 0x310, which it did not before, as the console does.

**Out of memory loading the big battle level (`banjo_renut_split_frame_mb`, 192).** Level 004CFC24, which only the last three logs reach. rc11 hit 4,260 upload ring failures at 1,882 MB and survived; rc12 crashed twice on the way in, `std::bad_alloc` on the render thread and a null write in the shader compiler, both the heap at 4-18 MB. The ring was the hog: "upload rings 1488 MB" with textures at 169 MB. It grew 16 MB every 20 ms. The earlier level uploaded about 60 bytes a draw and this one 1.9-4.2 KB. I first put that down to bone constants from its size alone, which nothing shows. Counted per inline draw (the "kind" ranges: Draw*UP, Begin*Vertices and the like) it is 300 bytes before and 5-18 KB here, with their share of draws up from 20 % to 24-40 %, so inline vertices are as likely. What the data is remains unmeasured. The ring only adds a chunk when one frame has outgrown all it has (`UploadRing::Allocate`), so a single frame did write that much, and the heap ran out. Logged block lines stop at a limit, which at first looked as if the heap had vanished by itself. **Probably not caused by the depth clear,** though that is inferred: rc11 had the same growth without it. The fix ends a frame on the GPU past the limit without presenting: SubmitAndAdvance, a new list, the fast path invalidated. Commands stay in order, so the picture should not change.

**The ring columns over Kameo's hands are not one of these.** They are cracked-ice decals (a 128x128 BC2 texture) she leaves on the wall, and they are in a hardware video too. Column contrast in the blue channel: hardware 10.1 and 8.0, ours 14.3 and 11.3, ours after blur and JPEG 9.3 and 7.5, at nearly the same x. That is one frame and a simulated codec, so it suggests rather than proves they match.

**The second pass, at the user's request to match RePowered wherever nothing conflicts (rc18, 2026-11-04).** All from its MIT source, with keys. None has been confirmed on the console yet.

| Key | What changed |
|---|---|
| `banjo_renut_last_pixel_shader` | My rc11 rule was incomplete. RePowered also keeps a null shader during the game's shadow pass (GeneratingShadowMap: 0x82B70B8F in the original, 0x82BCE10D in the update, set by 8226FC90 and cleared by 8226FF60). The flag is read on the game's thread and travels with the draw (`DrawCall::shadow_pass`). Without it, coloured patches appeared on the ground and changed with the player's form. That they are shadow-volume stencil polygons is inferred from RePowered's `renderShadowVolumeRecord` and from their colours following the last shader |
| `banjo_renut_dxn_swap` | The two BC4 halves of each DXN block swapped on upload (`ReadLevel`), which RePowered says otherwise transposes X and Y of every normal map. I recalled that Xenia doesn't do this, unchecked, so there is still no independent evidence: judge it on hardware footage |
| `banjo_renut_fmv_orb_skip` | The white disk over the FMV orb isn't drawn: PS 3547764B6B934FEE with 192 indices. RePowered also checks 65 vertices, which ours doesn't know at that point. It's a workaround with no known cause, and the scene hasn't been reached |

**Compared and matching already:** sampler LOD bias and border colours, alpha to coverage off at one sample, polygon offset, ring-written draws (`drawTube` and the inlined packet writers), and cube face layout. **Left out because they change the picture or cost too much on the console:** bilinear shadow filtering, bigger shadow maps, MSAA, widescreen, HUD layout and stereo. **Left out because they belong to its renderer's design:** draw merging, stale offscreen passes and the surface routing.

**Open, nothing changed:**
- **A resolve with no rectangle.** RePowered says the guest means the current viewport. The update's own D3DDevice_Resolve (820C5028) says otherwise: a null rectangle becomes (0,0) to the destination's size, and it reads no viewport word. Ours already does that, so nothing changed. If imposters still show the wrong model after the fix below, look here again.
- **Blue and pink dots on the same walls as the orange ones.** Not identified. They may be glows, in which case the occlusion fix covers them, or the sprites rc10 made visible.
- **The glitter around the teleport pad (fairy ring).** Heavy white hexagons and rings on the emulator. The user says this is emulator-only, so it isn't being fixed.
- **Grass edges.** RePowered multisamples, which lets alpha-to-mask foliage cover partial pixels. The console has one sample. A cost decision.
- **The wet lens.** RePowered replaces a vertex shader that draws window-pixel quads with the viewport off. Not checked here.

**Already right in ours, by reading:** half-pixel centres (`banjo_renut_half_pixel_offset`), per-view swizzles, signed formats, stacked textures as arrays, empty fetch slots as null textures, BeginVertices draws, the gamma ramp, volume fetches of 2D textures (`banjo_renut_fetch3d_on_2d`, from RePowered).

## Enhancements audit (2026-11-02)

`tools/enhancement_gap.py` and `tools/enhancement_state.py` against banjo-nx's `enhancements-reference.md`: 260 keys there, 245 defined here; of the 139 rows of "Every key", 135 ship as the reference does.

| Key | State | Why |
|---|---|---|
| `banjo_native_reverb` | off here, on in the reference | N&B's addresses; needs Kameo's XAudio functions. Only worth finding if a profile shows the audio thread high |
| `banjo_cpu_native_xaudio_src_m03` | absent | the same |
| `banjo_cpu_native_memcpy`, `_memset`, `_constants_*`, `_flush_*` | absent | N&B's hand natives; this port's hot lists fill from its own profiles |
| `banjo_renut_check_worker` | absent | removed from banjo-nx itself in package 28b (`banjo_renut_checks.h`); the reference is stale |
| `banjo_gpu_fetch_spec` | absent | tessellated draws only; no model or level in the game or its DLC uses the tessellated draw command (see "Tessellation" below). 0 tessellated draws in Thorn's Castle (`tessellation: 0 grid vertices`). Worth porting if a later level shows any |
| `banjo_renut_replay_lookahead`, `_shadow_sort`, `_texture_slot_skip`, `_inline_line_compare` | off | SLOWER ON THE CONSOLE, section 6b |

Picture-changing keys set off for this port (N&B's choices, pixel-identical by default): `banjo_renut_smaa`, `banjo_renut_fur_shells_percent` 100, `banjo_renut_portrait_fur_shells_percent` 100, `banjo_renut_portrait_msaa_samples` 0.
