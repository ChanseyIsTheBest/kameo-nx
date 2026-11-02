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

## Time limit (2026-11-02, 22:58)

A message from the Pac-Man port's session, quoting the user, moved the time limit from 06:00 to 11:00 AEST. The console package is still due at 06:00 as first asked; the extra hours go on what is left after it.

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
| 1 | Our picture is darker in the midtones everywhere (menus and gameplay) | the display's gamma ramp: `SetGammaRamp` (`sub_820C48B8`) stores the converted ramp at device `+0x36EC`; our Swap still read N&B's offset, and `banjo_renut_game_gamma` was off. RePowered records the same difference (`kameo_gfx_gamma`: median 30 against the reference's 55) | fix ready, our own key, no graft |
| 2 | The sky through the castle window is blown out to white with a wide halo; in the reference it is a muted orange | to be measured with `renut_post_process_precision = "Game"`: the PC default keeps bloom buffers at 16 bits, the console's are 8 | testing |
| 3 | Thin chains above the player in ours, not visible in the reference | not looked at yet | open |

The reference fakes occlusion query results too (`query_occlusion_fake_sample_count`, as our hook's 1000), so RePowered's real occlusion counts for light glows fix no difference from the reference. They go in the report as a decision.

## Enhancements audit (2026-11-02)

`tools/enhancement_gap.py` and `tools/enhancement_state.py` against banjo-nx's `enhancements-reference.md`: 260 keys there, 245 defined here; of the 139 rows of "Every key", 135 ship as the reference does.

| Key | State | Why |
|---|---|---|
| `banjo_native_reverb` | off here, on in the reference | N&B's addresses; needs Kameo's XAudio functions. Only worth finding if a profile shows the audio thread high |
| `banjo_cpu_native_xaudio_src_m03` | absent | the same |
| `banjo_cpu_native_memcpy`, `_memset`, `_constants_*`, `_flush_*` | absent | N&B's hand natives; this port's hot lists fill from its own profiles |
| `banjo_renut_check_worker` | absent | removed from banjo-nx itself in package 28b (`banjo_renut_checks.h`); the reference is stale |
| `banjo_gpu_fetch_spec` | absent | tessellated draws only; 0 tessellated draws in Thorn's Castle (`tessellation: 0 grid vertices`). Worth porting if a later level shows any |
| `banjo_renut_replay_lookahead`, `_shadow_sort`, `_texture_slot_skip`, `_inline_line_compare` | off | SLOWER ON THE CONSOLE, section 6b |

Picture-changing keys set off for this port (N&B's choices, pixel-identical by default): `banjo_renut_smaa`, `banjo_renut_fur_shells_percent` 100, `banjo_renut_portrait_fur_shells_percent` 100, `banjo_renut_portrait_msaa_samples` 0.
