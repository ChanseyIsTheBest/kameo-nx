// banjo-nx - hand-written native versions of hot guest functions (CPU enhancements banjo_hand_*).
//
// The localized copies of tools/localize_native.py keep a function's own statements; these replace the statements:
// the XDK C runtime's memcpy and memset and Direct3D's two shader float-constant writers, written from what the
// routines do. They are sa2-nx's natives (app/src/cpu/hot/n_827F05C0.h, n_827EFEA0.h, n_d3d_constants.h, GPL-3.0;
// the memcpy came there from masseffect-nx's n_82AC4AF0.h): tools/crt_signatures.py of sa2-nx finds the same three C
// runtime routines in Banjo's image (unique matches), and the two constant writers are the same instructions with
// the same two offsets. Banjo's own additions are the two writers of Direct3D's state flush (namespace flush below).
// What is different here is how they are used and checked:
//
//   * Each is a template on the store policy of banjo_hot_common.h, so the guard's logged form records what it is
//     about to write like any generated copy (no hand-written list of written ranges).
//   * tools/localize_native.py puts them IN FRONT of the generated copy of the same function wherever that copy is
//     used: as a hook of its own, and inside every other function's copy that contains it (app/src/cpu/hot/hand.txt).
//     Inside a caller's copy the call is inlined: no hook, no hand-over of every register (on the Switch that
//     hand-over is what made copies with real calls slower than their originals).
//   * A native returns false to DECLINE a call it does not cover (nothing written yet): the generated copy of the
//     original then runs, in the same place.
//   * The registers the original leaves behind as scratch (hand.txt gives them per function; tools/localize_native.py
//     takes them from the original's own statements) are NOT reproduced: the natives write no register. Those
//     registers are left out of the guard's register compare for every hook whose copy contains the native. Sound
//     because they are caller-saved scratch of assembly routines and of a callee the callers reach through the
//     normal calling convention: a caller that read one after the call would get a different value, would act on
//     it, and the guard would see that in everything else it compares.
//
// ARM (the A57 is where this has to pay, and sa2-nx's first native Direct3D layer lost there for reasons no PC run
// showed: docs/api-renderer.md of sa2-nx, section 6): nothing here is per thread (no thread_local: a function call on
// the Switch), nothing copies a context, no loop keeps a count where a store could alias it, nothing is byte-swapped
// word by word (the copies are guest order to guest order), and sizes up to 128 bytes (memcpy; 64 for memset) are
// moved with overlapping 16/8/4/2-byte loads and stores without a call into the C library. In a PC run of title,
// menus, intro and gameplay 62 % of the 21.9 million memcpy calls were under 64 bytes and 21 % were 64 to 127; 46 %
// of the memsets were 4 to 7 bytes; 98 % of the 26 million constant uploads were 1 to 7 vectors.
//
// Guest function addresses may be named in this folder only (tools/direct_calls.py and tools/localize_native.py
// read every other source's addresses as hooked functions; these four are hooked, by app/src/cpu/hot/localized.txt).
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

#include <rex/ppc/context.h>

#if defined(__aarch64__)
#include <arm_neon.h>
#endif

#include "cpu/banjo_hot_common.h"
#include <rex/ppc/diet_ops.h>  // katamari-nx: rex::ppc::SingleToDoublePure

// katamari-nx: the in-run A/B's target in its off half (renut_engine/gpu/banjo_renut_tuning.cpp).
#include <atomic>
namespace banjo::renut_gpu {
extern std::atomic<int> g_ab_off;
}  // namespace banjo::renut_gpu

namespace banjo::hand {

// The natives' own keys (banjo_hot_guest.cpp), each read once: 0 not read yet, 1 off, 2 on.
enum : uint32_t {
  kMemcpy,
  kMemset,
  kConstants,
  kNatives
};
extern uint8_t g_state[kNatives];
uint8_t Init(uint32_t which);
[[gnu::always_inline]] inline bool On(uint32_t which) {
  const uint8_t state = g_state[which];
  if (state == 2) [[likely]] return true;
  return state == 0 && Init(which) == 2;
}

namespace detail {

// 16 bytes through a register (any alignment; one NEON load or store on the A57).
struct B16 {
  uint8_t b[16];
};
[[gnu::always_inline]] inline B16 L16(const uint8_t* p) {
  B16 v;
  std::memcpy(v.b, p, 16);
  return v;
}
[[gnu::always_inline]] inline void S16(uint8_t* p, const B16& v) { std::memcpy(p, v.b, 16); }

// 1 <= n <= 64; all loads before all stores (memmove semantics for dst <= src overlap).
[[gnu::always_inline]] inline void CopySmall(uint8_t* d, const uint8_t* s, uint32_t n) {
  if (n >= 16) {
    if (n > 32) {
      const B16 a = L16(s), b = L16(s + 16), c = L16(s + n - 32), e = L16(s + n - 16);
      S16(d, a);
      S16(d + 16, b);
      S16(d + n - 32, c);
      S16(d + n - 16, e);
    } else {
      const B16 a = L16(s), e = L16(s + n - 16);
      S16(d, a);
      S16(d + n - 16, e);
    }
  } else if (n >= 8) {
    uint64_t a, e;
    std::memcpy(&a, s, 8);
    std::memcpy(&e, s + n - 8, 8);
    std::memcpy(d, &a, 8);
    std::memcpy(d + n - 8, &e, 8);
  } else if (n >= 4) {
    uint32_t a, e;
    std::memcpy(&a, s, 4);
    std::memcpy(&e, s + n - 4, 4);
    std::memcpy(d, &a, 4);
    std::memcpy(d + n - 4, &e, 4);
  } else if (n >= 2) {
    uint16_t a, e;
    std::memcpy(&a, s, 2);
    std::memcpy(&e, s + n - 2, 2);
    std::memcpy(d, &a, 2);
    std::memcpy(d + n - 2, &e, 2);
  } else {
    *d = *s;
  }
}

// 64 < n <= 128; all loads before all stores. (Banjo's calls: a fifth are 64 to 127 bytes.)
[[gnu::always_inline]] inline void CopyMedium(uint8_t* d, const uint8_t* s, uint32_t n) {
  const B16 a = L16(s), b = L16(s + 16), c = L16(s + 32), e = L16(s + 48);
  const B16 f = L16(s + n - 64), g = L16(s + n - 48), h = L16(s + n - 32), i = L16(s + n - 16);
  S16(d, a);
  S16(d + 16, b);
  S16(d + 32, c);
  S16(d + 48, e);
  S16(d + n - 64, f);
  S16(d + n - 48, g);
  S16(d + n - 32, h);
  S16(d + n - 16, i);
}

// 1 <= n <= 64, every byte = v.
[[gnu::always_inline]] inline void FillSmall(uint8_t* d, uint8_t v, uint32_t n) {
  if (n >= 16) {
    B16 x;
    std::memset(x.b, v, 16);
    S16(d, x);
    S16(d + n - 16, x);
    if (n > 32) {
      S16(d + 16, x);
      S16(d + n - 32, x);
    }
  } else {
    const uint64_t x = uint64_t(v) * 0x0101010101010101ull;
    if (n >= 8) {
      std::memcpy(d, &x, 8);
      std::memcpy(d + n - 8, &x, 8);
    } else if (n >= 4) {
      std::memcpy(d, &x, 4);
      std::memcpy(d + n - 4, &x, 4);
    } else if (n >= 2) {
      std::memcpy(d, &x, 2);
      std::memcpy(d + n - 2, &x, 2);
    } else {
      *d = v;
    }
  }
}

// [a, a + n) is one stretch of host memory: it does not wrap around 4 GB and (on hosts with the physical-view
// offset) does not cross its boundary. n >= 1.
[[gnu::always_inline]] inline bool Straight(uint32_t a, uint32_t n) {
  if (uint64_t(a) + n > 0x100000000ull) return false;
  return ::banjo::hot::PhysOff(a) == ::banjo::hot::PhysOff(a + n - 1u);
}

}  // namespace detail

// kameo-nx (2026-11-03, the first console profile; the evidence is in hand.txt). All three from banjo-nx's natives, each
// checked here against this game's own statements (XDK 2.0.2135 compiled them differently from N&B's).

// sub_8255D400 - XDK C runtime memcpy: r3 = dst, r4 = src, r5 = n (its 32-bit part); returns r3 as it came.
//
// The original stores r3 at -8(r1) first (a real store, reloaded at every return), then copies FORWARD: a byte head up to
// 8-byte dst alignment, then by the alignment of src: 128-byte blocks and doublewords (8-aligned), the same in words
// (4-aligned), or bytes (any other). dcbt / dcbtst are hints; the two `b .` after its block loops cannot be reached (the
// source pointer passes its limit before the block count ends). Facts used:
//   * every size and address is the 32-bit part (cmplw, rlwinm, ea = u32 + u32);
//   * no loop reads past src + n or writes past dst + n;
//   * units run upward and each unit's loads come before its stores: for dst <= src the result is memmove's, for ranges
//     that do not overlap a plain copy. For dst > src with dst - src < n the forward copy smears: DECLINED;
//   * n == 0 copies nothing (the save of r3 still happens).
// Also declined: ranges that wrap around 4 GB, and a destination that covers the save slot.
// Scratch the original leaves and this does not reproduce (hand.txt): r0, r4-r12.
namespace n_8255D400 {

template <class S>
[[gnu::always_inline]] inline bool Native(PPCContext& __restrict ctx, uint8_t* base, [[maybe_unused]] S& s) {
  if (!On(kMemcpy)) [[unlikely]] return false;
  const uint32_t dst = ctx.r3.u32, src = ctx.r4.u32, n = ctx.r5.u32, slot = ctx.r1.u32 - 8u;
  s.Hand(kMemcpy, n);
  if (n) {
    if (uint32_t(dst - src - 1u) < n - 1u) [[unlikely]] return false;  // dst > src, dst - src < n
    if (!detail::Straight(dst, n) || !detail::Straight(src, n)) [[unlikely]] return false;
    if (uint64_t(uint32_t(slot + 7u - dst)) < uint64_t(n) + 7u) [[unlikely]] return false;  // dst covers the slot
  }
  if (!s.Note(slot, 8)) [[unlikely]] return true;
  const uint64_t saved = __builtin_bswap64(ctx.r3.u64);
  std::memcpy(::banjo::hot::Raw(base, slot), &saved, 8);
  if (n == 0) return true;
  if (!s.Note(dst, n)) [[unlikely]] return true;
  ::banjo::hot::Card(base, dst, n);
  uint8_t* d = ::banjo::hot::Raw(base, dst);
  const uint8_t* p = ::banjo::hot::Raw(base, src);
  if (n <= 64) {
    detail::CopySmall(d, p, n);
  } else if (n <= 128) {
    detail::CopyMedium(d, p, n);
  } else if (uint32_t(src - dst) < n) {
    std::memmove(d, p, n);  // dst < src (or equal), overlapping: the forward copy is memmove
  } else {
    std::memcpy(d, p, n);
  }
  return true;
}

}  // namespace n_8255D400

// sub_82554238 - XDK C runtime memset: r3 = dst, r4 = value (its low byte), r5 = n; r3 is not written.
//
// Under 16 bytes: single stb. From 16: the byte splatted into a vector (two table lookups, a shift and an or, then vspltb);
// stvlx up to 16-byte alignment, 16-byte stores up to 128-byte alignment, then blocks of 128 (dcbz128 alone for a zero
// value; for any other, dcbz128 of the block four ahead and eight stvx), 16-byte stores, and stvrx for the rest, which
// writes nothing at an aligned address. Each byte of [dst, dst + n) ends as the value and nothing outside changes; nothing
// reads the destination. Facts used:
//   * every size and address is the 32-bit part; ctr = r5 is 64-bit on the byte path: anything in r5's upper half is DECLINED;
//   * the 16-byte step is a word of the image's data (0x82720314). It is 16 there (read from out/renut_image.bin); a game
//     that changed it would change the loops, so it is read here as there, and anything else is DECLINED;
//   * ranges that wrap around 4 GB are declined.
// Scratch not reproduced (hand.txt): r4-r10, v0, v12, v13.
namespace n_82554238 {

template <class S>
[[gnu::always_inline]] inline bool Native(PPCContext& __restrict ctx, uint8_t* base, [[maybe_unused]] S& s) {
  if (!On(kMemset)) [[unlikely]] return false;
  if (ctx.r5.u64 >> 32) [[unlikely]] return false;
  const uint32_t dst = ctx.r3.u32, n = ctx.r5.u32;
  s.Hand(kMemset, n);
  if (n == 0) return true;
  if (!detail::Straight(dst, n)) [[unlikely]] return false;
  if (n >= 16) {
    uint32_t step;
    std::memcpy(&step, ::banjo::hot::Raw(base, 0x82720314u), 4);
    if (__builtin_bswap32(step) != 16u) [[unlikely]] return false;
  }
  if (!s.Note(dst, n)) [[unlikely]] return true;
  ::banjo::hot::Card(base, dst, n);
  uint8_t* d = ::banjo::hot::Raw(base, dst);
  if (n <= 64) {
    detail::FillSmall(d, ctx.r4.u8, n);
  } else {
    std::memset(d, int(ctx.r4.u8), n);
  }
  return true;
}

}  // namespace n_82554238

// Direct3D's two shader float-constant writers (every constant upload of every draw):
//   sub_820C9C38: r3 = device, r4 = first constant, r5 = source (any alignment), r6 = count of 16-byte vectors.
//                 Destination ((r3 + (r4 + 120) * 16) & ~15) + 16 k; dirty mask at r3 + 16.
//   sub_820C9D80: the same with (r4 + 376) and the dirty mask at r3 + 24.
// XDK 2.0.2135's compile differs from N&B's: the mask comes from r4 and r6, not from r7, and r4, r6 and r3 go to the stack first
// (28(r1), 44(r1), -32(r1), real stores; r3 is read back for the mask). The originals copy four vectors per iteration, then
// one per iteration: lvlx + lvrx + vor is an unaligned 16-byte load (lvrx at an aligned address loads nothing), stvx128 an
// aligned store, dcbt are hints. Facts used:
//   * every address is the 32-bit part. r9 = r6 is decremented as 64 bits: a count with anything in its upper half is
//     DECLINED, and so is one the signed loop counters could not hold;
//   * the four-at-a-time loop loads its four vectors before it stores them: ranges that overlap are DECLINED, as are
//     ranges that wrap around 4 GB, and a destination over the three stack words (the original reads them back);
//   * the source is read after the stack stores here as there, and the mask after the copy.
// The mask's bits, as the original's statements: the blocks of four registers from r4 >> 2 to (r4 + r6 - 1) >> 2.
// Scratch not reproduced (hand.txt): r4-r10, v0, v7-v13.
namespace constants {

template <class S>
[[gnu::always_inline]] inline bool Upload(PPCContext& __restrict ctx, uint8_t* base, [[maybe_unused]] S& s,
                                          uint32_t first, uint32_t mask_offset) {
  if (!On(kConstants)) [[unlikely]] return false;
  if (ctx.r6.u64 >> 32) [[unlikely]] return false;
  const uint32_t count = ctx.r6.u32, src = ctx.r5.u32, device = ctx.r3.u32, sp = ctx.r1.u32;
  const uint32_t mask = device + mask_offset;
  const uint32_t dst = (((ctx.r4.u32 + first) << 4) + device) & ~15u;
  s.Hand(kConstants, count);
  if (mask > 0xFFFFFFF7u) [[unlikely]] return false;
  if (count > 0x0FFFFFFFu) [[unlikely]] return false;
  const uint32_t bytes = count << 4;
  if (count) {
    if (!detail::Straight(dst, bytes) || !detail::Straight(src, bytes)) [[unlikely]] return false;
    if (uint32_t(src - dst) < bytes || uint32_t(dst - src) < bytes) [[unlikely]] return false;
    // [sp - 32, sp + 48) holds the three stack words.
    if (uint32_t(sp - 32u - dst) < bytes || uint32_t(dst - (sp - 32u)) < 80u) [[unlikely]] return false;
  }
  const uint32_t r4_be = __builtin_bswap32(ctx.r4.u32), r6_be = __builtin_bswap32(count), r3_be = __builtin_bswap32(device);
  if (!s.Note(sp + 28u, 4)) [[unlikely]] return true;
  std::memcpy(::banjo::hot::Raw(base, sp + 28u), &r4_be, 4);
  if (!s.Note(sp + 44u, 4)) [[unlikely]] return true;
  std::memcpy(::banjo::hot::Raw(base, sp + 44u), &r6_be, 4);
  if (!s.Note(sp - 32u, 4)) [[unlikely]] return true;
  std::memcpy(::banjo::hot::Raw(base, sp - 32u), &r3_be, 4);
  if (count) {
    if (!s.Note(dst, bytes)) [[unlikely]] return true;
    ::banjo::hot::Card(base, dst, bytes);
    uint8_t* d = ::banjo::hot::Raw(base, dst);
    const uint8_t* p = ::banjo::hot::Raw(base, src);
    if (count <= 8) {
      const uint8_t* const end = p + bytes;  // (a pointer cursor: no store here can alias the loop's bound)
      do {
        detail::S16(d, detail::L16(p));
        d += 16;
        p += 16;
      } while (p != end);
    } else {
      std::memcpy(d, p, bytes);
    }
  }
  const uint64_t low = ctx.r4.u32 >> 2;
  const uint32_t last = uint32_t(ctx.r4.u64 + ctx.r6.u64 - 1u);
  const uint64_t blocks = ((uint64_t(last >> 2) - low) & 0xFFFFFFFFull) & 0x7F;
  const uint64_t run = uint64_t(int64_t(INT64_MIN) >> (blocks > 0x3F ? 0x3F : blocks));
  const uint64_t bits = (low & 0x40) ? 0 : (run >> (low & 0x7F));
  if (!s.Note(mask, 8)) [[unlikely]] return true;
  ::banjo::hot::Card(base, mask, 8);
  uint64_t m;
  std::memcpy(&m, ::banjo::hot::Raw(base, mask), 8);
  m |= __builtin_bswap64(bits);
  std::memcpy(::banjo::hot::Raw(base, mask), &m, 8);
  return true;
}

}  // namespace constants

namespace n_820C9C38 {
template <class S>
[[gnu::always_inline]] inline bool Native(PPCContext& __restrict ctx, uint8_t* base, S& s) {
  return constants::Upload(ctx, base, s, 120, 16);
}
}  // namespace n_820C9C38

namespace n_820C9D80 {
template <class S>
[[gnu::always_inline]] inline bool Native(PPCContext& __restrict ctx, uint8_t* base, S& s) {
  return constants::Upload(ctx, base, s, 376, 24);
}
}  // namespace n_820C9D80

}  // namespace banjo::hand
