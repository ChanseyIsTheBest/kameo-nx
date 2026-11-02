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
# kameo-nx: A presses alone, so no timing can send them down the wrong page: A takes the title, Solo, Normal, New Quest and
# the opening scene. New Quest's keyboard dialog would wait for ever (it is not in the captures: the book stays blank);
# `headless` answers prompts with their defaults. On a clock of the game's own input polls (input_script_polls: emu2 lost a
# press in a 46 s host stall). A DOWN to "Start without saving" kept landing on the wrong page (chain1-3, emu7).
PAD = ('input_script = "' + ','.join(f'{t}:A@0.5' for t in range(40, 300, 10)) + '"\n'
       'input_script_polls = 60\ninput_script_sticks = 320\nheadless = true\n')
NO_PAD = 'input_script = ""\ninput_script_sticks = 0\n'
# Unbuffered: a run is stopped by force, which drops a buffered log's tail (emu4prof's last 3 minutes).
PAD += 'log_file_writer = false\n'
CAPTURES = 'nfsmw_captura_cada_s = 15\nnfsmw_captura_max = 60\n'
TESTS = {
    'emu_verify.toml': (os.path.join('configs', 'renut_verify.toml'), PAD + CAPTURES),
    'emu_attract.toml': (os.path.join('configs', 'renut_verify.toml'), NO_PAD + CAPTURES),
    'emu_profile.toml': ('renut_play.toml', PAD + CAPTURES + 'switch_profile_stacks = true\n'),
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
