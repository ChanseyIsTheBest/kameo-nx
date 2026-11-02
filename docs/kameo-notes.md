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

## Code generation (2026-11-02)

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
