#!/usr/bin/env bash
# tools/switch_shaders.ps1 for a Linux host (kameo-nx, 2026-11-04): the Switch build's shader inputs from the PC build's
# XenosRecomp, the same steps in the same order. PYTHON names the interpreter (xxhash and Pillow installed); set
# BANJO_CENTROID=0 for the no-centroid folders, as -NoCentroid does.
#   bash tools/switch_shaders.sh
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PY="${PYTHON:-python3}"
EXE="$ROOT/app/out/pc/xenos_recomp/build/XenosRecomp/XenosRecomp"
COMMON="$ROOT/app/thirdparty/reblue-XenosRecomp/XenosRecomp/shader_common.h"
DUMP="$ROOT/out/renut_shader_dump"
if [ "${BANJO_CENTROID:-1}" = "0" ]; then
    WORK="$ROOT/out/deko3d_shaders_nocentroid"; PKG="$ROOT/app/out/renut_switch_nocentroid"; CACHE=0
else
    unset BANJO_CENTROID; WORK="$ROOT/out/deko3d_shaders"; PKG="$ROOT/app/out/renut_switch"; CACHE=1
fi
EARLY="$WORK/early_alpha_hlsl"; SPIRV="$WORK/spirv"
[ -x "$EXE" ] || { echo "no $EXE: build the PC app with Renut first" >&2; exit 1; }
rm -rf "$EARLY" "$SPIRV"; mkdir -p "$EARLY" "$SPIRV"
BANJO_HLSL_DUMP_ONLY=1 "$EXE" "$DUMP" "$WORK/early_alpha_pass1.cpp" "$COMMON" "$EARLY"
"$PY" "$ROOT/tools/renut_early_alpha.py" "$EARLY"
BANJO_HLSL_OVERRIDE="$EARLY" BANJO_DEKO3D_SPIRV_DIR="$SPIRV" "$EXE" "$DUMP" "$WORK/shader_cache_deko3d.cpp" "$COMMON"
cd "$ROOT"
"$PY" tools/deko3d_shaders.py --spirv "$SPIRV" --out "$WORK"
"$PY" tools/deko3d_glsl_opt.py --in "$WORK/glsl" --out "$WORK/glsl_opt"
"$PY" tools/deko3d_shader_package.py --glsl "$WORK/glsl_opt" --out "$PKG"
[ "$CACHE" = 1 ] && "$PY" tools/renut_switch_shader_cache.py --pc app/out/pc
ls -l "$PKG"
