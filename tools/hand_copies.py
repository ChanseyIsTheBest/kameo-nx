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
LOOP_82379E10 = [
    '\tif (cr6.lt) {',
    '\t\tconst uint32_t hand_owner = REX_LOAD_D_U32(l_r1.u32, 716);',
    '\t\tfor (;;) {',
    '\t\t\tREX_LOOP_BARRIER();',
    '\t\t\tREX_PREFETCH(r28.u32 + 512, 0);',
    '\t\t\tconst uint32_t hand_element = REX_LOAD_D_U32(r28.u32, -8);',
    '\t\t\tif (hand_element == hand_owner) goto loc_8237A2E0;',
    '\t\t\tl_r3.u64 = REX_LOAD_D_U32(r18.u32, 0);',
    '\t\t\tr22.s64 = r22.s64 + 1;',
    '\t\t\tr28.s64 = r28.s64 + 64;',
    '\t\t\tcr6.compare<int32_t>(r22.s32, l_r3.s32, xer);',
    '\t\t\tif (!cr6.lt) {',
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


PATCHES = {'sub_82379E10': patch_82379E10}


def apply(name, body):
    patch = PATCHES.get(name)
    return patch(name, body) if patch else body
