#!/usr/bin/env python3
"""kameo-nx (2026-11-04, the user's request: only the player's options in the toml, everything else hardcoded): writes the
package's tomls from app/kameo_switch.toml, the player's file. The built-in settings are app/kameo_switch_defaults.toml.

  renut.toml       the player's file as it is: what the package ships as kameo.toml;
  renut_play.toml  the same with the log on, for a bug report;
  renut_ab.toml    logs and diagnostics, a fixed resolution, an in-run A/B list with 30 s halves, target 1 (a Vulkan-only
                   key: does nothing on the Switch) as the control;
  configs/*.toml   test configs and options, each as few keys as it can be away from the player's file.

Configs that measure keep the conditions of the runs before them: dynamic resolution off, no frame cap, docked at 1080.

  python tools/make_kameo_tomls.py
"""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'dist', 'switch', 'kameo-nx')

LOG = {'log_level': '"info"'}
# What the shipped toml had on up to rc19: the log, the render thread's reports and the sampling profile's stacks.
DIAG = {
    **LOG,
    'banjo_renut_diag_resolve_trace': 'true',
    'banjo_renut_diag_gpu_shaders': 'true',
    'banjo_renut_diag_terrain': 'true',
    'banjo_shader_misses': '"sdmc:/switch/kameo-nx/shader_misses"',
    'banjo_stall_watch_seconds': '1',
    'renut_gpu_profile': '10.0',
    'switch_profile_stacks': 'true',
}
FIXED = {
    'banjo_renut_dynamic_resolution': 'false',
    'frame_cap': '"Off"',
    'banjo_renut_render_height_docked': '1080',
}
GPU_ROUND_OFF = {k: 'false' for k in (
    'banjo_deko3d_color_handover', 'banjo_deko3d_scanout_handover', 'banjo_deko3d_depth_handover_sampled',
    'banjo_deko3d_depth_read_layout', 'banjo_renut_gamma_upload_once', 'banjo_renut_present_no_pow',
    'banjo_renut_present_texel_load', 'banjo_deko3d_gpr32', 'banjo_deko3d_resolve_keeps_target')}
AUDIT_OFF = {
    **{k: 'false' for k in (
        'banjo_renut_last_pixel_shader', 'banjo_renut_poly_offset', 'banjo_renut_cull_both', 'banjo_renut_real_occlusion',
        'banjo_renut_resolve_clears_both', 'banjo_renut_dxn_swap', 'banjo_renut_fmv_orb_skip', 'banjo_renut_const_nan_flush')},
    'banjo_renut_whole_over_cap_mb': '0',
}
VERIFY = {
    'banjo_hot_guard_calls': '4000',
    'banjo_hot_guard_period': '64',
    **{k: 'true' for k in (
        'banjo_renut_fast_draw_verify', 'banjo_renut_resource_objects_verify', 'banjo_renut_fetch_cache_verify',
        'banjo_renut_whole_buffers_verify', 'banjo_renut_persistent_shared_verify', 'banjo_renut_chain_reuse_verify',
        'banjo_renut_snapshot_deltas_verify', 'banjo_renut_store_cards_verify', 'banjo_renut_record_extents_verify')},
}

# file: (what it is, the keys it changes)
CONFIGS = {
    'renut.toml': (None, {}),
    'renut_play.toml': ('the same with the log on, for a bug report', LOG),
    'renut_ab.toml': ('in-run A/B, 30 s halves, target 1 is the do-nothing control',
                      {**DIAG, **FIXED, 'banjo_renut_ab_targets': '"72,71,25,28,22,33,42,70,1"', 'banjo_renut_ab_s': '30'}),
    'configs/renut_profile.toml': ('the console profile: the log, diagnostics and the CPU profiler',
                                   {**DIAG, **FIXED, 'banjo_cpu_profiler': 'true'}),
    'configs/renut_verify.toml': ('the checks: slow, a test', {**DIAG, **FIXED, **VERIFY}),
    'configs/renut_pgo.toml': ('for a PGO instrumented build', FIXED),
    'configs/renut_no_dynamic.toml': ('a fixed 3D height', {**LOG, 'banjo_renut_dynamic_resolution': 'false'}),
    'configs/renut_648_864.toml': ('fixed 3D heights of 648 and 864',
                                   {**DIAG, **FIXED, 'banjo_renut_render_height_handheld': '648',
                                    'banjo_renut_render_height_docked': '864'}),
    'configs/renut_720_docked.toml': ('docked at a fixed 720',
                                      {**DIAG, **FIXED, 'banjo_renut_render_height_docked': '720'}),
    'configs/renut_smaa.toml': ('SMAA on: changes the picture', {**LOG, 'banjo_renut_smaa': 'true'}),
    'configs/renut_nan_flush_off.toml': ('without the fix for the coloured ground patches',
                                         {**LOG, 'banjo_renut_const_nan_flush': 'false'}),
    'configs/renut_no_gpu_round.toml': ('without the GPU round of 2026-11-03', {**LOG, **GPU_ROUND_OFF}),
    'configs/renut_audit_off.toml': ('without the renderer fixes matched to RePowered', {**LOG, **AUDIT_OFF}),
}


def make(base, what, keys):
    text = base
    if what:
        text = f'# kameo-nx test config: {what}. Copy over kameo.toml.\n' + text
    extra = []
    for key, value in keys.items():
        pattern = re.compile(rf'^(# )?{re.escape(key)} = .*$', re.M)
        if pattern.search(text):
            text = pattern.sub(f'{key} = {value}', text, count=1)
        else:
            extra.append(f'{key} = {value}')
    if extra:
        # Before the first section: a key after it would belong to it.
        marker = '# Button maps:'
        assert marker in text
        text = text.replace(marker, '# Test settings\n' + '\n'.join(extra) + '\n\n' + marker, 1)
    return text


def main():
    with open(os.path.join(ROOT, 'app', 'kameo_switch.toml'), encoding='utf-8', newline='') as f:
        base = f.read().replace('\r\n', '\n')
    os.makedirs(os.path.join(OUT, 'configs'), exist_ok=True)
    keep = {os.path.basename(name) for name in CONFIGS if name.startswith('configs/')}
    for name in os.listdir(os.path.join(OUT, 'configs')):
        if name.endswith('.toml') and name not in keep:
            os.remove(os.path.join(OUT, 'configs', name))
            print('removed configs/' + name)
    for name, (what, keys) in CONFIGS.items():
        with open(os.path.join(OUT, name), 'w', encoding='utf-8', newline='\n') as f:
            f.write(make(base, what, keys))
    print(len(CONFIGS), 'tomls written to', OUT)


if __name__ == '__main__':
    main()
