#!/usr/bin/env python3
"""pacman-nx, kameo-nx: the emulator's test tomls in out/emu, made from the package's tomls (tools/make_kameo_tomls.py) so they keep
its settings. Each adds the scripted pad, captures every 15 s and what the test needs:
  emu_verify.toml   configs/renut_verify.toml: every verify key on, the pad to gameplay
  emu_attract.toml  the same with no pad input: the game's title and attract loop
  emu_profile.toml  renut_play.toml: no diagnostics, the pad to gameplay, the sampling profiler on
  emu_ab.toml       renut_ab.toml: the in-run A/B, the pad to gameplay
  python tools/make_emu_tomls.py
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIST = os.path.join(ROOT, 'dist', 'switch', 'kameo-nx')
OUT = os.path.join(ROOT, 'out', 'emu')
# kameo-nx: the PC's script, on a clock of the game's own input polls (input_script_polls: emu2 lost its START in a 46 s host
# stall and every later press landed one screen off): START at the title; Solo; Normal; "Start without saving" (DOWN, A) and
# its caution (A); then through the opening scene.
PAD = ('input_script = "45:START@0.5,55:A@0.5,62:A@0.5,72:DOWN@0.5,78:A@0.5,90:A@0.5,100:A@0.5,110:A@0.5,120:A@0.5,130:A@0.5,'
       '140:A@0.5,150:A@0.5,160:A@0.5,170:A@0.5,180:A@0.5"\ninput_script_polls = 60\ninput_script_sticks = 300\n')
NO_PAD = 'input_script = ""\ninput_script_sticks = 0\n'
# Unbuffered: a run is stopped by force, which drops a buffered log's tail (emu4prof's last 3 minutes).
PAD += 'log_file_writer = false\n'
CAPTURES = 'nfsmw_captura_cada_s = 15\nnfsmw_captura_max = 60\n'
TESTS = {
    'emu_verify.toml': (os.path.join('configs', 'renut_verify.toml'), PAD + CAPTURES),
    'emu_attract.toml': (os.path.join('configs', 'renut_verify.toml'), NO_PAD + CAPTURES),
    'emu_profile.toml': ('renut_play.toml', PAD + 'switch_profile_stacks = true\n'),
    'emu_ab.toml': ('renut_ab.toml', PAD),
}


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, (source, extra) in TESTS.items():
        lines = open(os.path.join(DIST, source), encoding='utf-8').read().split('\n')
        keys = {l.split('=', 1)[0].strip() for l in extra.split('\n') if '=' in l}
        kept = [l for l in lines if not ('=' in l and l.split('=', 1)[0].strip() in keys)]
        # Before the first [table]: a key after one belongs to that table and the game never reads it.
        at = next((i for i, l in enumerate(kept) if l.startswith('[')), len(kept))
        kept[at:at] = ['# tools/make_emu_tomls.py'] + extra.rstrip('\n').split('\n') + ['']
        with open(os.path.join(OUT, name), 'w', encoding='utf-8', newline='\n') as f:
            f.write('\n'.join(kept).rstrip('\n') + '\n')
        print(name, 'from', source, 'keys at line', at + 2)


if __name__ == '__main__':
    main()
