#!/usr/bin/env python3
"""kameo-nx (2026-11-02): checks a list of function starts against the image instead of trusting it. For each address: the
word before it (a terminator or padding means it can be a start; anything else runs into it), its first instructions,
and what reaches it in the image: direct branches (bl / b / bc), and pointers (aligned data words, lis + addi pairs).

  python -I tools/check_func_hints.py [--image out/renut_image.bin] 0x82324118 ...
  python -I tools/check_func_hints.py --toml <file with "0x8XXXXXXX" = { } lines>
"""
import argparse
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import ppc_dis  # noqa: E402

BASE = 0x82000000


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--image', default=os.path.join(ROOT, 'out', 'renut_image.bin'))
    ap.add_argument('--toml')
    ap.add_argument('addrs', nargs='*')
    a = ap.parse_args()
    d = open(a.image, 'rb').read()
    words = struct.unpack('>%dI' % (len(d) // 4), d[:len(d) // 4 * 4])
    want = [int(x, 16) for x in a.addrs]
    if a.toml:
        want += [int(m, 16) for m in re.findall(r'^"?(0x8[0-9A-Fa-f]{7})"?\s*=', open(a.toml, encoding='utf-8').read(), re.M)]
    wants = set(want)
    br = {w: [] for w in want}
    dat = {w: [] for w in want}
    pend = {}
    for i, w in enumerate(words):
        pc = BASE + i * 4
        op = w >> 26
        if op == 18:
            t = (w & 0x03FFFFFC)
            if t & 0x02000000:
                t -= 0x04000000
            if not w & 2:
                t += pc
            t &= 0xFFFFFFFF
            if t in wants:
                br[t].append(('bl' if w & 1 else 'b', pc))
        elif op == 16:
            t = (w & 0xFFFC)
            if t & 0x8000:
                t -= 0x10000
            if not w & 2:
                t += pc
            t &= 0xFFFFFFFF
            if t in wants:
                br[t].append(('bc', pc))
        elif op == 15 and (w >> 16) & 31 == 0:  # lis
            pend[(w >> 21) & 31] = (pc, (w & 0xFFFF) << 16)
        elif op == 14:  # addi
            ra = (w >> 16) & 31
            if ra in pend and pc - pend[ra][0] < 0x400:
                v = pend[ra][1] + (((w & 0xFFFF) ^ 0x8000) - 0x8000)
                if (v & 0xFFFFFFFF) in wants:
                    br[v & 0xFFFFFFFF].append(('lis/addi', pc))
        if w in wants:
            dat[w].append(pc)
    term = {0x4E800020, 0x4E800420, 0}
    for x in want:
        i = (x - BASE) // 4
        prev = words[i - 1]
        before = 'PAD' if prev in term or (prev >> 26) == 18 and not (prev & 1) else f'FALL({prev:08X})'
        print(f'{x:08X} {before} refs: {br[x][:6]} data: {[hex(p) for p in dat[x][:4]]}')


if __name__ == '__main__':
    main()
