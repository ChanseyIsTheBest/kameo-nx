#!/usr/bin/env python3
"""kameo-nx (2026-11-02): moves Renut's renderer to Kameo's Direct3D, which is the game's own: D3D9LTCG compiles most of
D3D into the game, so its setters write its real device (XDK 2.0.2135 plus the update's 16 bytes from 0x34D8), and Renut
reads that device (docs/kameo-notes.md, "Renut on an inlined D3D").

Each edit is an exact (file, old, new) text pair; a pair whose new text is already there is skipped (idempotent), one whose
old text is missing is reported. Every existing hook line goes unhooked (no address of another game stays in the code);
app/src/renut_engine/gpu/d3d/hooks/kameo_d3d.cpp registers the hooks this game has.

  python -I tools/kameo_renut_port.py [--check]
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GPU = os.path.join(ROOT, 'app', 'src', 'renut_engine', 'gpu')
HOOKS = os.path.join(GPU, 'd3d', 'hooks')

DEVICE_OLD_START = '// Public D3DDevice (0x2B00 bytes in N&B\'s XDK) followed by the CDevice-private'
DEVICE_NEW = '''// kameo-nx (2026-11-02): the device is the game's own, XDK 2.0.2135's layout (with the title update's 16 bytes from 0x34D8):
// its setters are compiled into the game (D3D9LTCG), so this is what they write. From the game's flush sub_820D7128 and
// setters, RePowered's src/gpu/guest_device.h and guest_state.h: the register image's blocks are N&B's order, the packets
// 0x440 further on than N&B's, the private words between them.
constexpr u32 kDeviceAllocSize = 0x5700;  // not allocated here: the game's own Direct3D_CreateDevice makes the device
constexpr u32 kDeviceAlignment = 0x80;

struct Device {
  be_u32 m_pRing;                     // +0x0000  the command write pointer
  be_u32 m_pRingHardEnd;              // +0x0004  its segment's end
  be_u32 m_pRingLimit;                // +0x0008  that less 160: past it, KickOff
  be_u32 m_ReferenceCount;            // +0x000C
  be_u64 m_Pending[5];                // +0x0010  dirty masks: vertex constants, pixel constants, fetch and destination, packets, misc
  u8 pad_0038[0x60 - 0x38];           // +0x0038
  be_u32 m_SetRenderStateCall[97];    // +0x0060
  be_u32 m_SetSamplerStateCall[20];   // +0x01E4
  be_u32 m_GetRenderStateCall[97];    // +0x0234
  be_u32 m_GetSamplerStateCall[20];   // +0x03B8
  u8 pad_0408[0x480 - 0x408];         // +0x0408
  Constants m_Constants;              // +0x0480
  be_f32 m_ClipPlanes[6][4];          // +0x2820
  u8 private_2880[0x2CC0 - 0x2880];   // +0x2880  fences, flags, the declaration pointer (+0x2C90)
  DestinationPacket m_DestinationPacket;  // +0x2CC0
  WindowPacket m_WindowPacket;        // +0x2D00
  ValuesPacket m_ValuesPacket;        // +0x2D0C
  ProgramPacket m_ProgramPacket;      // +0x2D60
  ControlPacket m_ControlPacket;      // +0x2D74
  u8 m_TessellatorPacket[84];         // +0x2DA4
  u8 m_MiscPacket[152];               // +0x2DF8
  u8 m_PointPacket[32];               // +0x2E90
  u8 private_2EB0[kDeviceAllocSize - 0x2EB0];  // +0x2EB0
};
static_assert(sizeof(Device) == kDeviceAllocSize);
static_assert(offsetof(Device, m_pRing) == 0);
static_assert(offsetof(Device, m_ReferenceCount) == 0xC);
static_assert(offsetof(Device, m_Pending) == 0x10);
static_assert(offsetof(Device, m_SetRenderStateCall) == 0x60);
static_assert(offsetof(Device, m_SetSamplerStateCall) == 0x1E4);
static_assert(offsetof(Device, m_GetRenderStateCall) == 0x234);
static_assert(offsetof(Device, m_GetSamplerStateCall) == 0x3B8);
static_assert(offsetof(Device, m_Constants) == 0x480);
static_assert(offsetof(Device, m_ClipPlanes) == 0x2820);
static_assert(offsetof(Device, m_DestinationPacket) == 0x2CC0);
static_assert(offsetof(Device, m_WindowPacket) == 0x2D00);
static_assert(offsetof(Device, m_ValuesPacket) == 0x2D0C);
static_assert(offsetof(Device, m_ProgramPacket) == 0x2D60);
static_assert(offsetof(Device, m_ControlPacket) == 0x2D74);
static_assert(offsetof(Device, m_TessellatorPacket) == 0x2DA4);
static_assert(offsetof(Device, m_MiscPacket) == 0x2DF8);
static_assert(offsetof(Device, m_PointPacket) == 0x2E90);

// The words Renut's own hooks keep in the device (kameo_d3d.cpp), inside the snapshot, none of which the game reads or
// writes (tools/d3d_disp.py over the whole code range: no D-form access to 0x3254-0x325F):
//   the id of the vertex declaration's record (the game's object is read on the game thread: it reuses addresses),
//   the vertex shader (the game keeps it at +0x4FE8, past the snapshot),
//   the predication (no tiling in this game: always ~0).
constexpr u32 kKameoDeclarationId = 0x3254;
constexpr u32 kKameoVertexShaderMirror = 0x3258;
constexpr u32 kKameoPredication = 0x325C;
// The game's own words these stand for.
constexpr u32 kGameDeviceVertexDeclaration = 0x2C90;  // SetVertexDeclaration sub_820CA848
constexpr u32 kGameDeviceVertexShader = 0x4FE8;       // SetVertexShader sub_820CA7E0

constexpr u32 kDeviceOffsetCreationThreadId = 0x2888;
constexpr u32 kDeviceOffsetIsTitleProcess = 0x288C;
constexpr u32 kDeviceOffsetBehaviorFlags = 0x5040;  // not read here

// Bound objects the Set* calls store in the private area (guest VAs).
constexpr u32 kDeviceOffsetVertexDeclaration = kKameoDeclarationId;
constexpr u32 kDeviceOffsetIndexBuffer = 0x30F4;        // SetIndices
constexpr u32 kDeviceOffsetRenderTargets = 0x30F8;      // SetRenderTarget, [4]
constexpr u32 kDeviceOffsetDepthStencil = 0x3108;       // SetDepthStencilSurface
constexpr u32 kDeviceOffsetStreamSources = 0x310C;      // SetStreamSource, [16] of {offset, buffer}
constexpr u32 kDeviceOffsetStreamStrides = 0x3190;      // SetStreamSource, u8[16], stride / 4
constexpr u32 kDeviceOffsetTextures = 0x31A0;           // SetTexture, [kTextureFetchSlots]
constexpr u32 kDeviceOffsetViewport = 0x3208;           // D3DVIEWPORT9: four words, then two floats
constexpr u32 kDeviceOffsetScissorRect = 0x3220;        // SetScissorRect's RECT
constexpr u32 kDeviceOffsetPixelShader = 0x3290;        // SetPixelShader
constexpr u32 kDeviceOffsetVertexShader = kKameoVertexShaderMirror;
// kameo-nx: the stream's vertex buffer (XDK 2.0.2135 keeps {offset, buffer} per stream; N&B the buffer alone).
constexpr u32 StreamSourceOffset(u32 stream) {
  return kDeviceOffsetStreamSources + 4 + stream * 8;
}
// In m_TessellatorPacket, which holds registers 0x2280 on: VGT_HOS_CNTL
// (tessellation mode in bits 0-1: 0 discrete, 1 continuous, 2 adaptive) and
// the float VGT_HOS_MAX_TESS_LEVEL / VGT_HOS_MIN_TESS_LEVEL.
constexpr u32 kDeviceOffsetTessellationControl = 0x2DB8;
constexpr u32 kDeviceOffsetMaxTessellationLevel = 0x2DBC;
constexpr u32 kDeviceOffsetMinTessellationLevel = 0x2DC0;
constexpr u32 kDeviceOffsetPredication = kKameoPredication;
// D3DCommandBuffer being recorded: none in this game; a word of the device that stays 0.
constexpr u32 kDeviceOffsetCommandBuffer = 0x3250;
// The leading part of the device the draw, clear and resolve paths read; a copy of it stands in for the device on the
// render thread.
constexpr u32 kDeviceSnapshotSize = 0x32A0;
static_assert(kDeviceOffsetVertexShader + 4 <= kDeviceSnapshotSize);
static_assert(kDeviceOffsetPredication + 4 <= kDeviceSnapshotSize);
static_assert(kDeviceOffsetPixelShader + 4 <= kDeviceSnapshotSize);
// The write pointer the inline end of BeginVertices restores (RePowered's kBeginVerticesResume), the vertex pointer and its
// dword count after it (with the update's 16 bytes).
constexpr u32 kDeviceOffsetVerticesRingEnd = 0x3544;
constexpr u32 kDeviceOffsetVerticesData = 0x3548;
constexpr u32 kDeviceOffsetIndicesData = 0x354C;
constexpr u32 kDeviceOffsetVerticesDwords = 0x3550;
constexpr u32 kDeviceOffsetIndicesDwords = 0x3554;
// The ring's segment words StartNewSegment lays out (RePowered's, with the update's 16 bytes).
constexpr u32 kDeviceOffsetSegmentPredication = 0x36B0;
constexpr u32 kDeviceOffsetSegmentStart = 0x36BC;
constexpr u32 kDeviceOffsetSegmentDataBytes = 0x36C0;
constexpr u32 kDeviceOffsetPrimaryRingMask = 0x36A0;
// Fences: the pointer to the word the GPU writes and the CPU's next fence (BlockOnFence sub_820CFC08; SetFence
// sub_820CF838 adds 2).
constexpr u32 kDeviceOffsetFenceCompletion = 0x2890;
constexpr u32 kDeviceOffsetFenceTag = 0x289C;
constexpr u32 kDeviceOffsetInKickOff = 0x34D0;  // KickOff, BlockOnFence: nonzero while a kick runs
constexpr u32 kDeviceOffsetSwapCount = 0x4098;  // not used by the game's device here (the Swap hook does not count)
constexpr u32 kDeviceOffsetSwapFenceTag = 0x2AB0;     // not used here
constexpr u32 kDeviceOffsetPresentFenceTag = 0x3A44;  // not used here
constexpr u32 kDeviceOffsetSwapFlags = 0x28C1;
constexpr u8 kSwapFlagPending = 0x10;

constexpr u32 kMaxStreams = 16;
constexpr u32 kMaxRenderTargets = 4;

// Stream n's vertex fetch constant is VertexFetch[17 - n] (Xenos index 95 - n).
constexpr u32 StreamFetchIndex(u32 stream) {
  return kVertexFetchConstants - 1 - stream;
}

// Common bits of an index buffer: bit 31 32-bit indices, 29-30 endian mode.
constexpr u32 kIndexBufferCommon32Bit = 0x80000000u;
constexpr u32 IndexBufferEndian(u32 common) {
  return (common >> 29) & 3;
}

// What Renut reads of a surface (N&B's D3DSURFACE after its Resource header).
struct SurfaceDesc {
  u32 SurfaceInfo;  // RB_SURFACE_INFO: pitch 0-13, msaa 16-17
  u32 ColorInfo;    // RB_COLOR_INFO / RB_DEPTH_INFO: EDRAM base, format at 16
  u32 HiControl;
  u32 Size;         // (height - 1) << 3 | (width - 1) << 18, N&B's form
  u32 Format;       // D3DFORMAT's low 6 bits are all that is read
  u32 EdramSize;    // bytes (5120 per tile)
};

// kameo-nx: XDK 2.0.2135's surface (the creator sub_820C9088): a resource head, a texture fetch constant at +16 (its
// word 2 the size, width - 1 | (height - 1) << 13, word 1 the format in its low 6 bits), the EDRAM size at +40,
// RB_SURFACE_INFO at +48, RB_COLOR_INFO / RB_DEPTH_INFO at +52, the HiZ control at +56.
inline bool ReadSurfaceDesc(const u8 *surface, SurfaceDesc &out) {
  if (!surface)
    return false;
  auto word = [surface](u32 offset) -> u32 { return *reinterpret_cast<const be_u32 *>(surface + offset); };
  const u32 size = word(24);
  out.SurfaceInfo = word(48);
  out.ColorInfo = word(52);
  out.HiControl = word(56);
  out.Size = ((size >> 13) & 0x1FFF) << 3 | (size & 0x1FFF) << 18;
  out.Format = word(20) & 0x3F;
  out.EdramSize = word(40);
  return true;
}

inline u32 SurfaceWidth(u32 size) {
  return ((size >> 18) & 0x3FFF) + 1;
}
inline u32 SurfaceHeight(u32 size) {
  return ((size >> 3) & 0x7FFF) + 1;
}
// D3DFORMAT low 6 bits of the two depth formats (D24S8, D24FS8).
inline bool IsDepthFormat(u32 d3d_format) {
  const u32 f = d3d_format & 0x3F;
  return f == 22 || f == 23;
}

} // namespace renut::gpu::d3d
'''

RES_OLD_A = '''struct Resource {
  be_u32 Common;          // +0x00
  be_u32 ReferenceCount;  // +0x04
  be_u32 Fence;           // +0x08
  be_u32 ReadFence;       // +0x0C
  be_u32 Identifier;      // +0x10
  be_u32 BaseFlush;       // +0x14
};
static_assert(sizeof(Resource) == 24);
'''
RES_NEW_A = '''// kameo-nx: XDK 2.0.2135's resource head is 12 bytes: Common (the reference count in its low byte, the type in bits 16-18,
// D3DResource_GetType sub_820D51C0), the fence, and a word (zuma-nx's notes). The renderer's own records keep N&B's head
// (RecordResource): the game's objects are never those.
struct Resource {
  be_u32 Common;     // +0x00
  be_u32 Fence;      // +0x04
  be_u32 BaseFlush;  // +0x08
};
static_assert(sizeof(Resource) == 12);

struct RecordResource {
  be_u32 Common;          // +0x00
  be_u32 ReferenceCount;  // +0x04
  be_u32 Fence;           // +0x08
  be_u32 ReadFence;       // +0x0C
  be_u32 Identifier;      // +0x10
  be_u32 BaseFlush;       // +0x14
};
static_assert(sizeof(RecordResource) == 24);
'''

EDITS = [
    # Resource heads: XDK 2.0.2135's, for the game's objects (RePowered: fetch at object + 16 for textures, + 12 for vertex
    # buffers, index address at + 12).
    ('d3d/d3d.h', RES_OLD_A, RES_NEW_A),
    ('d3d/d3d.h', '''struct BaseTexture {
  Resource resource;            // +0x00
  be_u32 MipFlush;              // +0x18
  TextureFetchConstant Format;  // +0x1C
};
static_assert(sizeof(BaseTexture) == 52);
static_assert(offsetof(BaseTexture, Format) == 0x1C);
''', '''struct BaseTexture {
  Resource resource;            // +0x00
  be_u32 MipFlush;              // +0x0C
  TextureFetchConstant Format;  // +0x10
};
static_assert(sizeof(BaseTexture) == 40);
static_assert(offsetof(BaseTexture, Format) == 0x10);
'''),
    ('d3d/d3d.h', '''struct Surface {
  Resource resource;   // +0x00
  be_u32 word[6];      // +0x18 union (surface/depth info, size, format)
};
static_assert(sizeof(Surface) == 48);
''', '''struct Surface {
  be_u32 word[16];  // kameo-nx: 64 bytes in XDK 2.0.2135 (ReadSurfaceDesc)
};
static_assert(sizeof(Surface) == 64);
'''),
    ('d3d/d3d.h', '''struct VertexBuffer {
  Resource resource;           // +0x00
  VertexFetchConstant Format;  // +0x18
};
static_assert(sizeof(VertexBuffer) == 32);
''', '''struct VertexBuffer {
  Resource resource;           // +0x00
  VertexFetchConstant Format;  // +0x0C
};
static_assert(sizeof(VertexBuffer) == 20);
'''),
    ('d3d/d3d.h', '''struct IndexBuffer {
  Resource resource;  // +0x00
  be_u32 Address;     // +0x18
  be_u32 Size;        // +0x1C
};
static_assert(sizeof(IndexBuffer) == 32);
''', '''struct IndexBuffer {
  Resource resource;  // +0x00
  be_u32 Address;     // +0x0C
  be_u32 Size;        // +0x10
};
static_assert(sizeof(IndexBuffer) == 20);
'''),
    ('d3d/d3d.h', '''struct Shader {
  Resource resource;
};
static_assert(sizeof(Shader) == 24);
''', '''struct Shader {
  RecordResource resource;
};
static_assert(sizeof(Shader) == 24);
'''),
    # The field table (draw.h): the same fields at this layout.
    ('render/draw.h', '''  if (at < 0x2880)
    return kFieldBools;
  if (at < 0x28C0)
    return kFieldDest;
  if (at < 0x2968)
    return kFieldViewPackets;
  if (at >= 0x29C0 && at < 0x29C4)
    return kField29C0;
  if (at < 0x2A80)
    return kFieldMisc;
''', '''  if (at < 0x2880)
    return kFieldBools;
  // kameo-nx: XDK 2.0.2135 keeps the private words 0x2880-0x2CBF between the constants and the packets (the packets are
  // N&B's + 0x440).
  if (at < 0x2CC0)
    return kFieldUnread;
  if (at < 0x2D00)
    return kFieldDest;
  if (at < 0x2DA8)
    return kFieldViewPackets;
  if (at >= 0x2E00 && at < 0x2E04)
    return kField29C0;
  if (at < 0x2EB0)
    return kFieldMisc;
'''),
    ('render/draw.h', '''  if (at < 0x2DF8)  // katamari-nx: 0x2E04->0x2DF8
    return kFieldUnread;
  if (at >= d3d::kDeviceOffsetVertexDeclaration && at < d3d::kDeviceOffsetVertexDeclaration + 4)
    return kFieldDecl;
  if (at < 0x2EF8)  // katamari-nx: 0x2F08->0x2EF8
    return kFieldPrivate;
  if (at < d3d::kDeviceOffsetIndexBuffer)
    return kFieldTail;
''', '''  if (at >= d3d::kDeviceOffsetVertexDeclaration && at < d3d::kDeviceOffsetVertexDeclaration + 4)
    return kFieldDecl;
  if (at < 0x2F7C)
    return kFieldPrivate;
  if (at < d3d::kDeviceOffsetIndexBuffer)
    return kFieldTail;
'''),
    ('render/draw.h', '''  if (at < d3d::kDeviceOffsetTextures + 4 * d3d::kTextureFetchSlots)
    return kFieldTexWords;
  if (at < 0x317C)  // katamari-nx: 0x318C->0x317C
    return kFieldTail;
  if (at < d3d::kDeviceOffsetPixelShader)
    return kFieldScissor;
  if (at < d3d::kDeviceOffsetVertexShader + 4)
    return kFieldShaders;
  if (at < d3d::kDeviceOffsetPredication)
    return kFieldTail;
  return kFieldPredication;
''', '''  if (at < d3d::kDeviceOffsetTextures + 4 * d3d::kTextureFetchSlots)
    return kFieldTexWords;
  if (at < d3d::kDeviceOffsetScissorRect)
    return kFieldTail;
  if (at < d3d::kDeviceOffsetScissorRect + 16)
    return kFieldScissor;
  if (at >= d3d::kDeviceOffsetVertexShader && at < d3d::kDeviceOffsetVertexShader + 4)
    return kFieldShaders;
  if (at >= d3d::kDeviceOffsetPixelShader && at < d3d::kDeviceOffsetPixelShader + 4)
    return kFieldShaders;
  if (at >= d3d::kDeviceOffsetPredication && at < d3d::kDeviceOffsetPredication + 4)
    return kFieldPredication;
  return kFieldTail;
'''),
    # The delta chain's unread words.
    ('backend/queued.cpp', 'constexpr u32 kDeltaPrivateUnreadBegin = 10880, kDeltaPrivateUnreadEnd = 11776;',
     '// kameo-nx: XDK 2.0.2135\'s private words between the constants and the packets, 0x2880-0x2CBF.\n'
     'constexpr u32 kDeltaPrivateUnreadBegin = 0x2880, kDeltaPrivateUnreadEnd = 0x2CC0;'),
    ('backend/queued.cpp', 'static_assert(kDeltaReadChunks.count == 39);  // N&B (SA2 40)',
     'static_assert(kDeltaReadChunks.count == 12 + 28);  // kameo-nx: the fetch constants\' 12 and the tail\'s (0x2780-0x287F, 0x2CC0-0x329F)'),
    ('render/draw.cpp', 'static_assert(FieldsOfRange(10432, 10600 - 10432) == kFieldViewPackets && FieldOfOffset(0x29C0) == kField29C0 &&',
     'static_assert(FieldsOfRange(0x2D00, 0x2DA8 - 0x2D00) == kFieldViewPackets && FieldOfOffset(0x2E00) == kField29C0 &&  // kameo-nx'),
    ('render/draw.cpp', '  key[11] = device.U32(0x29C0);', '  key[11] = device.U32(0x2E00);  // kameo-nx: N&B\'s 0x29C0'),
    ('render/draw.cpp', '(banjo::renut_gpu::HalfPixelOffsetOn() && !(device.U32(0x29C0) & 1))',
     '(banjo::renut_gpu::HalfPixelOffsetOn() && !(device.U32(0x2E00) & 1))'),
    ('render/draw.cpp', 'static_assert(kCensusTailBegin < 10880 && 11776 < d3d::kDeviceSnapshotSize);',
     'static_assert(kCensusTailBegin < 0x2880 && 0x2CC0 < d3d::kDeviceSnapshotSize);  // kameo-nx'),
    ('render/draw.cpp', 'state = XXH3_64bits_withSeed(device_state + 11776, d3d::kDeviceSnapshotSize - 11776, state);',
     'state = XXH3_64bits_withSeed(device_state + 0x2CC0, d3d::kDeviceSnapshotSize - 0x2CC0, state);  // kameo-nx'),
    # Clears clamp to the viewport: four words here (RePowered: "reading the first four as floats made every resolve
    # viewport 0x0"), N&B's are floats.
    ('render/draw.cpp', '''  const float vx = device.F32(0x3160), vy = device.F32(0x3164);  // katamari-nx: 0x3170->0x3160, 0x3174->0x3164
  const float vw = device.F32(0x3168), vh = device.F32(0x316C);  // katamari-nx: 0x3178->0x3168, 0x317C->0x316C''',
     '''  // kameo-nx: D3DVIEWPORT9 is four words then two floats in this device.
  const float vx = float(device.U32(d3d::kDeviceOffsetViewport)), vy = float(device.U32(d3d::kDeviceOffsetViewport + 4));
  const float vw = float(device.U32(d3d::kDeviceOffsetViewport + 8)), vh = float(device.U32(d3d::kDeviceOffsetViewport + 12));'''),
    # The vertex declaration: the record of the id the draw hook left in the snapshot.
    ('render/draw.cpp', '''        g_last_declaration = FindRecord<VertexDeclarationRecord>(declaration_va, RecordKind::VertexDeclaration);
        g_last_declaration_va = g_last_declaration ? declaration_va : 0;''',
     '''        g_last_declaration = KameoDeclarationById(declaration_va);  // kameo-nx: the id the draw hook left
        g_last_declaration_va = g_last_declaration ? declaration_va : 0;'''),
    ('render/draw.cpp', '''      if (!declaration && declaration_va == Video::GuestDevice() + kDeviceDeclarationOffset)
        declaration = DeviceDeclarationLocked(device);  // katamari-nx
''', ''),
    ('guest/resources.h', '''// Drops the host side of an in-place guest shader object, if it has one.''',
     '''// kameo-nx: the record of a declaration id (d3d/hooks/kameo_d3d.cpp), null for none.
VertexDeclarationRecord *KameoDeclarationById(u32 id);
// kameo-nx: the host side of a shader object the game's own Create*Shader made from this container.
void RegisterGameShader(u32 shader_va, const u8 *container, bool pixel);

// Drops the host side of an in-place guest shader object, if it has one.'''),
    # Shaders: version 0x0E containers (zuma-nx's, XDK 2135's compiler).
    ('d3d/hooks/shader.cpp', '''// void XGRegisterVertexShader(D3DVertexShader *pShader, void *pPhysicalPart) [0x826376C8]''',
     '''// kameo-nx: the game's own Create*Shader made the object; this is its host side, keyed by the object's address, from the
// container it was given (XXH3 of the virtual part and the physical part: what the shader cache is keyed by).
void RegisterGameShaderImpl(u32 shader_va, const u8 *container, bool pixel) {
  const uint64_t hash = HashShaderContainer(container);
  const RecordKind kind = pixel ? RecordKind::PixelShader : RecordKind::VertexShader;
  const ShaderCacheEntry *const sort_entry = FindShaderEntry(hash);
  if (!sort_entry)
    CaptureMissingShader(container, pixel, hash);
  NoteShaderSortKey(shader_va, hash, ShaderSortableByHash(hash, sort_entry, pixel));
  render_thread::Queue([shader_va, hash, kind] {
    auto &shader = g_guest_shaders[shader_va];
    if (shader && shader->hash == hash && shader->kind == kind) {
      ForgetShader(shader_va);
      return;
    }
    ForgetShader(shader_va);
    if (shader) {
      std::lock_guard shader_lock(shader->host_mutex);
      ReleaseHostShader(shader.get());
    }
    shader = std::make_unique<ShaderRecord>();
    shader->kind = kind;
    shader->self_va = shader_va;
    shader->hash = hash;
    shader->entry = FindShaderEntry(hash);
    PipelineShaderLoaded(shader->entry);
  });
}

// void XGRegisterVertexShader(D3DVertexShader *pShader, void *pPhysicalPart) [0x826376C8]'''),
    ('d3d/hooks/shader.cpp', '''} // namespace

BANJO_RENUT_''', '''} // namespace

namespace renut::gpu {
void RegisterGameShader(u32 shader_va, const u8 *container, bool pixel) {
  RegisterGameShaderImpl(shader_va, container, pixel);
}
}  // namespace renut::gpu

BANJO_RENUT_'''),
    ('d3d/hooks/shader.cpp', 'if ((u32(header->flags) & 0xFFFFFF00u) != 0x102A1100u) {',
     'if ((u32(header->flags) & 0xFFFFFF00u) != 0x102A1100u && (u32(header->flags) & 0xFFFFFF00u) != 0x102A0E00u) {'),
    ('d3d/hooks/shader.cpp', 'if ((flags & 0xFFFFFF00u) != 0x102A1100u || ((flags & 1) != 0) == pixel ||',
     'if (((flags & 0xFFFFFF00u) != 0x102A1100u && (flags & 0xFFFFFF00u) != 0x102A0E00u) || ((flags & 1) != 0) == pixel ||'),
    # Surfaces: read through XDK 2.0.2135's layout.
    ('render/surfaces.cpp', '''  const auto *desc = GuestPtr<const d3d::SurfaceDesc>(surface_va + sizeof(d3d::Resource));
  if (!desc)
    return nullptr;
''', '''  // kameo-nx: from the game's own surface (XDK 2.0.2135's layout).
  d3d::SurfaceDesc desc_value;
  if (!d3d::ReadSurfaceDesc(GuestPtr<const u8>(surface_va), desc_value))
    return nullptr;
  const d3d::SurfaceDesc *const desc = &desc_value;
'''),
    ('render/draw.cpp', '''  if (const auto *desc = GuestPtr<const d3d::SurfaceDesc>(va + sizeof(d3d::Resource))) {
''', '''  d3d::SurfaceDesc desc_value;  // kameo-nx
  if (d3d::ReadSurfaceDesc(GuestPtr<const u8>(va), desc_value)) {
    const d3d::SurfaceDesc *const desc = &desc_value;
'''),
    # The native layer's device offsets, for what still runs of it (the ring, the fences).
    ('d3d/d3d_native.h', '''constexpr u32 Ring = 0x30;                 // m_pRing
constexpr u32 RingLimit = 0x34;            // m_pRingLimit
constexpr u32 RingGuarantee = 0x38;        // m_pRingGuarantee''',
     '''// kameo-nx: XDK 2.0.2135: the write pointer, its segment's hard end, and that less 160, which the game's inline writers test.
constexpr u32 Ring = 0x0;                  // m_pRing
constexpr u32 RingLimit = 0x4;             // m_pRingHardEnd
constexpr u32 RingGuarantee = 0x8;         // m_pRingLimit'''),
    ('d3d/d3d_native.h', '''constexpr u32 SetRenderStateCall = 0x40;   // [101]
constexpr u32 SetSamplerStateCall = 0x1D4; // [20]
constexpr u32 GetRenderStateCall = 0x224;  // [101]''',
     '''constexpr u32 SetRenderStateCall = 0x60;   // [97] (kameo-nx: XDK 2.0.2135)
constexpr u32 SetSamplerStateCall = 0x1E4; // [20]
constexpr u32 GetRenderStateCall = 0x234;  // [97]'''),
    ('d3d/d3d_native.h', '''constexpr u32 SurfaceInfo = 0x2880;        // m_DestinationPacket
constexpr u32 Color0Info = 0x2884;
constexpr u32 DepthInfo = 0x2888;
constexpr u32 WindowOffset = 0x28C0;       // m_WindowPacket
constexpr u32 WindowScissorTL = 0x28C4;
constexpr u32 WindowScissorBR = 0x28C8;
constexpr u32 ColorMask = 0x28DC;          // m_ValuesPacket
constexpr u32 DepthControl = 0x2934;       // m_ControlPacket
constexpr u32 BlendControl0 = 0x2938;
constexpr u32 ColorControl = 0x293C;
constexpr u32 HiControl = 0x2940;
constexpr u32 ClipControl = 0x2944;
constexpr u32 ModeControl = 0x2948;
constexpr u32 BlendControl1 = 0x2958;
constexpr u32 BlendControl2 = 0x295C;
constexpr u32 BlendControl3 = 0x2960;
constexpr u32 FenceCompletion = 0x2A90;    // -> the completed-fence word
constexpr u32 FenceTag = 0x2A9C;           // the current fence
constexpr u32 CommandBufferTagMask = 0x2AA0;  // Common bits a recorded buffer tracks
constexpr u32 TilePredication = 0x2AB4;    // SetPredication / SetCommandBufferPredication args
constexpr u32 RunPredication = 0x2AB8;
constexpr u32 Flags = 0x2ABC;              // bytes 0x2ABC-0x2ABF, see below
constexpr u32 Flags1 = 0x2ABD;
constexpr u32 Flags2 = 0x2ABE;
constexpr u32 Flags3 = 0x2ABF;''',
     '''// kameo-nx: the packets are N&B's + 0x440 here, the private words before them.
constexpr u32 SurfaceInfo = 0x2CC0;        // m_DestinationPacket
constexpr u32 Color0Info = 0x2CC4;
constexpr u32 DepthInfo = 0x2CC8;
constexpr u32 WindowOffset = 0x2D00;       // m_WindowPacket
constexpr u32 WindowScissorTL = 0x2D04;
constexpr u32 WindowScissorBR = 0x2D08;
constexpr u32 ColorMask = 0x2D1C;          // m_ValuesPacket
constexpr u32 DepthControl = 0x2D74;       // m_ControlPacket
constexpr u32 BlendControl0 = 0x2D78;
constexpr u32 ColorControl = 0x2D7C;
constexpr u32 HiControl = 0x2D80;
constexpr u32 ClipControl = 0x2D84;
constexpr u32 ModeControl = 0x2D88;
constexpr u32 BlendControl1 = 0x2D98;
constexpr u32 BlendControl2 = 0x2D9C;
constexpr u32 BlendControl3 = 0x2DA0;
constexpr u32 FenceCompletion = 0x2890;    // -> the completed-fence word
constexpr u32 FenceTag = 0x289C;           // the current fence
constexpr u32 CommandBufferTagMask = 0x28A0;  // Common bits a recorded buffer tracks (not used here)
constexpr u32 TilePredication = 0x28B4;    // (not used here)
constexpr u32 RunPredication = 0x28B8;     // (not used here)
constexpr u32 Flags = 0x28C0;              // bytes 0x28C0-0x28C3, see below
constexpr u32 Flags1 = 0x28C1;
constexpr u32 Flags2 = 0x28C2;
constexpr u32 Flags3 = 0x28C3;'''),
    ('d3d/d3d_native.h', '''constexpr u32 ScissorRect = 0x317C;        // SetScissorRect's RECT (katamari-nx: 0x318C->0x317C)
constexpr u32 IndexBuffer = 0x308C;  // katamari-nx: 0x309C->0x308C
constexpr u32 RenderTargets = 0x3090;      // [4] (katamari-nx: 0x30A0->0x3090)
constexpr u32 DepthStencil = 0x30A0;  // katamari-nx: 0x30B0->0x30A0
constexpr u32 StreamSources = 0x30A4;      // [16] (katamari-nx: 0x30B4->0x30A4)
constexpr u32 StreamStrides = 0x30E8;      // u8[16], stride / 4 (katamari-nx: 0x30F8->0x30E8)
constexpr u32 Textures = 0x30F8;           // [26] (katamari-nx: 0x3108->0x30F8)
constexpr u32 Predication = 0x319C;        // tile predication of the next draws, ~0 none (katamari-nx: 0x31AC->0x319C)''',
     '''constexpr u32 ScissorRect = 0x3220;        // SetScissorRect's RECT
constexpr u32 IndexBuffer = 0x30F4;
constexpr u32 RenderTargets = 0x30F8;      // [4]
constexpr u32 DepthStencil = 0x3108;
constexpr u32 StreamSources = 0x310C;      // [16] of {offset, buffer}
constexpr u32 StreamStrides = 0x3190;      // u8[16], stride / 4
constexpr u32 Textures = 0x31A0;           // [26]
constexpr u32 Predication = d3d::kKameoPredication;  // always ~0: no tiling in this game'''),
    ('d3d/d3d_native.h', '''constexpr u32 CommandBuffer = 0x34B4;      // the D3DCommandBuffer being recorded (katamari-nx: 0x34CC->0x34B4)''',
     '''constexpr u32 CommandBuffer = d3d::kDeviceOffsetCommandBuffer;  // kameo-nx: none recorded, a word that stays 0'''),
    ('d3d/d3d_native.h', '''constexpr u32 AutoDepthStencil = 0x39D0;   // InitializePresentationParameters' surfaces (katamari-nx: 0x39E8->0x39D0)
constexpr u32 AutoFrontBuffer = 0x39D4;  // katamari-nx: 0x39EC->0x39D4
constexpr u32 AutoBackBuffer = 0x39D8;  // katamari-nx: 0x39F0->0x39D8''',
     '''constexpr u32 AutoDepthStencil = 0x3684;   // kameo-nx: before the two D3DDevice_Present sub_820D1ED0 reads
constexpr u32 AutoFrontBuffer = 0x3688;
constexpr u32 AutoBackBuffer = 0x368C;'''),
    # device.cpp: what still runs (KickOff, the fences, Swap) at this device's words.
    ('d3d/hooks/device.cpp', '''REX_IMPORT(__imp__sub_824C1C10, GuestGetCurrentThreadId, u32());''',
     '''// kameo-nx: sub_82554D58 (`lwz r11,256(r13); lwz r3,332(r11); blr`, the calls in D3DDevice_ResourceDestroy and KickOff).
REX_IMPORT(__imp__sub_82554D58, GuestGetCurrentThreadId, u32());'''),
    ('d3d/hooks/device.cpp', '''constexpr u32 kDeviceOffsetFlags = 10940;''', '''constexpr u32 kDeviceOffsetFlags = 0x28C0;  // kameo-nx'''),
    ('d3d/hooks/device.cpp', '''constexpr u32 kDeviceOffsetFlags2 = 10941;
constexpr u8 kDeviceFlagImmediateFences = 0x02;''',
     '''// kameo-nx: 0x28C1 bit 0x04 here (SetFence sub_820CF838: `lbz r10,10433; rlwinm. r10,r10,0,29,29`).
constexpr u32 kDeviceOffsetFlags2 = 0x28C1;
constexpr u8 kDeviceFlagImmediateFences = 0x04;'''),
    ('d3d/hooks/device.cpp', '''constexpr u32 kDeviceOffsetFenceCompletion = 10896;''', '''constexpr u32 kDeviceOffsetFenceCompletion = 0x2890;  // kameo-nx'''),
    ('d3d/hooks/device.cpp', '''constexpr u32 kDeviceOffsetInKickOff = 13216;  // katamari-nx: 13232->13216''',
     '''constexpr u32 kDeviceOffsetInKickOff = d3d::kDeviceOffsetInKickOff;  // kameo-nx: KickOff sub_820CFB10 reads +13520'''),
    ('d3d/hooks/device.cpp', '''constexpr u32 kVdGlobalDeviceImportSlot = 0x82000750;  // katamari-nx: 0x820007F8->0x82000750''',
     '''constexpr u32 kVdGlobalDeviceImportSlot = 0x8200051C;  // kameo-nx: D3DDevice_IsFencePending sub_820CFFB0 (`lis r11,0x8200; lwz r11,1308(r11)`)'''),
    # The ring stub: the sink made at the first rewind (the game's CreateDevice runs, not Renut's), and the segment's words.
    ('d3d/hooks/device.cpp', '''void RewindRing(Device device) {
  if (!device || !g_ring_sink)
    return;
  device.SetW(off::Ring, g_ring_sink);
  device.SetW(off::RingLimit, g_ring_sink + kRingSinkSize);
  device.SetW(off::RingGuarantee, g_ring_sink + kRingSinkSize - kRingSinkGuaranteeMargin);
}''', '''void RewindRing(Device device) {
  if (!device)
    return;
  if (!g_ring_sink)  // kameo-nx: the game's CreateDevice runs, not Renut's
    g_ring_sink = records::AllocPhysicalBlock(kRingSinkSize);
  if (!g_ring_sink)
    return;
  // kameo-nx: as StartNewSegment lays a segment out (RePowered's KickOff hook): the writers pre-increment.
  device.SetW(off::Ring, g_ring_sink - 4);
  device.SetW(off::RingLimit, g_ring_sink + kRingSinkSize - 52);
  device.SetW(off::RingGuarantee, g_ring_sink + kRingSinkSize - 52 - 160);
  device.SetW(d3d::kDeviceOffsetSegmentStart, g_ring_sink);
  device.SetW(d3d::kDeviceOffsetSegmentPredication, 0);
  device.SetW(d3d::kDeviceOffsetSegmentDataBytes, 0);
}'''),
    # Swap: the game's device has no such words at N&B's offsets; the frame's fence and the present stay.
    ('d3d/hooks/device.cpp', '''    word(d3d::kDeviceOffsetSwapCount) = u32(word(d3d::kDeviceOffsetSwapCount)) + 1;
    const u32 fence_tag = word(d3d::kDeviceOffsetFenceTag);
    word(d3d::kDeviceOffsetSwapFenceTag) = fence_tag;
    word(d3d::kDeviceOffsetPresentFenceTag) = fence_tag;
    native::DeviceAt(device_va).Touch(d3d::kDeviceOffsetSwapFenceTag);  // banjo-nx (2026-10-26, banjo_renut_tags)
    device[d3d::kDeviceOffsetSwapFlags] &= u8(~d3d::kSwapFlagPending);
''', ''),
    ('d3d/hooks/device.cpp', '''    // banjo-nx (2026-10-20, banjo_renut_game_gamma): the device's gamma ramp (StoreGamma's +0x3A9C) for the present.
    Video::NoteGameGamma(device + 0x3A84);  // katamari-nx: 0x3A9C->0x3A84
''', ''),
    ('d3d/hooks/device.cpp', '''  if (device) {
    native::RewindRing(native::DeviceAt(device_va));
    native::ResetAllState(native::DeviceAt(device_va));
  }
  renutFrameLimit();''', '''  if (device)
    native::RewindRing(native::DeviceAt(device_va));
  // kameo-nx: the frame's end (the original's Swap runs on the game's Present and D3D's worker thread alike).
  g_banjo_game_frames.fetch_add(1, std::memory_order_relaxed);
  banjo::profiler::FrameBoundary();
  renutFrameLimit();'''),
    ('d3d/hooks/device.cpp', '''void renutFrameLimit();  // kameo_keys.cpp: the player's frame_cap''',
     '''void renutFrameLimit();  // kameo_keys.cpp: the player's frame_cap
#include "profiler/profiler.h"  // kameo-nx: the frame boundary at Swap
extern std::atomic<uint64_t> g_banjo_game_frames;  // kameo_keys.cpp: frames presented'''),
    # The hooks of this file that apply here, at this game's addresses (tools/d3d_disp.py and the disassembly in the notes).
    ('d3d/hooks/device.cpp', '''BANJO_RENUT_UNHOOKED_HOOK(unlinked_824E0590, D3DDevice_InsertCallback_hook);''',
     '''// kameo-nx: the game's functions this file's handlers take, read in this image (RePowered's names through its TU map):
BANJO_RENUT_HOOK(sub_820CFB10, KickOff_hook);  // D3D::CDevice::KickOff (RePowered sub_820CDC88)
BANJO_RENUT_HOOK(sub_820D1860, D3DDevice_Swap_hook);  // D3DDevice_Swap(device, front buffer, parameters) (RePowered sub_820CF9D8)
BANJO_RENUT_HOOK(sub_820CFFB0, IsFencePending_hook);  // D3DDevice_IsFencePending (RePowered sub_820CE128)
// D3D::CDevice::BlockOnFence(CDevice*, uint *fence) (RePowered sub_820CDD80): the fence by pointer in this XDK.
BANJO_RENUT_HOOK_RAW(sub_820CFC08) {
  const auto *fence = GuestPtr<const be_u32>(ctx.r4.u32);
  if (fence)
    BlockOnFence_hook(ctx.r3.u32, u32(*fence), 0, 0);
}
// D3DDevice_BlockUntilIdle(D3DDevice*) (RePowered sub_820CE0A0): the current fence, kicked and waited for.
BANJO_RENUT_HOOK_RAW(sub_820CFF28) {
  const u32 device_va = ctx.r3.u32;
  if (auto *bytes = GuestPtr<u8>(device_va)) {
    const u32 fence = *reinterpret_cast<const be_u32 *>(bytes + d3d::kDeviceOffsetFenceTag);
    BlockOnFence_hook(device_va, fence, 0, 0);
  }
}
// D3D::RingBufferDeviceAllocate(context, device, flags, size*, alignment), through device +13604 (RePowered sub_820CD188):
// flag 1 asks for vertex data, else for a command segment.
BANJO_RENUT_HOOK_RAW(sub_820CF010) {
  auto *size = GuestPtr<be_u32>(ctx.r6.u32);
  if (!size) {
    ctx.r3.u64 = 0;
    return;
  }
  if (!g_ring_sink)
    g_ring_sink = records::AllocPhysicalBlock(kRingSinkSize);
  if (!g_ring_data)
    g_ring_data = records::AllocPhysicalBlock(kRingDataSize);
  if (ctx.r5.u32 & 1) {
    ctx.r3.u64 = RingData(u32(*size), 16);
    return;
  }
  *size = kRingSinkSize;
  ctx.r3.u64 = g_ring_sink;
}
// D3D::CDevice::BlockOnPrimaryRange(CDevice*, pos, size) -> the wrapped end (RePowered sub_820CD578): nothing reads the ring.
BANJO_RENUT_HOOK_RAW(sub_820CF400) {
  const u32 end = ctx.r4.u32 + ctx.r5.u32;
  const auto *mask = GuestPtr<const be_u32>(ctx.r3.u32 + d3d::kDeviceOffsetPrimaryRingMask);
  ctx.r3.u64 = ctx.r3.u32 && mask ? end & u32(*mask) : end;
}
BANJO_RENUT_UNHOOKED_HOOK(unlinked_824E0590, D3DDevice_InsertCallback_hook);'''),
    ('d3d/hooks/resource.cpp', '''REX_IMPORT(__imp__sub_824CAB98, GameXMemFree, void(u32, u32));''',
     '''// kameo-nx: XMemFree is sub_820BA7C8 (the device code's `lis r4,0xB180; li r3,0; bl`).
REX_IMPORT(__imp__sub_820BA7C8, GameXMemFree, void(u32, u32));'''),
]

# Every `kDeviceOffsetStreamSources + X * 4` (N&B's array of pointers) becomes the pair's buffer word.
STREAM_RE = re.compile(r'd3d::kDeviceOffsetStreamSources \+ (.+?) \* 4\)')


def apply_edits(check):
    done = skipped = 0
    bad = []
    for rel, old, new in EDITS:
        p = os.path.join(GPU, rel.replace('/', os.sep))
        t = open(p, encoding='utf-8', errors='surrogateescape', newline='').read()
        crlf = '\r\n' in t
        o, n = (old.replace('\n', '\r\n'), new.replace('\n', '\r\n')) if crlf else (old, new)
        if n in t:
            skipped += 1
        elif o in t:
            if not check:
                t = t.replace(o, n, 1)
                open(p, 'w', encoding='utf-8', errors='surrogateescape', newline='').write(t)
            done += 1
        else:
            bad.append((rel, old[:80]))
    return done, skipped, bad


def device_block(check):
    p = os.path.join(GPU, 'd3d', 'd3d.h')
    t = open(p, encoding='utf-8', errors='surrogateescape', newline='').read()
    crlf = '\r\n' in t
    if 'kKameoDeclarationId' in t:
        return 0
    i = t.find(DEVICE_OLD_START)
    if i < 0:
        raise SystemExit('d3d.h: the device block was not found')
    new = DEVICE_NEW.replace('\n', '\r\n') if crlf else DEVICE_NEW
    if not check:
        open(p, 'w', encoding='utf-8', errors='surrogateescape', newline='').write(t[:i] + new)
    return 1


def streams(check):
    n = 0
    for d, _, fs in os.walk(GPU):
        for f in fs:
            if not f.endswith(('.cpp', '.h', '.inc')) or f == 'd3d.h':
                continue
            p = os.path.join(d, f)
            t = open(p, encoding='utf-8', errors='surrogateescape', newline='').read()
            t2, k = STREAM_RE.subn(lambda m: f'd3d::StreamSourceOffset({m.group(1)}))', t)
            if k:
                n += k
                if not check:
                    open(p, 'w', encoding='utf-8', errors='surrogateescape', newline='').write(t2)
    return n


# This game's own, kept when the script runs again.
KAMEO_HOOKS = {'820CFB10', '820D1860', '820CFFB0', '820CFC08', '820CFF28', '820CF010', '820CF400'}
LINE = re.compile(r'^(\s*)BANJO_RENUT_(UNHOOKED_)?HOOK(_RAW)?(_SHARED)?\(\s*(?:sub_|unlinked_)([0-9A-F]{8})(.*)$')


def unhook(check):
    """Every hook line of the five files: unhooked, named by its old address; no address stays a symbol."""
    n = 0
    for name in ('device.cpp', 'draw.cpp', 'resource.cpp', 'shader.cpp', 'state.cpp'):
        path = os.path.join(HOOKS, name)
        text = open(path, encoding='utf-8', errors='surrogateescape', newline='').read()
        nl = '\r\n' if '\r\n' in text else '\n'
        lines = text.split(nl)
        changed = False
        for k, line in enumerate(lines):
            m = LINE.match(line)
            if not m:
                continue
            indent, was_off, raw, shared, addr, rest = m.groups()
            if addr in KAMEO_HOOKS:
                continue
            new = f'{indent}BANJO_RENUT_UNHOOKED_HOOK{raw or ""}{shared or ""}(unlinked_{addr}{rest}'
            if new != line:
                lines[k] = new
                changed = True
                n += 1
        if changed and not check:
            open(path, 'w', encoding='utf-8', errors='surrogateescape', newline='').write(nl.join(lines))
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args()
    print('hook lines unhooked:', unhook(a.check))
    print('device block:', device_block(a.check))
    done, skipped, bad = apply_edits(a.check)
    print(f'{done} edits applied, {skipped} already there, {len(bad)} missing')
    for b in bad:
        print('MISSING', b)
    print('stream source sites:', streams(a.check))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
