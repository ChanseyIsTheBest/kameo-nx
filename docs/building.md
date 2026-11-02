# Building kameo-nx

You need your own copy of the game: the code generator and the shader tools read it. Nothing made from it is committed
(`.gitignore`): `app/assets`, `app/generated`, `out/`, `app/out/`.

## Tools

Windows; LLVM clang and the Visual Studio Build Tools (CMake and Ninja) for the PC build (`tools/pc_env.bat` names the
paths); devkitA64, libnx and deko3d from [devkitPro](https://devkitpro.org) for the Switch; Python 3 with `xxhash` and
`Pillow`. The deko3d host tools (`out/deko3d_tools`: SPIRV-Cross and the PC uam) come from `tools/deko3d_build_tools.sh`.

## 1. The game

Extract the disc (`tools/extract_iso.py`) and title update 2 (`tools/stfs_extract.py`): the update's `default.xexp` goes next
to `default.xex`, its language folders in an `update` folder. Make `app/assets` point at the folder that holds `default.xex`.
Code generation takes the patched image.

## 2. The PC build: code generation and the shader translator

```bat
tools\build_host.bat
tools\build_pc.bat
```

The PC build runs the code generator (`app/renut_manifest.toml`), the post-codegen passes (`tools/post_codegen.py`) and
Renut's XenosRecomp over `out/renut_shader_dump`. Run it with `tools/run_pc_renut.ps1`, or `tools/run_pc.ps1` with the
emulated GPU for the reference image.

## 3. The shaders

```bash
python tools/renut_shader_dump.py --image out/renut_image.bin --out out/renut_shader_dump
python tools/kameo_files.py --game <game folder> --out out/renut_shader_dump --report out/kameo_shader_files.txt
```

The first takes the containers stored in the executable (147), the second the 4,477 in the `.lvl` and `.mdl` CAFF archives
(`--dlc <folder>` adds installed DLC's). Then:

```bat
powershell -File tools\switch_shaders.ps1
```

`switch_shaders.ps1` translates all of them to GLSL and makes the everyday build's files (`app/out/renut_switch`).
`BANJO_CENTROID=0` (or `switch_shaders.ps1 -NoCentroid`) translates without centroid sampling.

## 4. The Switch build

The everyday build (`app/out/switch`, no LTO):

```bat
tools\build_switch.bat
```

PGO and LTO come last, on the user's word, in folders of their own (`BUILD_DIR` and `CMAKE_EXTRA` of
`tools/build_switch.bat`):

1. **Instrumented:** `-DBANJO_PGO=generate -DBANJO_SWITCH_LTO=ON`. Play it: it writes `pgo/banjo_pgo_<N>.stream` next to
   the NRO every 2 minutes, at HOME and at exit.
2. **Merge** on the PC that built it (the streams name its folders): `python tools/pgo_merge.py <folder with the
   streams>` fills `pgo/` (it stops if `pgo/` already has `.gcda` files: move them aside first).
3. **Optimised:** `-DBANJO_PGO=use -DBANJO_SWITCH_LTO=ON -DBANJO_RENUT_RELEASE_SHADERS=ON`. A function changed since the
   instrumented build keeps no profile.

Keep the temporary folder's path free of spaces (GCC's parallel LTO link writes a makefile with it unquoted), and never
run two of these builds at once: they share the SDK's archives in `sdk/out/switch-arm64`.

The fixed settings are `app/kameo_switch_defaults.toml`, compiled in; the player's are `app/kameo_switch.toml`, written next
to the NRO as `kameo.toml` when it is missing. A key added to `kameo.toml` overrides a fixed one (the test runs use that:
`tools/run_ryujinx.ps1 -Toml <file>`).
