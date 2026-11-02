#!/usr/bin/env python3
"""kameo-nx (2026-11-02): a contact sheet of a run's captures, for looking at a run at a glance.

  python -I tools/sheet.py <folder or png ...> --out sheet.jpg [--width 426] [--cols 3] [--last N]
"""
import argparse
import glob
import os

from PIL import Image, ImageDraw


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('paths', nargs='+')
    ap.add_argument('--out', required=True)
    ap.add_argument('--width', type=int, default=426)
    ap.add_argument('--cols', type=int, default=3)
    ap.add_argument('--last', type=int, default=0)
    a = ap.parse_args()
    files = []
    for p in a.paths:
        files += sorted(glob.glob(os.path.join(p, '*.png'))) if os.path.isdir(p) else [p]
    if a.last:
        files = files[-a.last:]
    if not files:
        raise SystemExit('no captures')
    first = Image.open(files[0])
    h = a.width * first.height // first.width
    rows = (len(files) + a.cols - 1) // a.cols
    sheet = Image.new('RGB', (a.width * a.cols, (h + 14) * rows), (32, 32, 32))
    draw = ImageDraw.Draw(sheet)
    for i, f in enumerate(files):
        im = Image.open(f).convert('RGB').resize((a.width, h))
        x, y = (i % a.cols) * a.width, (i // a.cols) * (h + 14)
        sheet.paste(im, (x, y + 14))
        draw.text((x + 2, y + 1), os.path.basename(f)[:60], fill=(255, 255, 0))
    sheet.save(a.out, quality=80)
    print(a.out, sheet.size, len(files), 'captures')


if __name__ == '__main__':
    main()
