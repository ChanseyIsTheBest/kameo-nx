# Third console run (2026-11-03, 20:21, package rc6_ready)

Handheld at stock clocks (CPU 1020 MHz, GPU 460.8 MHz), Thorn's Castle, about 80 s of gameplay, through the forwarder: `process cores 0xF`, core 3 granted. Audio Worker and shader compiler on core 3 only.

| | Second run (three cores) | Third run (four cores, hand work) |
|---|---|---|
| Gameplay frame rate | 26.3 fps | 29.5 fps |
| Frame time | 38.0 ms | 33.9 ms |
| Draws a frame | 2,489 | 2,280 |
| Render thread a draw | 9.24 us | 9.25 us |
| GPU a draw | 9.49 us | 9.71 us |
| Blocked on compiles | 0 s | 0 s |

**Not the same scene and not a clean test.** Fewer draws a frame, and the core change and the hand work arrived together. Fitting frame time against draws in the second run predicts 37.1 ms at the third run's draw counts, against 34.1 ms measured (8 % better). Windows with nearly the same draw count gain 2-4.5 ms. Separating the core change from the hand work needs a run of this NRO from the homebrew menu.

The drawing thread's whole cost per draw did not fall (14.17 to 14.28 us). The edited functions fell, and others rose by about as much, which reads as scene: `sub_82110280`, not edited, rose 12 %.

Per draw: `sub_820E7F10` -13.7 %, `sub_82524E98` -14.8 %, `sub_82379E10` -4.0 %, together about 0.7 ms a frame. Both skip loops did not get cheaper as loops: they wait on memory.

Core 3 idle in gameplay: 26.6-41.7 %. Audio: 0 requests without data in any gameplay report.

gameplay reports 7 (30.5 fps), of them with the core-2 worker under 10 %: 7 (30.5 fps), over: 0 (0.0 fps)
CPU total mean: all 282 %, calm 282 %, compiling 0 % of 300 %

| Thread | Top function | Core mask | Priority | CPU gameplay | CPU no compile | CPU compiling | Peak |
|---|---|---|---|---:|---:|---:|---:|
| 3160 XThreadCB231700 | CaptureDelta | 0x7 (pref -1) | 0x3B | 97.5 | 97.5 | 0.0 | 100 |
| 3158 Main XThread (F | sub_8232D9F8 | 0x7 (pref -1) | 0x3B | 76.9 | 76.9 | 0.0 | 84 |
| 3162 ? | FastDrawLocked | 0x2 (pref 1) | 0x2C | 65.1 | 65.1 | 0.0 | 76 |
| 3154 Audio Worker (F | sub_8233F578 | 0x8 (pref 3) | 0x2B | 18.0 | 18.0 | 0.0 | 22 |
| 3153 XMA Decoder (F8 | ? | 0x7 (pref -1) | 0x2B | 17.5 | 17.5 | 0.0 | 22 |
| 3150 ? | canCommuteDefSrc | 0x8 (pref 3) | 0x3B | 2.2 | 2.2 | 0.0 | 6 |
| 3146 rex::thread::TimerQueue | TimerThreadMain | 0x7 (pref -1) | 0x3B | 1.6 | 1.6 | 0.0 | 2 |
| 3166 ? | FillAndAppend | 0x1 (pref 0) | 0x2B | 1.2 | 1.2 | 0.0 | 1 |
| 3133 main (UI) | AddConvexPolyFilled | 0x1 (pref 0) | 0x2C | 0.8 | 0.8 | 0.0 | 1 |
| 3180 XThreadCC5FAC80 | CheckTitleTermination | 0x7 (pref -1) | 0x3B | 0.6 | 0.6 | 0.0 | 1 |
| 3157 ? | armDCacheClean | 0x4 (pref 2) | 0x3B | 0.2 | 0.2 | 0.0 | 0 |
| 3152 ? | __wrap_svcSleepThread | 0x7 (pref -1) | 0x3B | 0.1 | 0.1 | 0.0 | 0 |
| 3164 XThreadCB358F40 | mutexLock | 0x7 (pref -1) | 0x3B | 0.1 | 0.1 | 0.0 | 0 |
| 3151 ? | - | 0x4 (pref 2) | 0x3B | 0.0 | 0.0 | 0.0 | 0 |

## Classes per thread, % of a core
| Thread | Hand | Machine | Not optimised | of which plain recompile | of which held back | of which libraries |
|---|---:|---:|---:|---:|---:|---:|
| 3160 XThreadCB231700 | 25.6 | 62.1 | 9.8 | 1.4 | 0.0 | 8.3 |
| 3158 Main XThread (F | 3.6 | 68.3 | 5.0 | 3.9 | 0.0 | 1.1 |
| 3162 ? | 58.0 | 0.0 | 7.1 | 0.0 | 0.0 | 7.1 |
| 3154 Audio Worker (F | 0.8 | 13.2 | 4.0 | 0.5 | 0.0 | 3.5 |
| 3153 XMA Decoder (F8 | 0.0 | 0.0 | 17.5 | 0.0 | 0.0 | 17.5 |
| 3150 ? | 0.0 | 0.0 | 2.2 | 0.0 | 0.0 | 2.2 |
| 3146 rex::thread::TimerQueue | 0.0 | 0.0 | 1.6 | 0.0 | 0.0 | 1.6 |
| 3166 ? | 0.0 | 0.0 | 1.1 | 0.0 | 0.0 | 1.1 |
| All threads | 88.1 | 143.7 | 49.6 | | | |
Share of all busy CPU: Hand 31.3 %, Machine 51.1 %, Not optimised 17.6 %
   51.1 %  generated copy (localize_native)
   25.8 %  renderer, hand-written
    7.5 %  system library
    4.1 %  SDK runtime
    3.7 %  app, hand-written
    3.4 %  no symbol
    2.1 %  plain recompile; not on any list
    1.4 %  renderer back end, hand-written
    0.6 %  shader compiler
    0.3 %  hook replacing a game function
    0.1 %  hand-written native

## Functions per thread

### 3160 XThreadCB231700: 97.5 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| CaptureDelta | queued.cpp.obj | 13.5 | 13.15 | 369 | Hand | renderer, hand-written |
| sub_820E7F10 | app | 9.1 | 8.83 | 247 | Machine | generated copy (localize_native) |
| sub_82379E10 | app | 6.4 | 6.21 | 174 | Machine | generated copy (localize_native) |
| sub_82110280 | app | 5.6 | 5.47 | 153 | Machine | generated copy (localize_native) |
| sub_8231C268 | app | 4.2 | 4.11 | 115 | Machine | generated copy (localize_native) |
| ? | ? | 2.9 | 2.78 | 78 | Not optimised | no symbol |
| sub_82524E98 | app | 2.8 | 2.75 | 77 | Machine | generated copy (localize_native) |
| sub_8231B190 | app | 2.0 | 1.96 | 55 | Machine | generated copy (localize_native) |
| Prepare | kameo_d3d.cpp.obj | 2.0 | 1.96 | 55 | Hand | renderer, hand-written |
| sub_8225AC50 | app | 1.6 | 1.57 | 44 | Machine | generated copy (localize_native) |
| sub_820EB9B0 | app | 1.6 | 1.53 | 43 | Machine | generated copy (localize_native) |
| sub_820CA688 | app | 1.5 | 1.43 | 40 | Machine | generated copy (localize_native) |
| mutexUnlock | libnx.a | 1.5 | 1.43 | 40 | Not optimised | system library |
| sub_82265E40 | app | 1.4 | 1.32 | 37 | Machine | generated copy (localize_native) |
| sub_820C9568 | app | 1.3 | 1.29 | 36 | Machine | generated copy (localize_native) |
| mutexLock | libnx.a | 1.2 | 1.21 | 34 | Not optimised | system library |
| sub_821028C0 | app | 1.1 | 1.07 | 30 | Machine | generated copy (localize_native) |
| sub_820E57E0 | app | 1.0 | 0.96 | 27 | Machine | generated copy (localize_native) |
| sub_82379AB0 | app | 1.0 | 0.93 | 26 | Machine | generated copy (localize_native) |
| sub_82379728 | app | 0.9 | 0.86 | 24 | Machine | generated copy (localize_native) |
| Push | render_thread.cpp.obj | 0.9 | 0.86 | 24 | Hand | renderer, hand-written |
| sub_820CA330 | app | 0.8 | 0.82 | 23 | Machine | generated copy (localize_native) |
| sub_820F1820 | app | 0.8 | 0.82 | 23 | Machine | generated copy (localize_native) |
| sub_820C4300 | app | 0.8 | 0.75 | 21 | Machine | generated copy (localize_native) |
| sub_8217ABC8 | app | 0.7 | 0.72 | 20 | Machine | generated copy (localize_native) |

### 3158 Main XThread (F: 76.9 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| sub_8232D9F8 | app | 8.3 | 6.41 | 186 | Machine | generated copy (localize_native) |
| sub_82378E30 | app | 7.2 | 5.52 | 160 | Machine | generated copy (localize_native) |
| sub_8232EC18 | app | 2.8 | 2.14 | 62 | Machine | generated copy (localize_native) |
| sub_82265100 | app | 2.6 | 1.96 | 57 | Machine | generated copy (localize_native) |
| sub_8231EF78 | app | 2.4 | 1.86 | 54 | Machine | generated copy (localize_native) |
| sub_820E63D0 | app | 2.4 | 1.86 | 54 | Machine | generated copy (localize_native) |
| sub_823284E0 | app | 2.3 | 1.76 | 51 | Machine | generated copy (localize_native) |
| sub_822A1588 | app | 2.2 | 1.66 | 48 | Machine | generated copy (localize_native) |
| sub_8210EFB8 | app | 2.1 | 1.65 | 48 | Machine | generated copy (localize_native) |
| sub_824C7FB8 | app | 2.0 | 1.55 | 45 | Machine | generated copy (localize_native) |
| sub_8225AAC0 | app | 1.9 | 1.49 | 43 | Machine | generated copy (localize_native) |
| sub_820BA868 | app | 1.9 | 1.48 | 43 | Machine | generated copy (localize_native) |
| sub_82359AE0 | app | 1.8 | 1.42 | 41 | Machine | generated copy (localize_native) |
| sub_82322E98 | app | 1.7 | 1.31 | 38 | Machine | generated copy (localize_native) |
| sub_820FAE00 | app | 1.6 | 1.20 | 35 | Machine | generated copy (localize_native) |
| sub_823232A0 | app | 1.3 | 1.00 | 29 | Machine | generated copy (localize_native) |
| sub_82117FC0 | app | 1.1 | 0.83 | 24 | Machine | generated copy (localize_native) |
| sub_824C7868 | app | 1.0 | 0.76 | 22 | Machine | generated copy (localize_native) |
| sub_8210F378 | app | 1.0 | 0.76 | 22 | Machine | generated copy (localize_native) |
| sub_8214B588 | app | 0.9 | 0.69 | 20 | Machine | generated copy (localize_native) |
| sub_820E2F10 | app | 0.9 | 0.66 | 19 | Machine | generated copy (localize_native) |
| sub_820F7B60 | app | 0.9 | 0.65 | 19 | Machine | generated copy (localize_native) |
| sub_820FB460 | app | 0.8 | 0.65 | 19 | Machine | generated copy (localize_native) |
| sub_8237A8B0 | app | 0.8 | 0.62 | 18 | Machine | generated copy (localize_native) |
| sub_82260510 | app | 0.8 | 0.59 | 17 | Machine | generated copy (localize_native) |

### 3162 ?: 65.1 % of a core, mask 0x2
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| FastDrawLocked | draw.cpp.obj | 24.0 | 15.64 | 415 | Hand | renderer, hand-written |
| FillTextures | draw.cpp.obj | 9.9 | 6.41 | 170 | Hand | renderer, hand-written |
| PushFloatConstants | draw.cpp.obj | 8.8 | 5.70 | 151 | Hand | renderer, hand-written |
| WholeGeometryTry | draw.cpp.obj | 7.7 | 4.99 | 132 | Hand | renderer, hand-written |
| QueueCapture (queued call) | queued.cpp.obj | 4.9 | 3.20 | 85 | Hand | renderer, hand-written |
| ? | ? | 4.1 | 2.64 | 70 | Not optimised | no symbol |
| PagesClean | draw.cpp.obj | 2.5 | 1.65 | 44 | Hand | renderer, hand-written |
| ComputeViewport | draw.cpp.obj | 1.8 | 1.20 | 32 | Hand | renderer, hand-written |
| Give | queued.cpp.obj | 1.8 | 1.17 | 31 | Hand | renderer, hand-written |
| bindTextures | libplume.a | 1.7 | 1.13 | 30 | Hand | renderer back end, hand-written |
| ConvertVertices | vertex_format.cpp.obj | 1.4 | 0.90 | 24 | Hand | renderer, hand-written |
| ExpandElement | vertex_format.cpp.obj | 1.3 | 0.87 | 23 | Hand | renderer, hand-written |
| Run | render_thread.cpp.obj | 1.3 | 0.83 | 22 | Hand | renderer, hand-written |
| mutexUnlock | libnx.a | 1.2 | 0.79 | 21 | Not optimised | system library |
| GetHostSurfaceLocked | surfaces.cpp.obj | 1.2 | 0.79 | 21 | Hand | renderer, hand-written |
| FindShaderLocked | shader.cpp.obj | 1.1 | 0.72 | 19 | Hand | renderer, hand-written |
| BindTargets | draw.cpp.obj | 1.1 | 0.71 | 19 | Hand | renderer, hand-written |
| BindWholeStreams | draw.cpp.obj | 1.1 | 0.71 | 19 | Hand | renderer, hand-written |
| setPipeline | libplume.a | 1.0 | 0.64 | 17 | Hand | renderer back end, hand-written |
| GatherTargets | draw.cpp.obj | 0.9 | 0.57 | 15 | Hand | renderer, hand-written |
| bindGraphicsBuffers | libplume.a | 0.9 | 0.57 | 15 | Hand | renderer back end, hand-written |
| BoundSurfaceDesc | kameo_d3d.cpp.obj | 0.7 | 0.49 | 13 | Hand | renderer, hand-written |
| dkCmdBufPushConstants | libdeko3d.a | 0.7 | 0.45 | 12 | Not optimised | system library |
| drawIndexedInstanced | libplume.a | 0.6 | 0.41 | 11 | Hand | renderer back end, hand-written |
| BindStreams | draw.cpp.obj | 0.6 | 0.41 | 11 | Hand | renderer, hand-written |

### 3154 Audio Worker (F: 18.0 % of a core, mask 0x8
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| sub_8233F578 | app | 25.7 | 4.63 | 183 | Machine | generated copy (localize_native) |
| sub_823516E8 | app | 19.2 | 3.45 | 137 | Machine | generated copy (localize_native) |
| sub_82342E38 | app | 4.3 | 0.77 | 30 | Machine | generated copy (localize_native) |
| WriteRegister | librexruntime.a | 2.9 | 0.53 | 21 | Not optimised | SDK runtime |
| sub_82364DE0 | app | 2.1 | 0.38 | 15 | Machine | generated copy (localize_native) |
| WaitMultiple | librexruntime.a | 2.0 | 0.36 | 14 | Not optimised | SDK runtime |
| sub_82369768 | app | 2.0 | 0.36 | 14 | Machine | generated copy (localize_native) |
| sub_8235A8C8 | default | 1.7 | 0.30 | 12 | Not optimised | plain recompile; not on any list |
| sub_823517C8 | app | 1.6 | 0.29 | 12 | Machine | generated copy (localize_native) |
| sub_82359AE0 | app | 1.6 | 0.28 | 11 | Machine | generated copy (localize_native) |
| sub_8235CC00 | app | 1.4 | 0.26 | 10 | Machine | generated copy (localize_native) |
| xeKeKfAcquireSpinLock | librexruntime.a | 1.4 | 0.25 | 10 | Not optimised | SDK runtime |
| sub_82344048 | app | 1.3 | 0.23 | 9 | Machine | generated copy (localize_native) |
| sub_82338A40 | app | 1.2 | 0.22 | 9 | Machine | generated copy (localize_native) |
| sub_82341DD8 | app | 1.2 | 0.22 | 9 | Machine | generated copy (localize_native) |
| sub_823639D8 | app | 1.1 | 0.20 | 8 | Machine | generated copy (localize_native) |
| sub_8233A058 | app | 1.0 | 0.18 | 7 | Machine | generated copy (localize_native) |
| pthread_mutex_unlock | libsysbase.a | 1.0 | 0.18 | 7 | Not optimised | system library |
| sub_8233A140 | app | 1.0 | 0.18 | 7 | Machine | generated copy (localize_native) |
| sub_826F64D8 | app | 1.0 | 0.17 | 7 | Machine | generated copy (localize_native) |
| pthread_mutex_lock | libsysbase.a | 0.7 | 0.13 | 5 | Not optimised | system library |
| XMAGetOutputBufferWriteOffset_entry | librexruntime.a | 0.7 | 0.13 | 5 | Not optimised | SDK runtime |
| XMASetOutputBufferValid_entry | librexruntime.a | 0.7 | 0.13 | 5 | Not optimised | SDK runtime |
| mutexUnlock | libnx.a | 0.7 | 0.12 | 5 | Not optimised | system library |
| sub_82363FE8 | app | 0.7 | 0.12 | 5 | Machine | generated copy (localize_native) |

### 3153 XMA Decoder (F8: 17.5 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| ? | ? | 19.4 | 3.40 | 89 | Not optimised | no symbol |
| Work | librexruntime.a | 16.8 | 2.94 | 77 | Not optimised | SDK runtime |
| decode_frame | liblibavcodec.a | 15.5 | 2.71 | 72 | Not optimised | system library |
| WorkerThreadMain | librexruntime.a | 9.5 | 1.67 | 44 | Not optimised | SDK runtime |
| ff_wma_run_level_decode | liblibavcodec.a | 9.0 | 1.57 | 42 | Not optimised | system library |
| ConvertFrame | librexruntime.a | 2.9 | 0.50 | 13 | Not optimised | SDK runtime |
| _free_r | libc.a | 2.5 | 0.44 | 11 | Not optimised | system library |
| _malloc_r | libc.a | 1.9 | 0.34 | 9 | Not optimised | system library |
| mutexUnlock | libnx.a | 1.8 | 0.31 | 8 | Not optimised | system library |
| buffer_replace | liblibavutil.a | 1.7 | 0.30 | 8 | Not optimised | system library |
| mutexLock | libnx.a | 1.7 | 0.30 | 8 | Not optimised | system library |
| decode_receive_frame_internal | liblibavcodec.a | 1.6 | 0.27 | 7 | Not optimised | system library |
| StoreContextMerged | librexruntime.a | 1.1 | 0.20 | 5 | Not optimised | SDK runtime |
| now | libstdc++.a | 0.9 | 0.16 | 4 | Not optimised | system library |
| av_buffer_unref | liblibavutil.a | 0.9 | 0.15 | 4 | Not optimised | system library |
| av_buffer_pool_get | liblibavutil.a | 0.9 | 0.15 | 4 | Not optimised | system library |
| ff_decode_frame_props | liblibavcodec.a | 0.7 | 0.12 | 3 | Not optimised | system library |
| av_packet_unref | liblibavcodec.a | 0.7 | 0.12 | 3 | Not optimised | system library |
| condvarWaitTimeout | libnx.a | 0.7 | 0.12 | 3 | Not optimised | system library |
| Decode | librexruntime.a | 0.7 | 0.12 | 3 | Not optimised | SDK runtime |
| Wait | librexruntime.a | 0.6 | 0.11 | 3 | Not optimised | SDK runtime |
| av_dict_free | liblibavutil.a | 0.6 | 0.11 | 3 | Not optimised | system library |
| av_opt_free | liblibavutil.a | 0.4 | 0.08 | 2 | Not optimised | system library |
| av_buffer_ref | liblibavutil.a | 0.4 | 0.07 | 2 | Not optimised | system library |
| unlikely.decode_init | liblibavcodec.a | 0.4 | 0.07 | 2 | Not optimised | system library |

### 3150 ?: 2.2 % of a core, mask 0x8
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| canCommuteDefSrc | uam_local.o | 12.8 | 0.28 | 17 | Not optimised | shader compiler |
| mutexUnlock | libnx.a | 9.5 | 0.21 | 13 | Not optimised | system library |
| _free_r | libc.a | 7.6 | 0.16 | 11 | Not optimised | system library |
| canCommuteDefDef | uam_local.o | 6.7 | 0.15 | 9 | Not optimised | shader compiler |
| accept | uam_local.o | 4.0 | 0.09 | 6 | Not optimised | shader compiler |
| interfers | uam_local.o | 3.8 | 0.08 | 5 | Not optimised | shader compiler |
| accept | uam_local.o | 3.3 | 0.07 | 5 | Not optimised | shader compiler |
| visit_list_elements | uam_local.o | 3.3 | 0.07 | 5 | Not optimised | shader compiler |
| visit | uam_local.o | 2.1 | 0.05 | 3 | Not optimised | shader compiler |
| search | uam_local.o | 2.1 | 0.05 | 3 | Not optimised | shader compiler |
| _malloc_r | libc.a | 2.0 | 0.04 | 3 | Not optimised | system library |
| _mesa_hash_table_next_entry | uam_local.o | 2.0 | 0.04 | 3 | Not optimised | shader compiler |
| copy_propagate | uam_local.o | 2.0 | 0.04 | 3 | Not optimised | shader compiler |
| hash_table_search | uam_local.o | 2.0 | 0.04 | 3 | Not optimised | shader compiler |
| checkInterference | uam_local.o | 1.6 | 0.03 | 2 | Not optimised | shader compiler |
| coalesceValues | uam_local.o | 1.6 | 0.03 | 2 | Not optimised | shader compiler |
| isResultEqual | uam_local.o | 1.5 | 0.03 | 2 | Not optimised | shader compiler |
| isCommutationLegal | uam_local.o | 1.5 | 0.03 | 2 | Not optimised | shader compiler |
| mutexLock | libnx.a | 1.5 | 0.03 | 2 | Not optimised | system library |
| rmutexLock | libnx.a | 1.5 | 0.03 | 2 | Not optimised | system library |
| buildRIG | uam_local.o | 1.3 | 0.03 | 2 | Not optimised | shader compiler |
| ? | ? | 1.3 | 0.03 | 2 | Not optimised | no symbol |
| _mesa_glsl_parse | uam_local.o | 1.3 | 0.03 | 2 | Not optimised | shader compiler |
| releaseInstruction | uam_local.o | 0.8 | 0.02 | 1 | Not optimised | shader compiler |
| ~Instruction | uam_local.o | 0.8 | 0.02 | 1 | Not optimised | shader compiler |

### 3146 rex::thread::TimerQueue: 1.6 % of a core, mask 0x7
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| TimerThreadMain | librexruntime.a | 27.4 | 0.45 | 30 | Not optimised | SDK runtime |
| __cxa_get_globals | libstdc++.a | 12.3 | 0.20 | 14 | Not optimised | system library |
| mutexLock | libnx.a | 11.1 | 0.18 | 13 | Not optimised | system library |
| UpdateGuestClock | librexruntime.a | 7.9 | 0.13 | 9 | Not optimised | SDK runtime |
| threadTlsGet | libnx.a | 6.4 | 0.10 | 7 | Not optimised | system library |
| condvarWaitTimeout | libnx.a | 3.5 | 0.06 | 4 | Not optimised | system library |
| pthread_mutex_lock | libsysbase.a | 3.4 | 0.06 | 4 | Not optimised | system library |
| pthread_getspecific | libsysbase.a | 3.4 | 0.05 | 4 | Not optimised | system library |
| mutexUnlock | libnx.a | 3.3 | 0.05 | 4 | Not optimised | system library |
| __syscall_tls_get | libnx.a | 1.9 | 0.03 | 2 | Not optimised | system library |
| __syscall_cond_wait | libnx.a | 1.9 | 0.03 | 2 | Not optimised | system library |
| abstimespec2nsec | libsysbase.a | 1.9 | 0.03 | 2 | Not optimised | system library |
| _free_r | libc.a | 1.7 | 0.03 | 2 | Not optimised | system library |
| __getreent | libsysbase.a | 1.6 | 0.03 | 2 | Not optimised | system library |
| queued call | librexruntime.a | 1.6 | 0.03 | 2 | Not optimised | SDK runtime |
| uncaught_exception | libstdc++.a | 1.6 | 0.03 | 2 | Not optimised | system library |
| __syscall_getreent | libnx.a | 1.6 | 0.03 | 2 | Not optimised | system library |
| QueryGuestUptimeMillis | librexruntime.a | 1.1 | 0.02 | 1 | Not optimised | SDK runtime |
| __syscall_clock_gettime | libnx.a | 1.1 | 0.02 | 1 | Not optimised | system library |
| free | libc.a | 1.1 | 0.02 | 1 | Not optimised | system library |
| CompletionRoutine | librexruntime.a | 0.9 | 0.01 | 1 | Not optimised | SDK runtime |
| __notify_impl | libstdc++.a | 0.9 | 0.01 | 1 | Not optimised | system library |
| _malloc_r | libc.a | 0.8 | 0.01 | 1 | Not optimised | system library |
| __libc_cond_wait | libsysbase.a | 0.8 | 0.01 | 1 | Not optimised | system library |
| host_tick_count_platform | librexruntime.a | 0.8 | 0.01 | 1 | Not optimised | SDK runtime |

### 3166 ?: 1.2 % of a core, mask 0x1
| Function | Object | % of thread | % of a core | Samples | Class | Detail |
|---|---|---:|---:|---:|---|---|
| FillAndAppend | librexruntime.a | 44.9 | 0.48 | 4 | Not optimised | SDK runtime |
| MixFrameInto | librexruntime.a | 24.9 | 0.27 | 4 | Not optimised | SDK runtime |
| audoutWaitPlayFinish | libnx.a | 8.0 | 0.09 | 1 | Not optimised | system library |
| pthread_mutex_lock | libsysbase.a | 6.2 | 0.07 | 1 | Not optimised | system library |
| PumpTick | librexruntime.a | 5.3 | 0.06 | 1 | Not optimised | SDK runtime |
| Release | librexruntime.a | 5.3 | 0.06 | 1 | Not optimised | SDK runtime |
| ? | ? | 5.3 | 0.06 | 1 | Not optimised | no symbol |

## Biggest functions that are not optimised, all threads
| Function | Thread(s) | % of a core | Samples | Detail |
|---|---|---:|---:|---|
| ? | 3150, 3162, 3166, Audio Worker (F, Main XThread (F, XMA Decoder (F8, XThreadCB231700, XThreadE9200EA0 | 9.43 | 258 | no symbol |
| Work | XMA Decoder (F8 | 2.94 | 77 | SDK runtime |
| mutexUnlock | 3150, 3162, Audio Worker (F, XMA Decoder (F8, XThreadCB231700, rex::thread::TimerQueue | 2.91 | 91 | system library |
| decode_frame | XMA Decoder (F8 | 2.71 | 72 | system library |
| mutexLock | 3150, 3162, Audio Worker (F, XMA Decoder (F8, XThreadCB231700, XThreadCB358F40, main (UI), rex::thread::TimerQueue | 2.20 | 69 | system library |
| WorkerThreadMain | XMA Decoder (F8 | 1.67 | 44 | SDK runtime |
| ff_wma_run_level_decode | XMA Decoder (F8 | 1.57 | 42 | system library |
| pthread_mutex_lock | 3162, 3166, Audio Worker (F, Main XThread (F, XThreadCB231700, rex::thread::TimerQueue | 0.97 | 30 | system library |
| _free_r | 3150, 3162, Audio Worker (F, XMA Decoder (F8, XThreadCB231700, rex::thread::TimerQueue | 0.82 | 30 | system library |
| XXH3_64bits | XThreadCB231700 | 0.71 | 20 | system library |
| pthread_mutex_unlock | 3162, Audio Worker (F, XMA Decoder (F8, XThreadCB231700 | 0.65 | 20 | system library |
| WriteRegister | Audio Worker (F | 0.53 | 21 | SDK runtime |
| ConvertFrame | XMA Decoder (F8 | 0.50 | 13 | SDK runtime |
| FillAndAppend | 3166 | 0.48 | 4 | SDK runtime |
| _malloc_r | 3150, Audio Worker (F, XMA Decoder (F8, XThreadCB231700, rex::thread::TimerQueue | 0.46 | 15 | system library |
| dkCmdBufPushConstants | 3162 | 0.45 | 12 | system library |
| sub_822650D8 | Main XThread (F, XThreadCB231700 | 0.45 | 13 | plain recompile; not on any list |
| TimerThreadMain | rex::thread::TimerQueue | 0.45 | 30 | SDK runtime |
| __aarch64_read_tp | Main XThread (F, XThreadCB231700 | 0.39 | 11 | system library |
| dkCmdBufBindShaders | 3162 | 0.38 | 10 | system library |
| WaitMultiple | Audio Worker (F | 0.36 | 14 | SDK runtime |
| dkCmdBufDrawIndexed | 3162 | 0.34 | 9 | system library |
| buffer_replace | XMA Decoder (F8 | 0.30 | 8 | system library |
| sub_8235A8C8 | Audio Worker (F | 0.30 | 12 | plain recompile; not on any list |
| canCommuteDefSrc | 3150 | 0.28 | 17 | shader compiler |
| decode_receive_frame_internal | XMA Decoder (F8 | 0.27 | 7 | system library |
| MixFrameInto | 3166 | 0.27 | 4 | SDK runtime |
| __libc_lock_release | 3162, XThreadCB231700 | 0.26 | 7 | system library |
| __libc_lock_acquire | Audio Worker (F, XThreadCB231700 | 0.25 | 8 | system library |
| xeKeKfAcquireSpinLock | Audio Worker (F | 0.25 | 10 | SDK runtime |

Plain recompiled functions sampled in gameplay: 125; of them on the held-back list: 0; hand natives in the profile: 0
Samples in gameplay: 8185
