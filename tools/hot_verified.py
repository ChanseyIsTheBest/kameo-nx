#!/usr/bin/env python3
"""kameo-nx (2026-11-03): the request: "Verify each against the generated code before it is on". A verify run
(tools/run_pc_verify.ps1) leaves app/out/pc/hot_unverified.txt: the copies with no call compared. This moves those lines of
app/src/cpu/hot/localized_bulk.txt into app/src/cpu/hot/localized_unverified.txt, so the build's copies are the verified ones and
the others are kept for a later session that reaches them (merge them back with --restore before that session's verify run).

  python tools/hot_verified.py [--dry-run]
  python tools/hot_verified.py --restore
"""
import argparse
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BULK = os.path.join(ROOT, 'app', 'src', 'cpu', 'hot', 'localized_bulk.txt')
HELD = os.path.join(ROOT, 'app', 'src', 'cpu', 'hot', 'localized_unverified.txt')
LIST = os.path.join(ROOT, 'app', 'out', 'pc', 'hot_unverified.txt')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--restore', action='store_true')
    a = ap.parse_args()
    lines = open(BULK, encoding='utf-8').read().split('\n')
    if a.restore:
        held = open(HELD, encoding='utf-8').read().split('\n') if os.path.isfile(HELD) else []
        held = [line for line in held if line.strip()]
        with open(BULK, 'w', encoding='utf-8', newline='\n') as f:
            f.write('\n'.join([line for line in lines if line.strip()] + held) + '\n')
        if os.path.isfile(HELD):
            os.remove(HELD)
        print(len(held), 'copies back in the bulk list')
        return
    names = {re.sub(r'^sub_', '', n.strip()).upper() for n in open(LIST, encoding='utf-8') if n.strip()}
    keep, out = [], []
    for line in lines:
        address = line.split(' ', 1)[0].upper()
        (out if address in names else keep).append(line)
    print(f'{len(names)} copies with no call compared; {len(out)} of them on the bulk list; {len(keep)} lines stay')
    if a.dry_run:
        for line in out[:40]:
            print('  ', line[:80])
        return
    with open(BULK, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(keep))
    with open(HELD, 'a', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
