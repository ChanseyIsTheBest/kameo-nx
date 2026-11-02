#!/usr/bin/env python3
"""One deko3d GLSL shader through uam with specialization constants, as the run-time build does (kameo-nx, 2026-11-03).

    python tools/uam_variant.py <shader.glsl> <out_dir> [id=value ...]

Puts "#define SPIRV_CROSS_CONSTANT_ID_<id> <value>u" after the #version line (plume_deko3d.cpp's form) and runs uam, keeping
its TGSI and raw Maxwell code. Prints the constant registers the TGSI reads, which shows what the compile kept.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UAM = os.path.join(ROOT, 'out', 'deko3d_tools', 'uam-build', 'uam.exe')


def build(source, out_dir, pairs, name=None):
    text = open(source, encoding='utf-8', errors='replace').read()
    defines = ''.join(f'#define SPIRV_CROSS_CONSTANT_ID_{i} {v}u\n' for i, v in pairs)
    head, _, rest = text.partition('\n')
    stem = name or (os.path.basename(source).split('.')[0] + ''.join(f'_{i}-{v}' for i, v in pairs))
    os.makedirs(out_dir, exist_ok=True)
    glsl = os.path.join(out_dir, stem + '.glsl')
    open(glsl, 'w', encoding='utf-8', newline='\n').write(head + '\n' + defines + rest)
    stage = 'frag' if os.path.basename(source).startswith('ps_') else 'vert'
    tgsi = os.path.join(out_dir, stem + '.tgsi')
    raw = os.path.join(out_dir, stem + '.raw')
    r = subprocess.run([UAM, '-s', stage, '-o', os.path.join(out_dir, stem + '.dksh'), '-t', tgsi, '-r', raw, glsl],
                       capture_output=True, text=True, timeout=240)
    return r.returncode, (r.stdout + r.stderr).strip(), tgsi, raw


def main():
    source, out_dir = sys.argv[1], sys.argv[2]
    pairs = [p.split('=') for p in sys.argv[3:]]
    code, log, tgsi, raw = build(source, out_dir, pairs)
    print('uam exit', code, log[:400])
    if code == 0:
        text = open(tgsi, errors='replace').read()
        print('tgsi lines', text.count('\n'), 'raw bytes', os.path.getsize(raw))
        print('samples:', re.findall(r'^\s*\d+:\s*(TEX\S*|TXB\S*|TXL\S*)[^\n]*CUBE', text, re.M))


if __name__ == '__main__':
    main()
