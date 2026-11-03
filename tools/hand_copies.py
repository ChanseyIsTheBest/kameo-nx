#!/usr/bin/env python3
"""kameo-nx (2026-11-03, the user's request: hand-optimise the top of the drawing thread from the first console profile).
Hand edits of a generated copy's statements, for a function too large to rewrite whole, where the samples show one cost in
one place. tools/localize_native.py applies them to f_<name> only, after making it:

  - f_<name><Fast> and f_<name><Logged> have the edit; o_<name>, the original statements, does not. So the guard
    (app/src/cpu/banjo_hot_guest.cpp) compares the edited code with the original, call by call, as it does a machine copy;
  - each edit names the exact statements it expects. If a new code generation changes them the tool stops: an edit is never
    applied to code it was not written against.

The evidence goes next to each edit, as for app/src/cpu/hot/hand.txt.
"""
import sys


def replace_one(name, body, old, new):
    """body: [(text, block)]. The one line equal to 'old' becomes the lines 'new'."""
    at = [k for k, (text, _) in enumerate(body) if text == old]
    if len(at) != 1:
        sys.exit(f'hand_copies: {name}: {len(at)} lines are "{old.strip()}", expected 1: the code generation changed')
    block = body[at[0]][1]
    return body[:at[0]] + [(line, block) for line in new] + body[at[0] + 1:]


def expect_before(name, body, line, previous):
    """The statements in front of 'line', so an edit only lands where it was written."""
    at = next(k for k, (text, _) in enumerate(body) if text == line)
    got = [text for text, _ in body[at - len(previous):at]]
    if got != previous:
        sys.exit(f'hand_copies: {name}: the statements before "{line.strip()}" changed:\n' + '\n'.join(got))


# sub_82379E10, 7.9 % of the drawing thread (console, 2026-11-03, Thorn's Castle). It walks an array of 64-byte elements, and most
# fail the first test (the owner at -8(r28) against 716(r1)). Of its 566 samples, 146 are that load, a cache miss on every
# element, and 111 are the loop reloading the two stack floats at 144(r1) and 152(r1) and widening each, with its denormal
# check, for an element it then rejects.
#
# The edit: the rejected elements in a loop of their own, with no floats, and a prefetch eight elements ahead (the game's
# dcbt reaches only about the next one, too late for a loop this short). An element that passes the first test goes back
# to the original statements at loc_8237A2E0, which test it again. When the array ends, what the original's last rejected
# element leaves is written: r27, r11, r10, f31, f26. 716(r1) is this thread's own stack and nothing in the loop stores.
# The count at 0(r18) is read every time, as there.
#
# The second console run (2026-11-03 evening): that loop is 77 of this function's 209 samples, and about 12 of them wrote cr6
# to the stack for every element. So no cr6 inside: it was "lt" on the way in, and the loop goes on only while the same test
# is "lt", so it stays what the original's last compare left; the one exit where it changes writes it. The loop's values in
# locals of their own, so a function this large keeps them in registers.
LOOP_82379E10 = [
    '\tif (cr6.lt) {',
    '\t\tconst uint32_t hand_owner = REX_LOAD_D_U32(l_r1.u32, 716);',
    '\t\tconst uint32_t hand_count_at = r18.u32;',
    '\t\tint64_t hand_index = r22.s64, hand_at = r28.s64;',
    '\t\tuint64_t hand_count = l_r3.u64;',
    '\t\tfor (;;) {',
    '\t\t\tREX_LOOP_BARRIER();',
    '\t\t\tREX_PREFETCH((uint32_t)hand_at + 512, 0);',
    '\t\t\tconst uint32_t hand_element = REX_LOAD_D_U32((uint32_t)hand_at, -8);',
    '\t\t\tif (hand_element == hand_owner) {',
    '\t\t\t\tl_r3.u64 = hand_count;',
    '\t\t\t\tr22.s64 = hand_index;',
    '\t\t\t\tr28.s64 = hand_at;',
    '\t\t\t\tgoto loc_8237A2E0;',
    '\t\t\t}',
    '\t\t\thand_count = REX_LOAD_D_U32(hand_count_at, 0);',
    '\t\t\thand_index += 1;',
    '\t\t\thand_at += 64;',
    '\t\t\tif ((int32_t)hand_index >= (int32_t)hand_count) {',
    '\t\t\t\tl_r3.u64 = hand_count;',
    '\t\t\t\tr22.s64 = hand_index;',
    '\t\t\t\tr28.s64 = hand_at;',
    '\t\t\t\tcr6.compare<int32_t>(r22.s32, l_r3.s32, xer);',
    '\t\t\t\tr27.s64 = r28.s64 + -88;',
    '\t\t\t\tr11.u64 = hand_element;',
    '\t\t\t\tl_r10.u64 = hand_owner;',
    '\t\t\t\ttemp.u32 = REX_LOAD_D_U32(l_r1.u32, 144);',
    '\t\t\t\tf31.f64 = rex::ppc::SingleToDoubleAnyMode(temp.u32);',
    '\t\t\t\ttemp.u32 = REX_LOAD_D_U32(l_r1.u32, 152);',
    '\t\t\t\tf26.f64 = rex::ppc::SingleToDoubleAnyMode(temp.u32);',
    '\t\t\t\tbreak;',
    '\t\t\t}',
    '\t\t}',
    '\t}',
]


def patch_82379E10(name, body):
    line = '\tif (cr6.lt) goto loc_8237A2E0;'
    expect_before(name, body, line, ['\tl_r3.u64 = REX_LOAD_D_U32(r18.u32, 0);', '\tr22.s64 = r22.s64 + 1;',
                                     '\tr28.s64 = r28.s64 + 64;', '\tcr6.compare<int32_t>(r22.s32, l_r3.s32, xer);'])
    return replace_one(name, body, line, LOOP_82379E10)


# sub_82524E98, 3.4 % of the drawing thread (second console run). The same shape: a walk over 64-byte elements, most of them
# rejected by the word at 8(r31). That loop is 43 of its 105 samples: 23 on that load, a cache miss with no prefetch at all,
# and the rest on four stack spills and cr6 written to the stack.
#
# The edit: the rejected elements in a loop of their own with a prefetch eight elements ahead, its values in locals. A rejected
# element in the original: r11 = r24, r9 = r11 & 0xFF, cr6 from r9 against 0, and on to loc_8252582C because r24 is 0 (its
# only assignment in the function is "r24.s64 = 0": checked below); there r9 = the count at 32(r25), r26 + 1, r31 + 64, and
# back to the top while r26 < r9. What the two ways out leave is what the original's last statements leave: r7, r9, r11, r26,
# r31 and cr6. The count is read every time, as there, and nothing in the loop stores.
LOOP_82524E98 = [
    '\tif (!cr6.eq) {',
    '\t\tconst uint32_t hand_want = r21.u32, hand_count_at = r25.u32;',
    '\t\tint64_t hand_index = r26.s64, hand_at = r31.s64;',
    '\t\tfor (;;) {',
    '\t\t\tconst uint32_t hand_count = REX_LOAD_D_U32(hand_count_at, 32);',
    '\t\t\thand_index += 1;',
    '\t\t\thand_at += 64;',
    '\t\t\tif ((int32_t)hand_index >= (int32_t)hand_count) {',
    '\t\t\t\tl_r9.u64 = hand_count;',
    '\t\t\t\tr26.s64 = hand_index;',
    '\t\t\t\tr31.s64 = hand_at;',
    '\t\t\t\tr11.u64 = r24.u64;',
    '\t\t\t\tcr6.compare<int32_t>(r26.s32, l_r9.s32, xer);',
    '\t\t\t\tgoto loc_82525840;',
    '\t\t\t}',
    '\t\t\tREX_LOOP_BARRIER();',
    '\t\t\tREX_PREFETCH((uint32_t)hand_at + 8 + 512, 0);',
    '\t\t\tconst uint32_t hand_element = REX_LOAD_D_U32((uint32_t)hand_at, 8);',
    '\t\t\tif (hand_element == hand_want) {',
    '\t\t\t\tl_r7.u64 = hand_element;',
    '\t\t\t\tl_r9.u64 = hand_count;',
    '\t\t\t\tr26.s64 = hand_index;',
    '\t\t\t\tr31.s64 = hand_at;',
    '\t\t\t\tr11.u64 = r24.u64;',
    '\t\t\t\tcr6.compare<uint32_t>(l_r7.u32, r21.u32, xer);',
    '\t\t\t\tbreak;',
    '\t\t\t}',
    '\t\t}',
    '\t}',
]


def patch_82524E98(name, body):
    assigned = [text for text, _ in body if text.startswith('\tr24.') and ' = ' in text]
    if assigned != ['\tr24.s64 = 0;']:
        sys.exit(f'hand_copies: {name}: r24 is no longer only set to 0: {assigned}')
    line = '\tif (!cr6.eq) goto loc_825255F4;'
    expect_before(name, body, line, ['\tREX_LOOP_BARRIER();', '\tl_r7.u64 = REX_LOAD_D_U32(r31.u32, 8);',
                                     '\tcr6.compare<uint32_t>(l_r7.u32, r21.u32, xer);'])
    expect_before(name, body, '\tif (cr6.eq) goto loc_8252582C;', ['\tl_r9.u64 = r11.u32 & 0xFF;',
                                                                   '\tcr6.compare<uint32_t>(l_r9.u32, 0, xer);'])
    expect_before(name, body, '\tif (cr6.lt) goto loc_82525530;', ['\tl_r9.u64 = REX_LOAD_D_U32(r25.u32, 32);',
                                                                   '\tr26.s64 = r26.s64 + 1;', '\tr31.s64 = r31.s64 + 64;',
                                                                   '\tcr6.compare<int32_t>(r26.s32, l_r9.s32, xer);'])
    return replace_one(name, body, line, LOOP_82524E98)


# sub_820E5100, the bone palette's loop inside sub_820E7F10 (second console run): a matrix per bone, B at r28 + (the 16-bit
# index at 0(r30) << 6). 21 of the interpreter's 332 samples wait on the first load of each B, the rest of it being the same
# 64-byte line. A hint two bones ahead. The extra load reads an entry the loop itself reads two iterations on (r31 is the
# count left after this one, so r31 > 1 means it exists).
#
# And the call of the matrix native takes its three registers directly: its statement handed r3-r10 over through the context
# for the generated copy, which runs only if the native declines. What is then not in the context is scratch of that call
# (r3, r4, r6-r10: docs in banjo_hand_natives.h), and hand.txt's guard skip already leaves it out of the compare. r5, which this
# loop reads back, is not written by the native and is this statement's own local.
def patch_820E5100(name, body):
    line = '\tr30.s64 = r30.s64 + 8;'
    expect_before(name, body, line, ['\tr31.s64 = r31.s64 + -1;', '\tl_r5.s64 = l_r5.s64 + 64;'])
    body = replace_one(name, body, line, [
        line,
        '\tif (r31.u32 > 1) REX_PREFETCH(((uint32_t)REX_LOAD_D_U16(r30.u32, 8) << 6) + r28.u32, 0);',
    ])
    calls = [text for text, _ in body if 'n_820E0DC8::Native' in text]
    if len(calls) != 1:
        sys.exit(f'hand_copies: {name}: {len(calls)} calls of the matrix native, expected 1')
    return replace_one(name, body, calls[0], [
        '\tif (::banjo::hand::matrix::Ready(l_r3.u32, l_r4.u32, l_r5.u32)) [[likely]] {',
        '\t\t::banjo::hand::matrix::Apply<true>(ctx, base, s, l_r3.u32, l_r4.u32, l_r5.u32);',
        '\t\tif (s.Stopped()) return;',
        '\t} else',
        '\t' + calls[0],
    ])


# sub_822F3CB8, eventDone: Kameo: RePowered's cheat codes (app/src/kameo_codes.cpp) answer for some events while a cheat is on. Inlined
# in 23 copies and reached by every other caller through this copy's hook, so one test of a flag here costs a load where a hook
# would have made a real call of all of those. With a cheat on the guard would see the difference it is meant to see.
def patch_822F3CB8(name, body):
    if not body or not body[0][0].startswith('\ts.Hit('):
        sys.exit(f'hand_copies: {name}: the body does not start with its block count')
    block = body[0][1]
    return body[:1] + [(line, block) for line in [
        '\tif (::g_kameo_event_cheats) [[unlikely]] {',
        '\t\tif (::KameoEventCheated(ctx.r3.u32)) {',
        '\t\t\tctx.r3.u64 = 1;',
        '\t\t\treturn;',
        '\t\t}',
        '\t}',
    ]] + body[1:]


PATCHES = {'sub_82379E10': patch_82379E10, 'sub_82524E98': patch_82524E98, 'sub_820E5100': patch_820E5100,
           'sub_822F3CB8': patch_822F3CB8}

# Registers a function's copy leaves in the context instead of a local of its own. The original statements' own form, so exact
# by construction. For a function with real calls and few uses of the register: the copy syncs each local around every call
# it may have changed on any path, and a loop's back edge makes that nearly all of them. sub_820E7F10, the command-list
# interpreter (second console run, 18 % of its samples on context loads and stores): its 29 real calls carried 286 stores and
# 377 loads of f1-f13, and the 49 handlers use those registers in 22 places.
# r3-r10 too, compiled both ways: 149 fewer instructions and 45 fewer stack accesses (old build 7,399 and 1,090; f only 6,923 and
# 733; both 6,774 and 688). Tried and left out, compiled: sub_8231C268 with f and v (no real calls of its own: 312 more
# instructions and 82 more context stores), and sub_82110280, whose hand-over is 2.5 % of its samples.
KEEP_IN_CONTEXT = {'sub_820E7F10': {f'f{k}' for k in range(1, 14)} | {f'r{k}' for k in range(3, 11)}}


def apply(name, body):
    patch = PATCHES.get(name)
    return patch(name, body) if patch else body
