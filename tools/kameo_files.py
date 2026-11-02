#!/usr/bin/env python3
"""kameo-nx (2026-11-02): the shader containers in Kameo's data files. The executable holds 147; the game creates almost all of
its shaders from its levels and models (530 distinct in the first minute, none of them in the executable), which are Rare's
CAFF 28.01.05.0031 archives: each section one zlib stream (the format as Kameo: RePowered's tools/modkit/kameo_modkit/caff.py,
MaxDeadBear and BiRabittoh, MIT, reads it). So every section of every archive is unpacked and scanned for version 0x0E
containers, whole and valid by zuma-nx's container_0e.h rule.

  python tools/kameo_files.py --game D:/kameo/game --out out/renut_shader_dump [--report out/kameo_shader_files.txt]
                              [--dlc <folder of unpacked DLC>]
"""
import argparse
import os
import re
import struct
import sys
import zlib

import xxhash

MAGIC = re.compile(rb'\x10\x2a\x0e[\x00\x01]', re.S)
HEADER = 0x180


def sections(blob):
    """(name, unpacked bytes) of a CAFF archive."""
    if blob[:4] != b'CAFF':
        return
    cursor = struct.unpack_from('>I', blob, 0x38)[0]
    for i in range(blob[0x3D]):
        base = 0x40 + 40 * i
        name = blob[base:base + 8].rstrip(b'\0').decode('latin-1')
        values = struct.unpack_from('>8I', blob, base + 8)
        size, packed = values[2], values[7]
        if packed:
            yield name, zlib.decompress(blob[cursor:cursor + packed])
            cursor += packed


def containers(data):
    """(offset, bytes) of each valid container in data: any byte offset."""
    for m in MAGIC.finditer(data):
        o = m.start()
        if o + 0x18 > len(data):
            continue
        flags, virt, phys, defs, consts, info = struct.unpack_from('>6I', data, o)
        if not (0x18 <= virt < 0x10000 and 0 < phys < 0x100000 and o + virt + phys <= len(data)):
            continue
        if defs >= virt or consts >= virt or not 0x18 <= info < virt:
            continue
        yield o, data[o:o + virt + phys]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--game', required=True)
    ap.add_argument('--dlc', action='append', default=[])
    ap.add_argument('--out', required=True)
    ap.add_argument('--report')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    known = {n.split('_')[1].split('.')[0] for n in os.listdir(a.out) if n.endswith('.bin') and '_' in n}
    files = []
    for root in [a.game] + a.dlc:
        for d, _, names in os.walk(root):
            files += [os.path.join(d, n) for n in names if n.lower().endswith(('.lvl', '.mdl'))]
    files.sort()
    total, new, seen = 0, 0, {}
    report = []
    for k, path in enumerate(files):
        blob = open(path, 'rb').read()
        here = 0
        try:
            for name, data in sections(blob):
                for offset, shader in containers(data):
                    here += 1
                    digest = xxhash.xxh3_64_hexdigest(shader)
                    if digest in seen:
                        continue
                    seen[digest] = os.path.relpath(path, a.game)
                    total += 1
                    kind = 'vs' if shader[3] & 1 else 'ps'
                    if digest not in known:
                        open(os.path.join(a.out, f'{kind}_{digest}.bin'), 'wb').write(shader)
                        known.add(digest)
                        new += 1
                    report.append(f'{kind}_{digest} {seen[digest]} {name} +{offset:#x} {len(shader)}')
        except (zlib.error, struct.error) as e:
            print('skipped', path, e, file=sys.stderr)
        print(f'[{k + 1}/{len(files)}] {os.path.basename(path)}: {here} containers, {total} distinct so far, {new} new', flush=True)
    if a.report:
        open(a.report, 'w', encoding='utf-8').write('\n'.join(report) + '\n')
    print(f'{len(files)} archives: {total} distinct containers, {new} new in {a.out} ({len(known)} there)')


if __name__ == '__main__':
    main()
