/**
 * @file    gpu/d3d/hooks/kameo_d3d.cpp
 * @brief   kameo-nx (2026-11-02): Renut's hooks at Kameo's Direct3D (title update 2, XDK 2.0.2135, D3D9LTCG).
 *
 * The game's D3D is compiled into the game, so its setters write its own device directly and they stay: only the calls
 * that would reach a GPU are replaced here (draws, clears, resolves, the swap, the ring, the fence waits), as Kameo:
 * RePowered's renderer does (src/gpu/hooks, MaxDeadBear and BiRabittoh, MIT). Renut's draw path then reads the game's device
 * (d3d/d3d.h) exactly as it reads N&B's. docs/kameo-notes.md, "Renut on an inlined D3D".
 *
 * Addresses: RePowered's names at the base game, through its tools/tu/maps/map.json, each read in this image; or paired with
 * zuma-nx's (the same XDK) by tools/d3d_propagate.py (out/renut_map/zuma_to_kameo.json).
 */
#include <algorithm>
#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>
#include <unordered_map>

#include <xxhash.h>

#include <rex/hook.h>
#include <rex/ppc.h>
#include <rex/types.h>

#include "profiler/profiler.h"
#include "renut_engine/gpu/backend/queued.h"
#include "renut_engine/gpu/backend/render_thread.h"
#include "renut_engine/gpu/backend/video.h"
#include "renut_engine/gpu/banjo_renut_select.h"
#include "renut_engine/gpu/d3d/d3d.h"
#include "renut_engine/gpu/d3d/d3d_native.h"
#include "renut_engine/gpu/guest/guest_memory.h"
#include "renut_engine/gpu/guest/guest_records.h"
#include "renut_engine/gpu/guest/resources.h"
#include "renut_engine/gpu/log.h"
#include "renut_engine/gpu/render/draw.h"
#include "renut_engine/gpu/render/surfaces.h"

extern std::atomic<uint64_t> g_banjo_game_frames;  // kameo_keys.cpp: frames presented

using namespace renut::gpu;

namespace {

u32 Word(u32 va) {
  const auto *p = GuestPtr<const be_u32>(va);
  return p ? u32(*p) : 0;
}

void SetWord(u32 va, u32 value) {
  if (auto *p = GuestPtr<be_u32>(va))
    *p = value;
}

bool Ready() {
  auto &s = State();
  return s.host_ready && !s.shutting_down.load(std::memory_order_acquire);
}

// ---------------------------------------------------------------------------
// Vertex declarations: the game's objects, read on the game thread
// ---------------------------------------------------------------------------

// The game reuses a declaration's address for the next mesh (RePowered, vertex_declaration.cpp), and builds the device's
// own at +0x3010 in place, so a record is by content. The draw leaves its record's id in the device snapshot.
constexpr u32 kDeclarationCount = 8;       // the element count
constexpr u32 kDeclarationElements = 36;   // 12 bytes each
constexpr u32 kMaxDeclarationIds = 4096;

std::mutex g_declaration_mutex;
std::unordered_map<uint64_t, u32> g_declaration_ids;
std::deque<VertexDeclarationRecord> g_declaration_records;
std::atomic<VertexDeclarationRecord *> g_declaration_by_id[kMaxDeclarationIds];
// This thread's last answer: a draw mostly keeps its declaration.
thread_local u32 t_last_declaration_va = 0;
thread_local uint64_t t_last_declaration_hash = 0;
thread_local u32 t_last_declaration_id = 0;

u32 DeclarationIdOf(u32 declaration_va) {
  if (!declaration_va)
    return 0;
  const u32 count = Word(declaration_va + kDeclarationCount);
  if (!count || count > 32)
    return 0;
  const u8 *elements = GuestPtr<const u8>(declaration_va + kDeclarationElements);
  if (!elements)
    return 0;
  const uint64_t hash = XXH3_64bits(elements, size_t(count) * 12);
  if (declaration_va == t_last_declaration_va && hash == t_last_declaration_hash)
    return t_last_declaration_id;
  std::lock_guard lock(g_declaration_mutex);
  u32 &id = g_declaration_ids[hash];
  if (!id && g_declaration_records.size() + 1 < kMaxDeclarationIds) {
    VertexDeclarationRecord &record = g_declaration_records.emplace_back();
    const auto *guest = reinterpret_cast<const GuestVertexElement *>(elements);
    for (u32 i = 0; i < count; ++i) {
      if (u32(guest[i].Stream) == 0xFF)  // D3DDECL_END
        break;
      record.elements.push_back(guest[i]);
    }
    BuildDeclarationLayout(record);
    id = u32(g_declaration_records.size());
    g_declaration_by_id[id].store(&record, std::memory_order_release);
  }
  t_last_declaration_va = declaration_va;
  t_last_declaration_hash = hash;
  t_last_declaration_id = id;
  return id;
}

// ---------------------------------------------------------------------------
// What the game's flush would do, on the device, before a draw is taken
// ---------------------------------------------------------------------------

// The device's vertex fetch constants: the game's SetStreamSource keeps {offset, buffer} and its lazy flush
// (sub_820D5EA8, called by the draw's flush sub_820D7128) builds them only then, and the flush is what these hooks replace.
// Word for word as it does, for the streams its declaration names (declaration +12, the highest).
void SyncStreams(u32 device, u32 declaration) {
  if (!declaration)
    return;
  const u32 highest = Word(declaration + 12);
  auto *bytes = GuestPtr<u8>(device);
  for (u32 n = 0; n <= highest && n < d3d::kMaxStreams; ++n) {
    const u32 buffer = u32(*reinterpret_cast<const be_u32 *>(bytes + d3d::StreamSourceOffset(n)));
    if (!buffer)
      continue;
    const u32 offset = u32(*reinterpret_cast<const be_u32 *>(bytes + d3d::kDeviceOffsetStreamSources + n * 8));
    const auto *vb = GuestPtr<const d3d::VertexBuffer>(buffer);
    if (!vb)
      continue;
    const u32 cpu = u32(vb->Format.dword[0]) + offset;
    const u32 w0 = (cpu & 0x1FFFFFFFu) + (((cpu >> 20) + 0x200u) & 0x1000u);
    const u32 w1 = ((u32(vb->Format.dword[1]) - offset) & ~0x30000000u) | 0x10000000u;
    const u32 at = u32(offsetof(d3d::Device, m_Constants) + offsetof(d3d::Constants, VertexFetch)) + d3d::StreamFetchIndex(n) * 8;
    *reinterpret_cast<be_u32 *>(bytes + at) = w0;
    *reinterpret_cast<be_u32 *>(bytes + at + 4) = w1;
  }
}

// The device's first hook: the renderer learns it (the game's own Direct3D_CreateDevice made it).
void NoteDevice(u32 device) {
  static std::atomic<u32> known{0};
  if (known.load(std::memory_order_relaxed) == device)
    return;
  known.store(device, std::memory_order_relaxed);
  records::Init();
  Video::SetGuestDevice(device);
  SetWord(device + d3d::kDeviceOffsetPredication, ~0u);
  SetWord(device + d3d::kDeviceOffsetCommandBuffer, 0);
  RGPU_INFO("[kameo] the game's device at {:#010x}: read back {:#010x}, fence {}", device,
            Word(device + d3d::kDeviceOffsetFenceCompletion), Word(device + d3d::kDeviceOffsetFenceTag));
}

// Before any capture of the device: its words the snapshot reads that the game keeps elsewhere.
void Prepare(u32 device) {
  NoteDevice(device);
  SetWord(device + d3d::kKameoVertexShaderMirror, Word(device + d3d::kGameDeviceVertexShader));
  const u32 declaration = Word(device + d3d::kGameDeviceVertexDeclaration);
  SetWord(device + d3d::kKameoDeclarationId, DeclarationIdOf(declaration));
  SyncStreams(device, declaration);
}

bool ReadRect(u32 va, ClearRect &rect) {
  const auto *p = GuestPtr<const be_u32>(va);
  if (!p)
    return false;
  rect = {i32(u32(p[0])), i32(u32(p[1])), i32(u32(p[2])), i32(u32(p[3]))};
  return true;
}

bool ReadVector4(u32 va, float out[4]) {
  const auto *p = GuestPtr<const be_u32>(va);
  if (!p)
    return false;
  for (int i = 0; i < 4; ++i) {
    const u32 bits = p[i];
    std::memcpy(&out[i], &bits, 4);
  }
  return true;
}

constexpr u32 kVerticesScratchSize = 4u << 20;
u32 VerticesScratch() {
  static const u32 scratch = records::AllocPhysicalBlock(kVerticesScratchSize);
  return scratch;
}

}  // namespace

namespace renut::gpu {
VertexDeclarationRecord *KameoDeclarationById(u32 id) {
  return id < kMaxDeclarationIds ? g_declaration_by_id[id].load(std::memory_order_acquire) : nullptr;
}
}  // namespace renut::gpu

// void D3DDevice_DrawVertices(D3DDevice*, D3DPRIMITIVETYPE, UINT StartVertex, UINT VertexCount)
BANJO_RENUT_HOOK_RAW(sub_820D0BD0) {
  const u32 device = ctx.r3.u32;
  if (!Ready())
    return;
  Prepare(device);
  DrawCall call;
  call.primitive = ctx.r4.u32;
  call.start_vertex = ctx.r5.u32;
  call.count = ctx.r6.u32;
  QueueDraw(device, call, 0, 0);
}

// void D3DDevice_DrawIndexedVertices(D3DDevice*, D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT StartIndex, UINT IndexCount)
BANJO_RENUT_HOOK_RAW(sub_820D0E10) {
  const u32 device = ctx.r3.u32;
  if (!Ready())
    return;
  Prepare(device);
  DrawCall call;
  call.primitive = ctx.r4.u32;
  call.indexed = true;
  call.base_vertex = i32(ctx.r5.u32);
  call.start_index = ctx.r6.u32;
  call.count = ctx.r7.u32;
  QueueDraw(device, call, 0, 0);
}

// void D3DDevice_DrawVerticesUP(D3DDevice*, D3DPRIMITIVETYPE, UINT VertexCount, const void *pData, UINT Stride)
BANJO_RENUT_HOOK_RAW(sub_820D0860) {
  const u32 device = ctx.r3.u32, count = ctx.r5.u32, data = ctx.r6.u32, stride = ctx.r7.u32;
  if (!Ready() || !data)
    return;
  Prepare(device);
  DrawCall call;
  call.primitive = ctx.r4.u32;
  call.count = count;
  call.up_vertices = GuestPtr<const u8>(data);
  call.up_stride = stride;
  call.up_vertex_count = count;
  QueueDraw(device, call, count * stride, 0);
}

// void *D3DDevice_BeginVertices(D3DDevice*, D3DPRIMITIVETYPE, UINT VertexCount, UINT Stride): the 2D icon system and the
// Bink blit (RePowered). The game writes the vertices and ends inline, restoring its write pointer from +0x3544, so the
// draw's state is taken now and its vertices at the device's next hook.
BANJO_RENUT_HOOK_RAW(sub_820D05C0) {
  const u32 device = ctx.r3.u32, count = ctx.r5.u32, stride = ctx.r6.u32;
  const u64 bytes = u64(count) * stride;
  const u32 scratch = bytes && bytes <= kVerticesScratchSize ? VerticesScratch() : 0;
  if (!scratch) {
    ctx.r3.u64 = 0;
    return;
  }
  Prepare(device);
  SetWord(device + d3d::kDeviceOffsetVerticesRingEnd, Word(device + u32(offsetof(d3d::Device, m_pRing))));
  SetWord(device + d3d::kDeviceOffsetVerticesData, scratch);
  SetWord(device + d3d::kDeviceOffsetVerticesDwords, u32(bytes >> 2));
  DrawCall call;
  call.primitive = ctx.r4.u32;
  call.count = count;
  call.up_vertices = GuestPtr<const u8>(scratch);
  call.up_stride = stride;
  call.up_vertex_count = count;
  if (Ready())
    QueueDeferredDraw(device, call, call.up_vertices, u32(bytes));
  else
    FlushDeferredVertices(device);
  ctx.r3.u64 = scratch;
}

// void D3DDevice_ClearF(D3DDevice*, DWORD Flags, const D3DRECT *pRect, const D3DVECTOR4 *pColor, float Z, DWORD Stencil):
// the original only builds packets (RePowered). Z is f1; it takes r7's slot, so Stencil arrives in r8.
BANJO_RENUT_HOOK_RAW(sub_820C6C80) {
  const u32 device = ctx.r3.u32;
  if (!Ready())
    return;
  Prepare(device);
  ClearRect rect;
  const bool has_rect = ctx.r5.u32 && ReadRect(ctx.r5.u32, rect);
  float color[4] = {0, 0, 0, 0};
  if (ctx.r6.u32)
    ReadVector4(ctx.r6.u32, color);
  QueueClear(device, ctx.r4.u32, has_rect ? &rect : nullptr, color, float(ctx.f1.f64), ctx.r8.u32);
}

// void D3DDevice_Resolve(D3DDevice*, DWORD Flags, const D3DRECT *pSourceRect, D3DBaseTexture *pDestTexture,
//     const D3DPOINT *pDestPoint, UINT DestLevel, UINT DestSliceOrFace, const D3DVECTOR4 *pClearColor, float ClearZ,
//     DWORD ClearStencil, const D3DRESOLVE_PARAMETERS *pParameters): only packets too, and for a resolve that clears a
// nested ClearF, whose work QueueResolve does from the flags (0x100 colour, 0x200 depth and stencil). ClearStencil is on the
// caller's stack, the slot after ClearZ's (+0x5C; D3DDevice_Present writes it there).
BANJO_RENUT_HOOK_RAW(sub_820C5028) {
  const u32 device = ctx.r3.u32, dest = ctx.r6.u32;
  if (!Ready() || !dest)
    return;
  Prepare(device);
  ClearRect rect;
  const bool has_rect = ctx.r5.u32 && ReadRect(ctx.r5.u32, rect);
  i32 dest_x = 0, dest_y = 0;
  if (const auto *p = GuestPtr<const be_u32>(ctx.r7.u32)) {
    dest_x = i32(u32(p[0]));
    dest_y = i32(u32(p[1]));
  }
  float color[4] = {0, 0, 0, 0};
  const bool has_color = ctx.r10.u32 && ReadVector4(ctx.r10.u32, color);
  const u32 stencil = Word(ctx.r1.u32 + 0x5C);
  QueueResolve(device, ctx.r4.u32, has_rect ? &rect : nullptr, dest, dest_x, dest_y, ctx.r8.u32, ctx.r9.u32,
               has_color ? color : nullptr, float(ctx.f1.f64), stencil);
}

// D3DDevice_CreateVertexShader / CreatePixelShader(const DWORD *pFunction): the game's own, then the host side.
BANJO_RENUT_HOOK_RAW(sub_820CA548) {
  const u32 function = ctx.r3.u32;
  __imp__sub_820CA548(ctx, base);
  if (ctx.r3.u32 && function)
    RegisterGameShader(ctx.r3.u32, GuestPtr<const u8>(function), false);
}
BANJO_RENUT_HOOK_RAW(sub_820CA1D8) {
  const u32 function = ctx.r3.u32;
  __imp__sub_820CA1D8(ctx, base);
  if (ctx.r3.u32 && function)
    RegisterGameShader(ctx.r3.u32, GuestPtr<const u8>(function), true);
}

// D3DDevice_SynchronizeToPresentationInterval(D3DDevice*): pacing is the host swap chain's.
BANJO_RENUT_HOOK_RAW(sub_820D16F0) {
  FlushDeferredVertices(ctx.r3.u32);
}

// HRESULT D3DQuery_GetData(D3DQuery*, void *pData, DWORD Size, DWORD Flags): a hardware occlusion query (type 9) has no GPU
// behind it: every sample visible until its graft (docs/kameo-notes.md). Others are the game's.
constexpr u32 kQueryTypeOcclusion = 9;
constexpr u32 kFakeOcclusionSamples = 1000;
BANJO_RENUT_HOOK_RAW(sub_820D27F0) {
  if (!ctx.r3.u32 || Word(ctx.r3.u32 + 4) != kQueryTypeOcclusion) {
    __imp__sub_820D27F0(ctx, base);
    return;
  }
  if (ctx.r4.u32 && ctx.r5.u32 >= 4)
    SetWord(ctx.r4.u32, kFakeOcclusionSamples);
  ctx.r3.u64 = 0;
}
