/**
 * @file    gpu/render/occlusion.h
 * @brief   kameo-nx (2026-11-04): real occlusion counts for the game's D3DQuery objects (occlusion.cpp).
 */
#pragma once

#include <rex/types.h>

namespace renut::gpu {

// D3DQuery_Issue of an occlusion query, in the game's thread: 'flags' as the call's (2 begin, 1 end).
void OcclusionQueryIssue(u32 device_va, u32 query, u32 flags);

// Whether the render thread is between a query's begin and end: a draw with no pixel shader there is its test quad.
bool OcclusionQueryOpenLocked();

// D3DQuery_GetData of one: false while its last end is still on its way through the GPU.
bool OcclusionQueryResult(u32 query, u32 *samples);

} // namespace renut::gpu
