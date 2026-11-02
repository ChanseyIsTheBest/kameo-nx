#!/usr/bin/env python3
"""kameo-nx (2026-11-02): which functions use a displacement (a D-form load or store, or an addi), to find a device field's
users in the image. `--hist LO HI` instead prints every displacement in [LO, HI) used in the code range with its count, to
find holes in a structure. With no --lo / --hi the whole code range is searched.

  python -I tools/d3d_disp.py 12536 12552 [--lo 0x820C0000 --hi 0x820DC000]
  python -I tools/d3d_disp.py --hist 0x2C00 0x2CC0 [--lo ... --hi ...]
"""
import argparse
import bisect
import glob
import os
import re
import struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = 0x82000000
OPS = {32: 'lwz', 33: 'lwzu', 34: 'lbz', 35: 'lbzu', 36: 'stw', 37: 'stwu', 38: 'stb', 39: 'stbu', 40: 'lhz', 41: 'lhzu',
       42: 'lha', 44: 'sth', 45: 'sthu', 48: 'lfs', 50: 'lfd', 52: 'stfs', 54: 'stfd', 14: 'addi', 58: 'ld', 62: 'std'}


def starts():
    s = set()
    for path in glob.glob(os.path.join(ROOT, 'app', 'generated', 'renut_recomp.*.cpp')):
        for m in re.finditer(r'^(?:DEFINE_REX_FUNC|REX_WEAK_FUNC|PPC_FUNC_IMPL)\(sub_([0-9A-F]+)\)', open(path, errors='replace').read(), re.M):
            s.add(int(m.group(1), 16))
    return sorted(s)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('disp', nargs='*')
    ap.add_argument('--hist', nargs=2)
    ap.add_argument('--lo', default='0x820B0000')
    ap.add_argument('--hi', default='0x82718000')
    ap.add_argument('--image', default=os.path.join(ROOT, 'out', 'renut_image.bin'))
    a = ap.parse_args()
    d = open(a.image, 'rb').read()
    lo, hi = int(a.lo, 16), int(a.hi, 16)
    fs = starts()
    if a.hist:
        h0, h1 = int(a.hist[0], 0), int(a.hist[1], 0)
        counts = {}
        for pc in range(lo, hi, 4):
            w = struct.unpack_from('>I', d, pc - BASE)[0]
            op = w >> 26
            if op in OPS:
                imm = w & 0xFFFF
                if op in (58, 62):
                    imm &= 0xFFFC
                if h0 <= imm < h1:
                    counts.setdefault(imm, {}).setdefault(OPS[op], 0)
                    counts[imm][OPS[op]] += 1
        for k in sorted(counts):
            print(f'{k:#06x} {k:6d} {counts[k]}')
        return
    want = {int(x, 0) for x in a.disp}
    out = {}
    for pc in range(lo, hi, 4):
        w = struct.unpack_from('>I', d, pc - BASE)[0]
        op = w >> 26
        if op in OPS:
            imm = w & 0xFFFF
            if op in (58, 62):
                imm &= 0xFFFC
            if imm in want:
                i = bisect.bisect_right(fs, pc) - 1
                f = fs[i] if i >= 0 else 0
                out.setdefault(f, []).append(f'{pc:08X} {OPS[op]} r{(w >> 21) & 31},{imm}(r{(w >> 16) & 31})')
    for f in sorted(out):
        print(f'sub_{f:08X}: ' + '; '.join(out[f][:6]))


if __name__ == '__main__':
    main()
