#!/usr/bin/env python3
"""kameo-nx (2026-11-02): writes the package's tomls from katamari-nx's shipped renut.toml (itself from banjo-nx's), with
Kameo's settings:

  renut.toml       the shipped settings: Renut on deko3d, 720 lines handheld and 1080 docked (as katamari-nx ships; no
                   dynamic resolution), every exact enhancement as the base ships it, the items measured SLOWER ON THE
                   CONSOLE off, diagnostics as the base's renut.toml (logs on);
  renut_play.toml  renut.toml with the render-thread diagnostics off;
  renut_ab.toml    renut.toml with an in-run A/B list, 30 s halves, target 1 (a Vulkan-only key: does nothing on the
                   Switch) as the control.
  configs/*.toml   test configs and the picture-changing options, each one key away from renut.toml, for the user.

  python tools/make_kameo_tomls.py --base <katamari-nx renut.toml> --out dist/switch/kameo-nx
"""
import argparse
import os
import re

HEADER = """# kameo-nx (Kameo: Elements of Power on the Switch, Renut's renderer on deko3d): {what}
# Made by tools/make_kameo_tomls.py from katamari-nx's shipped renut.toml. Copy to sdmc:/switch/kameo-nx/renut.toml.
# Credits: reNut's renderer (masterspike52, reuse permitted), Kameo: RePowered (MaxDeadBear, BiRabittoh; MIT), katamari-nx,
# banjo-nx, sa2-nx, NetherSX2_nx (four-core layout), battd_nx (the GPU arena), bejeweled3-nx and the zuma ports.
"""

# Keys set for Kameo (the rest as katamari-nx ships them).
SHIPPED = {
    'banjo_renderer': '"renut"',
    'renut_gpu_backend': '"deko3d"',
    'banjo_renut_render_height_handheld': '720',
    'banjo_renut_render_height_docked': '1080',
    'banjo_renut_dynamic_resolution': 'false',
    'disable_msaa': 'false',
    'renut_shadow_resolution': '"1x"',
    'renut_ambient_occlusion': '"Half"',
    'renut_post_process_precision': '"Game"',
    # The game paces itself (its presentation interval); no extra cap.
    'frame_cap': '"Off"',
    'target_refreshRate': '-1',
    'masseffect_renderer_native': 'false',
    'gpu_plugin': '""',
    'banjo_forwarder_program_id': '""',
    # Off on Katamari's console measurement (exact either way: the same samples).
    'banjo_xma_frame_cache': 'false',
    'banjo_shader_misses': '"sdmc:/switch/kameo-nx/shader_misses"',
    # Pixel-identical by default (the request): N&B's choices that change the picture stay off. Each is a config below.
    'banjo_renut_smaa': 'false',
    'banjo_renut_fur_shells_percent': '100',
    'banjo_renut_portrait_fur_shells_percent': '100',
    'banjo_renut_portrait_msaa_samples': '0',
    # The display's gamma ramp the game loads: without it the picture is darker than the reference (docs/kameo-notes.md).
    'banjo_renut_game_gamma': 'true',
    # Kameo's level geometry: the PC's cache reached 234.5 MB in Thorn's Castle; at the base's 128 MB, emu4prof counted
    # "index cache full 14367" and the draws fell back to range copies converted every frame (under 2 fps). Exact either way.
    'banjo_renut_stream_cache_mb': '256',
}
PLAY = {'banjo_renut_diag_resolve_trace': 'false', 'banjo_renut_diag_gpu_shaders': 'false',
        'banjo_renut_diag_terrain': 'false'}
# The targets that act on Kameo's draws (all immediate, no command-buffer recordings: docs/kameo-notes.md's census): the
# fast draw (25), pipelines by content (28), whole-buffer copies (22), merged draws (33), mesh objects (42), the localized
# copies of the hot functions (70); 1 (a Vulkan-only key) is the do-nothing control on the Switch. The record-time items
# (45-50) have nothing to act on here, and target 26 (the setters' touch compare) neither: the game's own setters stay.
# 7 targets x 60 s = 7 minutes a round: 21+ minutes.
AB = {'banjo_renut_ab_targets': '"25,28,22,33,42,70,1"', 'banjo_renut_ab_s': '30',
      'banjo_renut_dynamic_resolution': 'false'}
VERIFY = {k: 'true' for k in (
    'banjo_renut_fast_draw_verify', 'banjo_renut_resource_objects_verify', 'banjo_renut_fetch_cache_verify',
    'banjo_renut_whole_buffers_verify', 'banjo_renut_persistent_shared_verify', 'banjo_renut_chain_reuse_verify',
    'banjo_renut_snapshot_deltas_verify', 'banjo_renut_store_cards_verify', 'banjo_renut_record_extents_verify')}
# The CPU natives' guard (banjo_hot_guest.cpp): each thread's first 4000 calls of every native compared with the
# generated original, then one in 64 ("[hot] DIFFERENCE" lines; the 10-s "[hot]" report counts the checks).
VERIFY.update({'banjo_hot_guard_calls': '4000', 'banjo_hot_guard_period': '64'})
# The PGO build's renut.toml: no log, no profilers, no diagnostics; rex_crash.log and rex_stderr.log stay.
PGO = {'log_level': '"off"', 'switch_profile_stacks': 'false', 'banjo_cpu_profiler': 'false',
       'banjo_show_profiler': 'false', 'banjo_show_fps': 'false', 'banjo_stall_watch_seconds': '0',
       'renut_gpu_profile': '0.0', 'banjo_renut_diag_resolve_trace': 'false', 'banjo_renut_diag_gpu_shaders': 'false',
       'banjo_renut_diag_terrain': 'false', 'banjo_hot_guard_calls': '0', 'banjo_hot_guard_period': '0'}
# A console profile: the sampling profiler's stacks on (optimising-functions.md); samples go to rex_profile.log.
PROFILE = {'switch_profile_stacks': 'true', 'banjo_cpu_profiler': 'true'}
CONFIGS = {
    'renut_verify.toml': (VERIFY, 'every Renut verify mode on (emulator / test runs: slow; the log\'s verify lines '
                                  'must count 0 differences)'),
    'renut_pgo.toml': (PGO, 'the shipped settings with only the crash logs (the PGO build\'s renut.toml)'),
    'renut_profile.toml': (PROFILE, 'the shipped settings with the console profile on (rex_profile.log)'),
    'renut_smaa.toml': ({'banjo_renut_smaa': 'true'}, 'SMAA on the final composite (not pixel-identical; it needs the '
                                                      'composite\'s pixel shader hash, banjo_renut_smaa_shaders)'),
    'renut_720_docked.toml': ({'banjo_renut_render_height_docked': '720'},
                              '720 lines docked too, the Xbox 360\'s own (pixel-identical)'),
    'renut_648_864.toml': ({'banjo_renut_render_height_handheld': '648', 'banjo_renut_render_height_docked': '864'},
                           'banjo-nx\'s internal resolution: 648 lines handheld, 864 docked (not pixel-identical)'),
    'renut_dynamic.toml': ({'banjo_renut_dynamic_resolution': 'true'},
                           'banjo-nx\'s dynamic resolution, 540-720 handheld / 720-1080 docked (not pixel-identical)'),
}

# What belongs to the base's game: stale addresses (banjo_cpu_native_sub_*, this port's come from its own lists) and
# Katamari's own keys.
DROP = re.compile(r'^\s*(banjo_cpu_native_sub_|banjo_cpu_native_xaudio_src|katamari_)[^\n]*\n', re.M)


def apply(text, keys):
    """Sets each key: in place where the base has it at the top level, else before the first [section] (banjo-nx's
    [input_p1] / [input_p2] stay last: a key after a table header belongs to that table)."""
    first_section = re.search(r'^\[', text, re.M)
    top_end = first_section.start() if first_section else len(text)
    for k, v in keys.items():
        pat = re.compile(r'^(\s*)' + re.escape(k) + r'\s*=.*$', re.M)
        m = pat.search(text, 0, top_end)
        if m:
            text = text[:m.start()] + f'{m.group(1)}{k} = {v}' + text[m.end():]
        else:
            text = text[:top_end].rstrip('\n') + f'\n{k} = {v}\n\n' + text[top_end:]
        first_section = re.search(r'^\[', text, re.M)
        top_end = first_section.start() if first_section else len(text)
    return text


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--base', required=True)
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    base = open(a.base, encoding='utf-8').read()
    base = base[base.index('# banjo-nx test config'):].replace('sdmc:/switch/katamari-nx/', 'sdmc:/switch/kameo-nx/')
    base = base.replace('sdmc:/switch/banjo-nx/', 'sdmc:/switch/kameo-nx/')
    base = DROP.sub('', base)
    # banjo-nx's note on its own title update.
    base = re.sub(r'^# Title update 3 \(2026-10-29\).*?^# renut_guard\.toml[^\n]*\n', '', base, flags=re.M | re.S)
    shipped = apply(base, SHIPPED)
    os.makedirs(os.path.join(a.out, 'configs'), exist_ok=True)
    outs = {
        'renut.toml': (shipped, 'the shipped settings (720 lines handheld, 1080 docked; diagnostics and logs on)'),
        'renut_play.toml': (apply(shipped, PLAY), 'renut.toml with the render-thread diagnostics off, for playing'),
        'renut_ab.toml': (apply(shipped, AB), 'renut.toml with an in-run A/B list, 30 s halves, target 1 as the '
                                              'control (a Vulkan-only key: nothing on the Switch)'),
    }
    for name, (cfg, what) in CONFIGS.items():
        outs[os.path.join('configs', name)] = (apply(shipped, cfg), what)
    for name, (text, what) in outs.items():
        with open(os.path.join(a.out, name), 'w', encoding='utf-8', newline='\n') as f:
            f.write(HEADER.format(what=what) + text)
        print(name)


if __name__ == '__main__':
    main()
