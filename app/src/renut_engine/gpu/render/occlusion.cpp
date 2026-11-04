/**
 * @file    gpu/render/occlusion.cpp
 * @brief   kameo-nx (2026-11-04, the user's console run: lights floating in the air): real occlusion counts.
 *
 * The light glows and the sun ask the GPU how much of a small test quad passed the depth test, and scale themselves by
 * it. With no GPU behind D3DQuery every query answered "1000 samples", as Xenia's query_occlusion_fake_sample_count does
 * (which is why the emulated reference showed the same), so a light behind a wall drew anyway.
 *
 * The idea and the rules (the count in guest pixels, not ready until the GPU has written it, a query stuck too long
 * settles for its last answer, a null pixel shader draw inside a query is its test quad) are Kameo: RePowered's, by
 * MaxDeadBear and BiRabittoh (src/gpu/occlusion.cpp, src/gpu/hooks/sync.cpp; its host-side source is under the MIT
 * License: THIRD_PARTY_NOTICES.md). The GPU side here is deko3d's samples-passed counter: reset at the begin, reported
 * into memory at the end, in the render thread's order with the draws. Only deko3d has it; elsewhere the old answer.
 */
#include "renut_engine/gpu/render/occlusion.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <rex/cvar.h>

#include "renut_engine/gpu/backend/backend.h"
#include "renut_engine/gpu/backend/queued.h"
#include "renut_engine/gpu/backend/render_thread.h"
#include "renut_engine/gpu/backend/video.h"
#include "renut_engine/gpu/backend/video_internal.h"
#include "renut_engine/gpu/guest/command_buffers.h"
#include "renut_engine/gpu/log.h"
#include "renut_engine/gpu/render/surfaces.h"
#if defined(PLUME_DEKO3D)
#include <plume_deko3d.h>
#endif

REXCVAR_DEFINE_BOOL(banjo_renut_real_occlusion, true, "Banjo/Enhancements",
                    "Occlusion queries answered by the GPU's samples-passed counter, so light glows behind a wall are "
                    "hidden as on the console (render/occlusion.cpp, from Kameo: RePowered). Off: every query answers "
                    "1000 samples, as Xenia. Changes the picture, towards the original")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace renut::gpu {
namespace {

constexpr u32 kFakeSamples = 1000;
bool g_query_open = false;  // render thread

#if defined(PLUME_DEKO3D)
constexpr u32 kReports = 4096;  // far more than a frame's queries: a slot is only reused long after it was read
constexpr auto kTimeout = std::chrono::milliseconds(500);

struct QueryState {
  uint64_t end_seq = 0;     // the game's latest end
  uint64_t report_seq = 0;  // the latest end the render thread has recorded, into 'report'
  uint64_t result_seq = 0;  // the latest end 'result' answers
  u32 report = 0;
  u32 result = 0;
  float samples_per_host_sample = 1.0f;
  std::chrono::steady_clock::time_point end_time{};
};

std::mutex g_mutex;
std::unordered_map<u32, QueryState> g_queries;
uint64_t g_seq = 0;
std::unique_ptr<plume::RenderQueryPool> g_pool;  // render thread; read through g_pool_ready after that
std::atomic<bool> g_pool_ready{false};
u32 g_next_report = 0;

std::atomic<uint64_t> g_answered{0}, g_answered_zero{0}, g_timed_out{0};

bool On() {
  static const bool on = REXCVAR_GET(banjo_renut_real_occlusion) && IsDeko3D();
  return on;
}

plume::RenderCommandList *ListLocked() {
  auto &s = State();
  if (!Video::EnsureCommandListLocked())
    return nullptr;
  if (!g_pool) {
    g_pool = s.device->createQueryPool(kReports);
    if (!g_pool)
      return nullptr;
    g_pool_ready.store(true, std::memory_order_release);
  }
  return s.command_list;
}

void BeginLocked() {
  if (!On())
    return;
  if (auto *list = ListLocked())
    plume::Deko3DResetSamplesPassed(list);
}

void EndLocked(u32 query, uint64_t seq) {
  if (!On())
    return;
  auto *list = ListLocked();
  if (!list)
    return;
  const u32 report = g_next_report++ % kReports;
  plume::Deko3DClearReport(g_pool.get(), report);
  plume::Deko3DReportSamplesPassed(list, g_pool.get(), report);
  const float scale = ResolutionScale().Factor();
  std::lock_guard lock(g_mutex);
  QueryState &state = g_queries[query];
  if (state.report_seq < seq) {
    state.report_seq = seq;
    state.report = report;
    state.samples_per_host_sample = 1.0f / (scale * scale);
  }
}
#endif

} // namespace

void OcclusionQueryIssue(u32 device_va, u32 query, u32 flags) {
#if defined(PLUME_DEKO3D)
  if (IsRecordingCommandBuffer(device_va))
    return;
  // What a draw of this device has still to queue comes before the query's own end.
  FlushDeferredVertices(device_va);
  if (flags & 2) {
    render_thread::Queue([] {
      g_query_open = true;
      BeginLocked();
    });
  } else if (flags & 1) {
    uint64_t seq;
    {
      std::lock_guard lock(g_mutex);
      seq = ++g_seq;
      QueryState &state = g_queries[query];
      state.end_seq = seq;
      state.end_time = std::chrono::steady_clock::now();
    }
    render_thread::Queue([query, seq] {
      g_query_open = false;
      EndLocked(query, seq);
    });
  }
#else
  (void)device_va;
  (void)query;
  (void)flags;
#endif
}

bool OcclusionQueryOpenLocked() { return g_query_open; }

bool OcclusionQueryResult(u32 query, u32 *samples) {
#if defined(PLUME_DEKO3D)
  if (On()) {
    std::unique_lock lock(g_mutex);
    const auto it = g_queries.find(query);
    if (it != g_queries.end()) {
      QueryState &state = it->second;
      if (state.result_seq < state.end_seq) {
        uint64_t count = 0;
        if (state.report_seq == state.end_seq && g_pool_ready.load(std::memory_order_acquire) &&
            plume::Deko3DReadReport(g_pool.get(), state.report, &count)) {
          state.result_seq = state.end_seq;
          state.result = u32(std::min(std::ceil(double(count) * state.samples_per_host_sample), 4294967295.0));
          const u64 answered = g_answered.fetch_add(1, std::memory_order_relaxed) + 1;
          g_answered_zero.fetch_add(state.result == 0, std::memory_order_relaxed);
          if (answered % 4096 == 0)
            RGPU_INFO("[kameo] occlusion queries (banjo_renut_real_occlusion): {} answered, {} of them 0 samples, {} timed out",
                      answered, g_answered_zero.load(std::memory_order_relaxed), g_timed_out.load(std::memory_order_relaxed));
        } else if (std::chrono::steady_clock::now() - state.end_time < kTimeout) {
          return false;
        } else {
          // Never reached the GPU, or its report is slow: its last answer, so the game goes on to its next query.
          state.result_seq = state.end_seq;
          g_timed_out.fetch_add(1, std::memory_order_relaxed);
        }
      }
      *samples = state.result;
      return true;
    }
  }
#else
  (void)query;
#endif
  *samples = kFakeSamples;
  return true;
}

} // namespace renut::gpu
