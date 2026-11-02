/**
 * @file    gpu/d3d/d3d.h
 * @brief   Guest D3D (Xbox 360 XDK) layouts as used by Banjo-Kazooie: Nuts & Bolts.
 *
 * Offsets come from N&B's own default.xex IDB (typed D3DDevice / D3DConstants /
 * GPU_*PACKET and resource structs); every one is static_asserted. Only fields
 * the renderer reads or maintains are named.
 */
#pragma once

#include <cstddef>

#include <rex/types.h>

namespace renut::gpu::d3d {

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

// D3DRESOURCETYPE as N&B's XDK defines it (differs from other XDK versions).
// Common's low 4 bits hold the base type; the 16+ values are what
// D3DResource_GetType reports for texture variants.
enum class ResourceType : u32 {
  VertexBuffer = 1,
  IndexBuffer = 2,
  Texture = 3,
  Surface = 4,
  VertexDeclaration = 5,
  VertexShader = 6,
  PixelShader = 7,
  ConstantBuffer = 8,
  CommandBuffer = 9,
  AsyncCommandBufferCall = 10,
  PerfCounterBatch = 11,
  OcclusionQueryBatch = 12,
  Volume = 16,
  VolumeTexture = 17,
  CubeTexture = 18,
  ArrayTexture = 19,
  LineTexture = 20,
};
constexpr u32 kCommonTypeMask = 0xF;

// kameo-nx: XDK 2.0.2135's resource head is 12 bytes: Common (the reference count in its low byte, the type in bits 16-18,
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

struct TextureFetchConstant {
  be_u32 dword[6];
};
static_assert(sizeof(TextureFetchConstant) == 24);

// A texture header holds the CPU addresses of its base and mip data; the
// GPU address is what D3DDevice_SetTexture (0x821FE558) writes into the
// device's fetch constant: the low 29 bits, plus 4 KB for the 0xE0000000
// (4 KB page) physical view, which starts at physical 0x1000.
constexpr u32 CpuToGpuAddress(u32 cpu_address) {
  return (cpu_address & 0x1FFFFFFFu) + (((cpu_address >> 20) + 0x200u) & 0x1000u);
}

struct VertexFetchConstant {
  be_u32 dword[2];
};
static_assert(sizeof(VertexFetchConstant) == 8);

// D3DBaseTexture / D3DTexture / D3DCubeTexture / D3DVolumeTexture / D3DLineTexture.
struct BaseTexture {
  Resource resource;            // +0x00
  be_u32 MipFlush;              // +0x0C
  TextureFetchConstant Format;  // +0x10
};
static_assert(sizeof(BaseTexture) == 40);
static_assert(offsetof(BaseTexture, Format) == 0x10);

struct Surface {
  be_u32 word[16];  // kameo-nx: 64 bytes in XDK 2.0.2135 (ReadSurfaceDesc)
};
static_assert(sizeof(Surface) == 64);

struct VertexBuffer {
  Resource resource;           // +0x00
  VertexFetchConstant Format;  // +0x0C
};
static_assert(sizeof(VertexBuffer) == 20);

struct IndexBuffer {
  Resource resource;  // +0x00
  be_u32 Address;     // +0x0C
  be_u32 Size;        // +0x10
};
static_assert(sizeof(IndexBuffer) == 20);

// D3DVertexShader / D3DPixelShader / D3DVertexDeclaration headers.
struct Shader {
  RecordResource resource;
};
static_assert(sizeof(Shader) == 24);

// The microcode container handed to CreateVertexShader / CreatePixelShader
// (same layout XenosRecomp's shader.h reads).
struct ShaderContainer {
  be_u32 flags;
  be_u32 virtualSize;
  be_u32 physicalSize;
  be_u32 fieldC;
  be_u32 constantTableOffset;
  be_u32 definitionTableOffset;
  be_u32 shaderOffset;
  be_u32 field1C;
  be_u32 field20;
};
static_assert(sizeof(ShaderContainer) == 0x24);

// ---------------------------------------------------------------------------
// Device
// ---------------------------------------------------------------------------

constexpr u32 kTextureFetchSlots = 26;
constexpr u32 kVertexFetchConstants = 18;
constexpr u32 kFirstVertexFetchIndex = 78;  // Xenos vfetch index of VertexFetch[0]

// _D3DConstants, the register image D3D keeps for the GPU.
struct Constants {
  TextureFetchConstant TextureFetch[kTextureFetchSlots];  // +0x000
  VertexFetchConstant VertexFetch[kVertexFetchConstants];  // +0x270
  be_f32 VertexShaderF[256][4];                           // +0x300
  be_f32 PixelShaderF[256][4];                            // +0x1300
  be_u32 VertexShaderB[4];                                // +0x2300
  be_u32 PixelShaderB[4];                                 // +0x2310
  be_u32 VertexShaderI[16];                               // +0x2320
  be_u32 PixelShaderI[16];                                // +0x2360
};
static_assert(sizeof(Constants) == 9120);
static_assert(offsetof(Constants, VertexFetch) == 0x270);
static_assert(offsetof(Constants, VertexShaderF) == 0x300);
static_assert(offsetof(Constants, PixelShaderF) == 0x1300);
static_assert(offsetof(Constants, VertexShaderB) == 0x2300);
static_assert(offsetof(Constants, VertexShaderI) == 0x2320);

struct DestinationPacket {
  be_u32 SurfaceInfo;      // +0x00
  be_u32 Color0Info;       // +0x04
  be_u32 DepthInfo;        // +0x08
  be_u32 Color1Info;       // +0x0C
  be_u32 Color2Info;       // +0x10
  be_u32 Color3Info;       // +0x14
  be_u32 CoherDestBase[8]; // +0x18
  be_u32 ScreenScissorTL;  // +0x38
  be_u32 ScreenScissorBR;  // +0x3C
};
static_assert(sizeof(DestinationPacket) == 64);

struct WindowPacket {
  be_u32 WindowOffset;
  be_u32 WindowScissorTL;
  be_u32 WindowScissorBR;
};
static_assert(sizeof(WindowPacket) == 12);

struct ValuesPacket {
  be_u32 MaxVtxIndx;            // +0x00
  be_u32 MinVtxIndx;            // +0x04
  be_u32 IndxOffset;            // +0x08
  be_u32 MultiPrimIbResetIndx;  // +0x0C
  be_u32 ColorMask;             // +0x10
  be_f32 BlendRed;              // +0x14
  be_f32 BlendGreen;            // +0x18
  be_f32 BlendBlue;             // +0x1C
  be_f32 BlendAlpha;            // +0x20
  be_u32 Unused[3];             // +0x24
  be_u32 StencilRefMaskBF;      // +0x30
  be_u32 StencilRefMask;        // +0x34
  be_f32 AlphaRef;              // +0x38
  be_f32 VportXScale;           // +0x3C
  be_f32 VportXOffset;          // +0x40
  be_f32 VportYScale;           // +0x44
  be_f32 VportYOffset;          // +0x48
  be_f32 VportZScale;           // +0x4C
  be_f32 VportZOffset;          // +0x50
};
static_assert(sizeof(ValuesPacket) == 84);

struct ProgramPacket {
  be_u32 ProgramControl;
  be_u32 ContextMisc;
  be_u32 InterpolatorControl;
  be_u32 Wrapping0;
  be_u32 Wrapping1;
};
static_assert(sizeof(ProgramPacket) == 20);

struct ControlPacket {
  be_u32 DepthControl;      // +0x00
  be_u32 BlendControl0;     // +0x04
  be_u32 ColorControl;      // +0x08
  be_u32 HiControl;         // +0x0C
  be_u32 ClipControl;       // +0x10
  be_u32 ModeControl;       // +0x14
  be_u32 VteControl;        // +0x18
  be_u32 Unused;            // +0x1C
  be_u32 EdramModeControl;  // +0x20
  be_u32 BlendControl1;     // +0x24
  be_u32 BlendControl2;     // +0x28
  be_u32 BlendControl3;     // +0x2C
};
static_assert(sizeof(ControlPacket) == 48);

// kameo-nx (2026-11-02): the device is the game's own, XDK 2.0.2135's layout (with the title update's 16 bytes from 0x34D8):
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
