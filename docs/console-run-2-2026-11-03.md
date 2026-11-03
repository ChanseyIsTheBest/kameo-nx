# Second console run, 2026-11-03

Package `2026-11-03_1850_rc5_try`, NRO md5 54493c1588a4574374127a9b63facc2b, 999 copies, handheld, 720 lines, stock clocks. 18:54:32 to 18:57:13: 100 s of gameplay. The first run is `docs/console-run-2026-11-03.md`.

Not the same scene as the first run: the XMA worker, whose code did not change, did 4,769 contexts a second against 3,496, at 25.1 us each against 29.0. So per-draw figures across the two runs do not separate code from scene. `sub_82379E10` is listed as a machine copy but carries the hand edit of `tools/hand_copies.py`, and the memcpy, memset and constant natives are inlined into the copies that call them, so they have no line of their own.

gameplay reports 8 (28.2 fps), of them with the core-2 worker under 10 %: 8 (28.2 fps), over: 0 (0.0 fps)
CPU total mean: all 278 %, calm 278 %, compiling 0 % of 300 %

| Thread | Top function | Core mask | Priority | CPU gameplay | CPU no compile | CPU compiling | Peak |
|---|---|---|---|---:|---:|---:|---:|
| 2733 XThreadD7964EE0 | CaptureDelta | 0x7 (pref -1) | 0x3B | 95.3 | 95.3 | 0.0 | 98 |
| 2731 Main XThread (F | sub_8232D9F8 | 0x7 (pref -1) | 0x3B | 72.4 | 72.4 | 0.0 | 80 |
| 2735 ? | FastDrawLocked | 0x2 (pref 1) | 0x2C | 62.7 | 62.7 | 0.0 | 76 |
| 2727 Audio Worker (F | sub_8233F578 | 0x7 (pref -1) | 0x2B | 18.8 | 18.8 | 0.0 | 22 |
| 2726 XMA Decoder (F8 | Work | 0x7 (pref -1) | 0x2B | 16.8 | 16.8 | 0.0 | 20 |
| 2723 ? | canCommuteDefSrc | 0x4 (pref 2) | 0x3B | 7.0 | 7.0 | 0.0 | 12 |
| 2720 rex::thread::TimerQueue | TimerThreadMain | 0x7 (pref -1) | 0x3B | 1.6 | 1.6 | 0.0 | 2 |
| 2739 ? | FillAndAppend | 0x1 (pref 0) | 0x2B | 1.3 | 1.3 | 0.0 | 1 |
| 2531 main (UI) | BaseMode | 0x1 (pref 0) | 0x2C | 0.8 | 0.8 | 0.0 | 1 |
| 2746 XThread120F29C0 | Wait | 0x7 (pref -1) | 0x3B | 0.6 | 0.6 | 0.0 | 1 |
| 2730 ? | armDCacheClean | 0x4 (pref 2) | 0x3B | 0.2 | 0.2 | 0.0 | 0 |
| 2725 ? | __wrap_svcSleepThread | 0x7 (pref -1) | 0x3B | 0.1 | 0.1 | 0.0 | 0 |
| 2737 XThreadAA00EA0  | Wait | 0x7 (pref -1) | 0x3B | 0.1 | 0.1 | 0.0 | 0 |
| 2724 ? | - | 0x4 (pref 2) | 0x3B | 0.0 | 0.0 | 0.0 | 0 |

## Classes per thread, % of a core
| Thread | Hand | Machine | Not optimised | of which plain recompile | of which held back | of which libraries |
|---|---:|---:|---:|---:|---:|---:|
| 2733 XThreadD7964EE0 | 24.5 | 61.3 | 9.5 | 1.5 | 0.0 | 8.0 |
| 2731 Main XThread (F | 3.2 | 63.7 | 5.5 | 4.1 | 0.0 | 1.3 |
| 2735 ? | 57.3 | 0.0 | 5.4 | 0.0 | 0.0 | 5.4 |
| 2727 Audio Worker (F | 0.6 | 14.0 | 4.3 | 0.6 | 0.0 | 3.6 |
| 2726 XMA Decoder (F8 | 0.0 | 0.0 | 16.8 | 0.0 | 0.0 | 16.8 |
| 2723 ? | 0.0 | 0.0 | 7.0 | 0.0 | 0.0 | 7.0 |
| 2720 rex::thread::TimerQueue | 0.0 | 0.0 | 1.5 | 0.0 | 0.0 | 1.5 |
| 2739 ? | 0.0 | 0.0 | 1.2 | 0.0 | 0.0 | 1.2 |
| All threads | 85.9 | 139.2 | 52.2 | | | |
Share of all busy CPU: Hand 31.0 %, Machine 50.2 %, Not optimised 18.8 %
   50.2 %  generated copy (localize_native)
   26.2 %  renderer, hand-written
    7.6 %  system library
    4.1 %  SDK runtime
    3.2 %  app, hand-written
    2.9 %  no symbol
    2.3 %  plain recompile; not on any list
    1.9 %  shader compiler
    1.3 %  renderer back end, hand-written
    0.2 %  hook replacing a game function
    0.0 %  hand-written native

## Functions per thread

### 2733 XThreadD7964EE0: 95.3 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| CaptureDelta | queued.cpp.obj | 13.3 | 12.71 | 418 | Hand | renderer, hand-written |
| sub_820E7F10 | app | 10.6 | 10.08 | 332 | Machine | generated copy (localize_native) |
| sub_82379E10 | app | 6.7 | 6.37 | 209 | Machine | generated copy (localize_native) |
| sub_82110280 | app | 5.0 | 4.80 | 158 | Machine | generated copy (localize_native) |
| sub_8231C268 | app | 4.1 | 3.93 | 129 | Machine | generated copy (localize_native) |
| sub_82524E98 | app | 3.3 | 3.18 | 105 | Machine | generated copy (localize_native) |
| ? | ? | 3.1 | 2.98 | 98 | Not optimised | no symbol |
| Prepare | kameo_d3d.cpp.obj | 2.5 | 2.34 | 77 | Hand | renderer, hand-written |
| sub_8231B190 | app | 1.9 | 1.85 | 61 | Machine | generated copy (localize_native) |
| mutexLock | libnx.a | 1.6 | 1.52 | 50 | Not optimised | system library |
| sub_820EB9B0 | app | 1.6 | 1.49 | 49 | Machine | generated copy (localize_native) |
| sub_8225AC50 | app | 1.5 | 1.46 | 48 | Machine | generated copy (localize_native) |
| sub_820C4300 | app | 1.3 | 1.22 | 40 | Machine | generated copy (localize_native) |
| mutexUnlock | libnx.a | 1.2 | 1.15 | 38 | Not optimised | system library |
| sub_820CA688 | app | 1.1 | 1.06 | 35 | Machine | generated copy (localize_native) |
| sub_82265E40 | app | 1.0 | 0.98 | 32 | Machine | generated copy (localize_native) |
| sub_820C9568 | app | 1.0 | 0.95 | 31 | Machine | generated copy (localize_native) |
| sub_820E57E0 | app | 1.0 | 0.91 | 30 | Machine | generated copy (localize_native) |
| Push | render_thread.cpp.obj | 1.0 | 0.91 | 30 | Hand | renderer, hand-written |
| sub_82379AB0 | app | 0.9 | 0.82 | 27 | Machine | generated copy (localize_native) |
| sub_820F1820 | app | 0.9 | 0.81 | 27 | Machine | generated copy (localize_native) |
| sub_820EB798 | app | 0.8 | 0.80 | 26 | Machine | generated copy (localize_native) |
| sub_8214CBB8 | app | 0.8 | 0.79 | 26 | Machine | generated copy (localize_native) |
| QueueDraw | queued.cpp.obj | 0.8 | 0.79 | 26 | Hand | renderer, hand-written |
| sub_82379728 | app | 0.8 | 0.76 | 25 | Machine | generated copy (localize_native) |

### 2731 Main XThread (F: 72.4 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| sub_8232D9F8 | app | 9.0 | 6.54 | 225 | Machine | generated copy (localize_native) |
| sub_82378E30 | app | 7.2 | 5.21 | 179 | Machine | generated copy (localize_native) |
| sub_820E63D0 | app | 2.9 | 2.13 | 73 | Machine | generated copy (localize_native) |
| sub_8232EC18 | app | 2.8 | 2.04 | 70 | Machine | generated copy (localize_native) |
| sub_822A1588 | app | 2.6 | 1.87 | 64 | Machine | generated copy (localize_native) |
| sub_8210EFB8 | app | 2.4 | 1.76 | 60 | Machine | generated copy (localize_native) |
| sub_82359AE0 | app | 2.4 | 1.75 | 60 | Machine | generated copy (localize_native) |
| sub_82265100 | app | 2.3 | 1.66 | 57 | Machine | generated copy (localize_native) |
| sub_823284E0 | app | 2.1 | 1.52 | 52 | Machine | generated copy (localize_native) |
| sub_824C7FB8 | app | 1.8 | 1.31 | 45 | Machine | generated copy (localize_native) |
| sub_820FAE00 | app | 1.8 | 1.31 | 45 | Machine | generated copy (localize_native) |
| sub_82322E98 | app | 1.7 | 1.20 | 41 | Machine | generated copy (localize_native) |
| sub_820BA868 | app | 1.6 | 1.17 | 40 | Machine | generated copy (localize_native) |
| sub_8231EF78 | app | 1.6 | 1.16 | 40 | Machine | generated copy (localize_native) |
| sub_8225AAC0 | app | 1.3 | 0.96 | 33 | Machine | generated copy (localize_native) |
| sub_820E2F10 | app | 1.2 | 0.90 | 31 | Machine | generated copy (localize_native) |
| sub_823232A0 | app | 1.2 | 0.84 | 29 | Machine | generated copy (localize_native) |
| sub_8237A8B0 | app | 1.1 | 0.79 | 27 | Machine | generated copy (localize_native) |
| sub_8210F378 | app | 1.1 | 0.78 | 27 | Machine | generated copy (localize_native) |
| sub_820F7B60 | app | 1.1 | 0.78 | 27 | Machine | generated copy (localize_native) |
| sub_82524A78 | app | 0.9 | 0.64 | 22 | Machine | generated copy (localize_native) |
| sub_822FD678 | app | 0.8 | 0.55 | 19 | Machine | generated copy (localize_native) |
| sub_82111C10 | app | 0.8 | 0.55 | 19 | Machine | generated copy (localize_native) |
| sub_82187320 | app | 0.7 | 0.53 | 18 | Machine | generated copy (localize_native) |
| sub_82260510 | app | 0.7 | 0.52 | 18 | Machine | generated copy (localize_native) |

### 2735 ?: 62.7 % of a core, mask 0x2
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| FastDrawLocked | draw.cpp.obj | 25.4 | 15.90 | 484 | Hand | renderer, hand-written |
| WholeGeometryTry | draw.cpp.obj | 9.5 | 5.95 | 181 | Hand | renderer, hand-written |
| FillTextures | draw.cpp.obj | 9.0 | 5.65 | 172 | Hand | renderer, hand-written |
| PushFloatConstants | draw.cpp.obj | 8.3 | 5.20 | 159 | Hand | renderer, hand-written |
| QueueCapture (queued call) | queued.cpp.obj | 6.5 | 4.10 | 125 | Hand | renderer, hand-written |
| PagesClean | draw.cpp.obj | 2.7 | 1.72 | 52 | Hand | renderer, hand-written |
| ? | ? | 2.5 | 1.59 | 49 | Not optimised | no symbol |
| ComputeViewport | draw.cpp.obj | 1.9 | 1.17 | 36 | Hand | renderer, hand-written |
| BindWholeStreams | draw.cpp.obj | 1.7 | 1.05 | 32 | Hand | renderer, hand-written |
| Run | render_thread.cpp.obj | 1.6 | 0.99 | 30 | Hand | renderer, hand-written |
| Give | queued.cpp.obj | 1.6 | 0.98 | 30 | Hand | renderer, hand-written |
| bindTextures | libplume.a | 1.2 | 0.75 | 23 | Hand | renderer back end, hand-written |
| BindTargets | draw.cpp.obj | 1.1 | 0.72 | 22 | Hand | renderer, hand-written |
| GetHostSurfaceLocked | surfaces.cpp.obj | 1.1 | 0.69 | 21 | Hand | renderer, hand-written |
| mutexUnlock | libnx.a | 1.0 | 0.63 | 19 | Not optimised | system library |
| setPipeline | libplume.a | 1.0 | 0.62 | 19 | Hand | renderer back end, hand-written |
| bindGraphicsBuffers | libplume.a | 0.9 | 0.55 | 17 | Hand | renderer back end, hand-written |
| flushDescriptorInvalidate | libplume.a | 0.8 | 0.52 | 16 | Hand | renderer back end, hand-written |
| ProveCopyRange | draw.cpp.obj | 0.8 | 0.49 | 15 | Hand | renderer, hand-written |
| PushSharedConstants | draw.cpp.obj | 0.7 | 0.46 | 14 | Hand | renderer, hand-written |
| FindShaderLocked | shader.cpp.obj | 0.7 | 0.46 | 14 | Hand | renderer, hand-written |
| TransitionSurfaceLocked | surfaces.cpp.obj | 0.7 | 0.43 | 13 | Hand | renderer, hand-written |
| dkCmdBufBindShaders | libdeko3d.a | 0.6 | 0.39 | 12 | Not optimised | system library |
| BindTextureLocked | textures.cpp.obj | 0.6 | 0.39 | 12 | Hand | renderer, hand-written |
| Get | draw.cpp.obj | 0.6 | 0.36 | 11 | Hand | renderer, hand-written |

### 2727 Audio Worker (F: 18.8 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| sub_8233F578 | app | 28.9 | 5.45 | 163 | Machine | generated copy (localize_native) |
| sub_823516E8 | app | 17.5 | 3.30 | 98 | Machine | generated copy (localize_native) |
| sub_82342E38 | app | 4.6 | 0.87 | 26 | Machine | generated copy (localize_native) |
| sub_823639D8 | app | 2.0 | 0.38 | 11 | Machine | generated copy (localize_native) |
| WriteRegister | librexruntime.a | 2.0 | 0.37 | 11 | Not optimised | SDK runtime |
| sub_82338A40 | app | 2.0 | 0.37 | 11 | Machine | generated copy (localize_native) |
| sub_8235CC00 | app | 1.8 | 0.34 | 10 | Machine | generated copy (localize_native) |
| sub_82359AE0 | app | 1.6 | 0.30 | 9 | Machine | generated copy (localize_native) |
| pthread_mutex_lock | libsysbase.a | 1.3 | 0.24 | 7 | Not optimised | system library |
| WaitMultiple | librexruntime.a | 1.3 | 0.24 | 7 | Not optimised | SDK runtime |
| sub_82341100 | app | 1.3 | 0.24 | 7 | Machine | generated copy (localize_native) |
| sub_8236B790 | app | 1.2 | 0.23 | 7 | Machine | generated copy (localize_native) |
| sub_82341DD8 | app | 1.2 | 0.23 | 7 | Machine | generated copy (localize_native) |
| now | libstdc++.a | 1.1 | 0.21 | 6 | Not optimised | system library |
| sub_8235A8C8 | default | 1.1 | 0.21 | 6 | Not optimised | plain recompile; not on any list |
| sub_82364DE0 | app | 1.1 | 0.20 | 6 | Machine | generated copy (localize_native) |
| sub_82344048 | app | 1.0 | 0.20 | 6 | Machine | generated copy (localize_native) |
| sub_823517C8 | app | 1.0 | 0.20 | 6 | Machine | generated copy (localize_native) |
| mutexUnlock | libnx.a | 0.9 | 0.17 | 5 | Not optimised | system library |
| sub_826F64D8 | app | 0.9 | 0.17 | 5 | Machine | generated copy (localize_native) |
| RexSwitchPerfAdd | switch_perf.cpp.obj | 0.9 | 0.16 | 5 | Hand | renderer, hand-written |
| sub_8233A058 | app | 0.9 | 0.16 | 5 | Machine | generated copy (localize_native) |
| sub_8233A140 | app | 0.8 | 0.16 | 5 | Machine | generated copy (localize_native) |
| sub_82369768 | app | 0.7 | 0.14 | 4 | Machine | generated copy (localize_native) |
| xeKeKfAcquireSpinLock | librexruntime.a | 0.7 | 0.14 | 4 | Not optimised | SDK runtime |

### 2726 XMA Decoder (F8: 16.8 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| Work | librexruntime.a | 22.9 | 3.85 | 101 | Not optimised | SDK runtime |
| ? | ? | 18.6 | 3.13 | 85 | Not optimised | no symbol |
| decode_frame | liblibavcodec.a | 12.8 | 2.15 | 58 | Not optimised | system library |
| ff_wma_run_level_decode | liblibavcodec.a | 11.1 | 1.87 | 50 | Not optimised | system library |
| WorkerThreadMain | librexruntime.a | 6.3 | 1.06 | 28 | Not optimised | SDK runtime |
| _malloc_r | libc.a | 2.9 | 0.49 | 13 | Not optimised | system library |
| mutexUnlock | libnx.a | 2.6 | 0.44 | 11 | Not optimised | system library |
| buffer_replace | liblibavutil.a | 2.2 | 0.37 | 9 | Not optimised | system library |
| _free_r | libc.a | 2.2 | 0.37 | 10 | Not optimised | system library |
| ConvertFrame | librexruntime.a | 1.3 | 0.23 | 6 | Not optimised | SDK runtime |
| Peek | librexruntime.a | 1.3 | 0.22 | 6 | Not optimised | SDK runtime |
| mutexLock | libnx.a | 1.3 | 0.22 | 6 | Not optimised | system library |
| __syscall_clock_gettime | libnx.a | 0.9 | 0.16 | 4 | Not optimised | system library |
| avcodec_send_packet | liblibavcodec.a | 0.9 | 0.16 | 4 | Not optimised | system library |
| Decode | librexruntime.a | 0.9 | 0.15 | 4 | Not optimised | SDK runtime |
| av_packet_unref | liblibavcodec.a | 0.9 | 0.15 | 4 | Not optimised | system library |
| decode_receive_frame_internal | liblibavcodec.a | 0.8 | 0.14 | 4 | Not optimised | system library |
| ff_get_buffer | liblibavcodec.a | 0.7 | 0.11 | 3 | Not optimised | system library |
| avcodec_default_get_buffer2 | liblibavcodec.a | 0.5 | 0.08 | 2 | Not optimised | system library |
| pthread_mutex_lock | libsysbase.a | 0.4 | 0.07 | 2 | Not optimised | system library |
| now | libstdc++.a | 0.4 | 0.07 | 2 | Not optimised | system library |
| rmutexLock | libnx.a | 0.4 | 0.07 | 2 | Not optimised | system library |
| av_frame_unref.part.0 | liblibavutil.a | 0.4 | 0.07 | 2 | Not optimised | system library |
| Wait | librexruntime.a | 0.4 | 0.07 | 2 | Not optimised | SDK runtime |
| NoteProduction | librexruntime.a | 0.4 | 0.07 | 2 | Not optimised | SDK runtime |

### 2723 ?: 7.0 % of a core, mask 0x4
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| canCommuteDefSrc | uam_local.o | 20.0 | 1.40 | 48 | Not optimised | shader compiler |
| _free_r | libc.a | 9.8 | 0.69 | 25 | Not optimised | system library |
| canCommuteDefDef | uam_local.o | 6.5 | 0.45 | 16 | Not optimised | shader compiler |
| interfers | uam_local.o | 5.2 | 0.37 | 12 | Not optimised | shader compiler |
| mutexUnlock | libnx.a | 5.0 | 0.35 | 11 | Not optimised | system library |
| accept | uam_local.o | 4.8 | 0.34 | 12 | Not optimised | shader compiler |
| mutexLock | libnx.a | 3.6 | 0.25 | 9 | Not optimised | system library |
| visit_list_elements | uam_local.o | 2.3 | 0.16 | 6 | Not optimised | shader compiler |
| _memalign_r | libc.a | 1.9 | 0.14 | 5 | Not optimised | system library |
| set | uam_local.o | 1.4 | 0.10 | 4 | Not optimised | shader compiler |
| isResultEqual | uam_local.o | 1.3 | 0.09 | 3 | Not optimised | shader compiler |
| search | uam_local.o | 1.3 | 0.09 | 3 | Not optimised | shader compiler |
| accept | uam_local.o | 1.2 | 0.09 | 3 | Not optimised | shader compiler |
| rmutexLock | libnx.a | 1.2 | 0.08 | 3 | Not optimised | system library |
| _mesa_glsl_parse | uam_local.o | 1.2 | 0.08 | 3 | Not optimised | shader compiler |
| ~Instruction | uam_local.o | 1.1 | 0.08 | 3 | Not optimised | shader compiler |
| hash_table_search | uam_local.o | 1.0 | 0.07 | 3 | Not optimised | shader compiler |
| visit_enter | uam_local.o | 1.0 | 0.07 | 2 | Not optimised | shader compiler |
| visit | uam_local.o | 1.0 | 0.07 | 3 | Not optimised | shader compiler |
| eliminate_dead_code | uam_local.o | 0.9 | 0.06 | 2 | Not optimised | shader compiler |
| equals | uam_local.o | 0.8 | 0.06 | 2 | Not optimised | shader compiler |
| _malloc_r | libc.a | 0.8 | 0.06 | 2 | Not optimised | system library |
| visit | uam_local.o | 0.8 | 0.05 | 2 | Not optimised | shader compiler |
| valueHash | uam_local.o | 0.7 | 0.05 | 2 | Not optimised | shader compiler |
| ralloc_free | uam_local.o | 0.7 | 0.05 | 2 | Not optimised | shader compiler |

### 2720 rex::thread::TimerQueue: 1.6 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| TimerThreadMain | librexruntime.a | 23.0 | 0.36 | 30 | Not optimised | SDK runtime |
| mutexLock | libnx.a | 12.5 | 0.20 | 15 | Not optimised | system library |
| UpdateGuestClock | librexruntime.a | 8.4 | 0.13 | 9 | Not optimised | SDK runtime |
| condvarWaitTimeout | libnx.a | 6.4 | 0.10 | 7 | Not optimised | system library |
| pthread_getspecific | libsysbase.a | 6.1 | 0.10 | 8 | Not optimised | system library |
| threadTlsGet | libnx.a | 5.4 | 0.09 | 6 | Not optimised | system library |
| mutexUnlock | libnx.a | 4.8 | 0.08 | 6 | Not optimised | system library |
| TimerThreadMain | librexruntime.a | 4.0 | 0.06 | 5 | Not optimised | SDK runtime |
| __syscall_clock_gettime | libnx.a | 3.4 | 0.05 | 4 | Not optimised | system library |
| _malloc_r | libc.a | 3.1 | 0.05 | 4 | Not optimised | system library |
| now | libstdc++.a | 2.7 | 0.04 | 2 | Not optimised | system library |
| __notify_impl | libstdc++.a | 2.3 | 0.04 | 3 | Not optimised | system library |
| uncaught_exception | libstdc++.a | 2.0 | 0.03 | 3 | Not optimised | system library |
| abstimespec2nsec | libsysbase.a | 1.9 | 0.03 | 2 | Not optimised | system library |
| __cxa_get_globals | libstdc++.a | 1.9 | 0.03 | 3 | Not optimised | system library |
| _free_r | libc.a | 1.8 | 0.03 | 2 | Not optimised | system library |
| __libc_lock_release_recursive | libsysbase.a | 1.8 | 0.03 | 2 | Not optimised | system library |
| operator delete | libstdc++.a | 1.4 | 0.02 | 1 | Not optimised | system library |
| __libc_lock_release | libsysbase.a | 1.2 | 0.02 | 1 | Not optimised | system library |
| RexSwitchPerfAdd | switch_perf.cpp.obj | 1.2 | 0.02 | 1 | Hand | renderer, hand-written |
| clock_gettime | libsysbase.a | 0.9 | 0.01 | 1 | Not optimised | system library |
| pthread_mutex_lock | libsysbase.a | 0.9 | 0.01 | 1 | Not optimised | system library |
| queued call | librexruntime.a | 0.7 | 0.01 | 1 | Not optimised | SDK runtime |
| __syscall_cond_wait | libnx.a | 0.6 | 0.01 | 1 | Not optimised | system library |
| now | libstdc++.a | 0.6 | 0.01 | 1 | Not optimised | system library |

### 2739 ?: 1.3 % of a core, mask 0x1
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| FillAndAppend | librexruntime.a | 36.4 | 0.42 | 6 | Not optimised | SDK runtime |
| MixFrameInto | librexruntime.a | 16.5 | 0.19 | 3 | Not optimised | SDK runtime |
| audoutWaitPlayFinish | libnx.a | 14.1 | 0.16 | 1 | Not optimised | system library |
| pthread_mutex_lock | libsysbase.a | 14.1 | 0.16 | 1 | Not optimised | system library |
| lrintf | libm.a | 7.1 | 0.08 | 1 | Not optimised | system library |
| mutexUnlock | libnx.a | 7.1 | 0.08 | 1 | Not optimised | system library |
| waitObjects | libnx.a | 4.7 | 0.05 | 1 | Not optimised | system library |

## Biggest functions that are not optimised, all threads
| Function | Thread(s) | % of a core | Samples | Detail |
|---|---|---:|---:|---|
| ? | 2723, 2735, Main XThread (F, XMA Decoder (F8, XThread7C00EA0 , XThreadD7964EE0, main (UI) | 8.12 | 247 | no symbol |
| Work | XMA Decoder (F8 | 3.85 | 101 | SDK runtime |
| mutexUnlock | 2723, 2735, 2739, Audio Worker (F, Main XThread (F, XMA Decoder (F8, XThreadD7964EE0, rex::thread::TimerQueue | 2.93 | 92 | system library |
| mutexLock | 2723, 2735, Audio Worker (F, XMA Decoder (F8, XThreadD7964EE0, rex::thread::TimerQueue | 2.55 | 91 | system library |
| decode_frame | XMA Decoder (F8 | 2.15 | 58 | system library |
| ff_wma_run_level_decode | XMA Decoder (F8 | 1.87 | 50 | system library |
| canCommuteDefSrc | 2723 | 1.40 | 48 | shader compiler |
| _free_r | 2723, 2735, Audio Worker (F, XMA Decoder (F8, XThreadD7964EE0, main (UI), rex::thread::TimerQueue | 1.25 | 42 | system library |
| pthread_mutex_lock | 2735, 2739, Audio Worker (F, XMA Decoder (F8, XThreadD7964EE0, rex::thread::TimerQueue | 1.07 | 30 | system library |
| WorkerThreadMain | XMA Decoder (F8 | 1.06 | 28 | SDK runtime |
| pthread_mutex_unlock | 2735, Audio Worker (F, XMA Decoder (F8, XThreadD7964EE0 | 0.74 | 23 | system library |
| _malloc_r | 2723, Audio Worker (F, XMA Decoder (F8, XThreadD7964EE0, rex::thread::TimerQueue | 0.69 | 22 | system library |
| sub_822650D8 | Main XThread (F, XThreadD7964EE0 | 0.49 | 17 | plain recompile; not on any list |
| canCommuteDefDef | 2723 | 0.45 | 16 | shader compiler |
| FillAndAppend | 2739 | 0.42 | 6 | SDK runtime |
| __syscall_clock_gettime | 2735, Audio Worker (F, Main XThread (F, XMA Decoder (F8, rex::thread::TimerQueue | 0.41 | 14 | system library |
| XXH3_64bits | XThreadD7964EE0 | 0.40 | 13 | system library |
| dkCmdBufBindShaders | 2735 | 0.39 | 12 | system library |
| buffer_replace | XMA Decoder (F8 | 0.37 | 9 | system library |
| WriteRegister | Audio Worker (F | 0.37 | 11 | SDK runtime |
| interfers | 2723 | 0.37 | 12 | shader compiler |
| dkCmdBufPushConstants | 2735 | 0.36 | 11 | system library |
| TimerThreadMain | rex::thread::TimerQueue | 0.36 | 30 | SDK runtime |
| sub_822A1CE0 | Main XThread (F | 0.35 | 12 | plain recompile; not on any list |
| accept | 2723 | 0.34 | 12 | shader compiler |
| now | 2735, Audio Worker (F, XMA Decoder (F8, rex::thread::TimerQueue | 0.32 | 10 | system library |
| dkCmdBufDrawIndexed | 2735 | 0.29 | 9 | system library |
| __aarch64_read_tp | Main XThread (F, XThreadD7964EE0 | 0.27 | 9 | system library |
| dkCmdBufBindVtxBufferState | 2735 | 0.26 | 8 | system library |
| __libc_lock_release | 2735, Audio Worker (F, XThreadD7964EE0, rex::thread::TimerQueue | 0.25 | 8 | system library |

Plain recompiled functions sampled in gameplay: 136; of them on the held-back list: 0; hand natives in the profile: 0
Samples in gameplay: 8993
