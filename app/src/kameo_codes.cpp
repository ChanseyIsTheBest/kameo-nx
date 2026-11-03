// kameo-nx (2026-11-03, the user's request): Kameo: RePowered's cheat codes. Eight presses of X, Y and the bumpers on any pause
// or options page reveal a hidden feature for good: Level Select, Unlock All Extras, All Warriors & Upgrades and the
// collectible tracker. Until a code is entered nothing is added to any page, so the game is as it was.
//
// Adapted from Kameo: RePowered by MaxDeadBear and BiRabittoh (src/ui/pause_menu/, src/ui/text_window.cpp, src/engine/unlocks.cpp,
// src/engine/kept_progress.cpp, src/engine/collectibles.cpp; its host-side source is under the MIT License: THIRD_PARTY_NOTICES.md).
//
// What differs from RePowered, and why:
//   * This port runs title update 2, and RePowered's sources are written against the original game. Addresses come from its own
//     tools/tu maps. Where the maps and the update's code disagree, the update's code wins: the cost table's rows are 16 bytes
//     (822F3930 walks them so), the task jump's four words are 828E1F0C-828E1F18 and its last two stores are 64-bit (82441588).
//   * RePowered's All Warriors & Upgrades works only on the original game. The update's kaiSackPopulate (821A3FF8) still adds each
//     warrior by the same call after eventDone of its story event, so the warriors are added after it here, and the moves are
//     their 40 events in the cost table (239-278, the rows of negative cost) answered as done. The fruit balance (822F3200) sums
//     the cost of every done event, so it does not see those answers.
//   * eventDone is not hooked: 23 machine copies inline it. tools/hand_copies.py gives its copy one flag test instead, which also
//     serves every other caller through the copy's hook. With banjo_cpu_native_bulk off the two cheats do nothing.
//   * The pages hang from one "Extras" line on the game's Game Options page, not from RePowered's options pages, and the choices
//     are kept in kameo_codes.txt next to the NRO, which a new package's kameo.toml does not replace.
//   * Resume presses Start for the game instead of writing its pause state.
//   * English only, no tribe shop sales and no save backup or leaderboard mark (RePowered's cheat guard).
//
// Guest functions are called as __imp__sub_X: the original, whatever hooks exist. Those names do not count as hooks for
// tools/direct_calls.py, so the functions keep their machine copies.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/func.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include "cpu/banjo_hot_common.h"

REXCVAR_DEFINE_BOOL(kameo_cheat_codes, true, "Kameo",
                    "RePowered's cheat codes on the pause and options pages (kameo_codes.cpp). Nothing changes until one is entered")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);
REXCVAR_DEFINE_INT32(kameo_codes_test_unlock, 0, "Kameo",
                     "For tests: features unlocked as if their codes had been entered, not saved. 1 Level Select, 2 Unlock All "
                     "Extras, 4 All Warriors & Upgrades, 8 Collectibles")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

REX_EXTERN(__imp__sub_824454B0);  // storybookSimplePauseObj_tick(pauseObj)
REX_EXTERN(__imp__sub_82445BD8);  // setupMenuItemsForSettingsMenu(window, pauseObj)
REX_EXTERN(__imp__sub_82445870);  // activateSimplePauseSettingsMenu(pauseObj, window)
REX_EXTERN(__imp__sub_82447080);  // the settings pages' B handler (window, _, pauseObj)
REX_EXTERN(__imp__sub_824470D0);  // storybookSimplePauseMenuPressACallback(window, index, pauseObj)
REX_EXTERN(__imp__sub_82278C30);  // where Start opens and closes a gameplay pause
REX_EXTERN(__imp__sub_820B7BA8);  // LevelChange(name)
REX_EXTERN(__imp__sub_821A3FF8);  // kaiSackPopulate(sack)
REX_EXTERN(__imp__sub_821A3C30);  // kaiSackPopulateSack(sack, warrior name)
REX_EXTERN(__imp__sub_822F3200);  // the fruit balance: the cost of every done event
REX_EXTERN(__imp__sub_822F3CB8);  // eventDone(event)
REX_EXTERN(__imp__sub_822F3F00);  // eventSetEx(event, value, score)
REX_EXTERN(__imp__sub_822F87E0);  // taskDone(task)
REX_EXTERN(__imp__sub_822F8958);  // taskStartTaskNoLevelChange(task, 1)
REX_EXTERN(__imp__sub_822F69E0);  // taskStartLevel(task)
REX_EXTERN(__imp__sub_820BD3B0);  // saves the game
REX_EXTERN(__imp__sub_8210C6F0);  // adds a line to a text window
REX_EXTERN(__imp__sub_8210CEE0);  // textWindowScrollToMenuItem(window, index, seconds)
REX_EXTERN(__imp__sub_821095C0);  // clears a text object's lines
REX_EXTERN(__imp__sub_822B1090);  // 1 in a network game
REX_EXTERN(__imp__sub_825531D8);  // a localised string by its id
REX_EXTERN(__imp__sub_82317EA8);  // plays a sound

// The eventDone copy's two (tools/hand_copies.py; cpu/banjo_hot_common.h declares them). Bit 0 Unlock All Extras, bit 1 All Warriors.
uint8_t g_kameo_event_cheats = 0;

namespace {

thread_local bool t_in_fruit_balance = false;

}  // namespace

bool KameoEventCheated(uint32_t event) {
  // The mirror's unlockables table (82783EF0, 42 rows) names events 485-526: every cheat, costume and extra is gated on one.
  if ((g_kameo_event_cheats & 1) && event >= 485 && event <= 526)
    return true;
  // The warrior moves, the cost table's 40 rows of negative cost.
  if ((g_kameo_event_cheats & 2) && event >= 239 && event <= 278 && !t_in_fruit_balance)
    return true;
  return false;
}

namespace {

// --- Guest memory ---------------------------------------------------------------------------------------------------------
uint8_t Load8(uint8_t *base, uint32_t a) { return *::banjo::hot::Raw(base, a); }
uint16_t Load16(uint8_t *base, uint32_t a) {
  uint16_t v;
  std::memcpy(&v, ::banjo::hot::Raw(base, a), 2);
  return __builtin_bswap16(v);
}
uint32_t Load32(uint8_t *base, uint32_t a) {
  uint32_t v;
  std::memcpy(&v, ::banjo::hot::Raw(base, a), 4);
  return __builtin_bswap32(v);
}
float LoadFloat(uint8_t *base, uint32_t a) {
  const uint32_t bits = Load32(base, a);
  float f;
  std::memcpy(&f, &bits, 4);
  return f;
}
void Store8(uint8_t *base, uint32_t a, uint8_t v) {
  ::banjo::hot::Card(base, a, 1);
  *::banjo::hot::Raw(base, a) = v;
}
void Store16(uint8_t *base, uint32_t a, uint16_t v) {
  ::banjo::hot::Card(base, a, 2);
  v = __builtin_bswap16(v);
  std::memcpy(::banjo::hot::Raw(base, a), &v, 2);
}
void Store32(uint8_t *base, uint32_t a, uint32_t v) {
  ::banjo::hot::Card(base, a, 4);
  v = __builtin_bswap32(v);
  std::memcpy(::banjo::hot::Raw(base, a), &v, 4);
}
void Store64(uint8_t *base, uint32_t a, uint64_t v) {
  ::banjo::hot::Card(base, a, 8);
  v = __builtin_bswap64(v);
  std::memcpy(::banjo::hot::Raw(base, a), &v, 8);
}
void StoreFloat(uint8_t *base, uint32_t a, float f) {
  uint32_t bits;
  std::memcpy(&bits, &f, 4);
  Store32(base, a, bits);
}

// A guest call on a copy of the registers. The flush mode a callee leaves is the hardware's, so the copy's record of it comes back.
template <class F>
PPCContext Call(PPCContext &ctx, uint8_t *base, F function, uint64_t r3 = 0, uint64_t r4 = 0, uint64_t r5 = 0) {
  PPCContext call = ctx;
  call.r3.u64 = r3;
  call.r4.u64 = r4;
  call.r5.u64 = r5;
  function(call, base);
  ctx.fpscr = call.fpscr;
  return call;
}

// --- The game's controller records ------------------------------------------------------------------------------------------
constexpr uint32_t kPlayerControllers = 0x82784510;
constexpr uint32_t kPlayerControllerBytes = 36;
constexpr uint32_t kSticks = 0x827C0FD8;
constexpr uint32_t kStickBytes = 320;
constexpr uint32_t kStickPressed = 8;
constexpr uint32_t kPlayerList = 0x828683D8;
constexpr uint32_t kButtonStart = 0x0010;
constexpr uint32_t kButtonLB = 0x0100;
constexpr uint32_t kButtonRB = 0x0200;
constexpr uint32_t kButtonX = 0x4000;
constexpr uint32_t kButtonY = 0x8000;

int PausePlayer(uint8_t *base, uint32_t pause) {
  return std::clamp<int>(int8_t(Load8(base, Load32(base, pause + 8) + 146)), 0, 3);
}
uint32_t PlayerStick(uint8_t *base, int player) {
  return Load32(base, kSticks) + Load8(base, kPlayerControllers + kPlayerControllerBytes * uint32_t(player)) * kStickBytes;
}

// --- Text windows -----------------------------------------------------------------------------------------------------------
constexpr uint32_t kWindowTextObject = 16;
constexpr uint32_t kTextFirstItem = 124;
constexpr uint32_t kTextColourCallback = 176;
constexpr uint32_t kTextColourCallbackData = 180;
constexpr uint32_t kTextMenu = 256;
constexpr uint32_t kItemNext = 4;
constexpr uint32_t kItemSelectable = 8;
constexpr uint32_t kItemLineCount = 16;
constexpr uint32_t kItemHeight = 108;
constexpr uint32_t kItemTop = 128;
constexpr uint32_t kItemId = 144;
constexpr uint32_t kMenuScrollStart = 84;
constexpr uint32_t kMenuScroll = 88;
constexpr uint32_t kMenuScrollTarget = 92;
constexpr uint32_t kMenuBackCallback = 100;
constexpr uint32_t kMenuChangedCallback = 104;
constexpr uint32_t kMenuCallbackData = 108;
constexpr uint32_t kMenuPreviousIndex = 112;
constexpr uint32_t kMenuSelectedIndex = 114;

constexpr uint32_t kStringTable = 0x827A9B68;
constexpr uint32_t kDefaultColourCallback = 0x82108870;
constexpr uint32_t kOverwritePreviousLine = 0x82BCE931;
constexpr uint32_t kPressACallback = 0x824470D0;
constexpr uint32_t kBackCallback = 0x82447080;
// Localised layout strings the settings page itself uses.
constexpr int32_t kLabelFormatString = 588;
constexpr int32_t kValueFormatString = 4451;
constexpr int32_t kPageTitleString = 383;
constexpr int32_t kCentredLineString = 380;

uint32_t TextObjectOf(uint8_t *base, uint32_t window) { return Load32(base, window + kWindowTextObject); }
uint32_t MenuOf(uint8_t *base, uint32_t text) { return Load32(base, text + kTextMenu); }

// A UTF-16 copy of 'text' in guest memory, allocated once per distinct string and never freed.
uint32_t GuestText(uint8_t *base, const std::u16string &text) {
  static std::mutex mutex;
  static std::unordered_map<std::u16string, uint32_t> cache;
  std::lock_guard<std::mutex> lock(mutex);
  if (const auto found = cache.find(text); found != cache.end())
    return found->second;
  auto *memory = rex::system::kernel_state()->memory();
  const uint32_t address = memory ? memory->SystemHeapAlloc(uint32_t(text.size() + 1) * 2) : 0;
  if (!address)
    return 0;
  for (size_t i = 0; i <= text.size(); ++i)
    Store16(base, address + uint32_t(i) * 2, i < text.size() ? uint16_t(text[i]) : 0);
  cache.emplace(text, address);
  return address;
}

std::u16string Wide(const char *text) {
  std::u16string out;
  for (; *text; ++text)
    out += char16_t(uint8_t(*text));
  return out;
}
std::u16string Number(int32_t value) { return Wide(std::to_string(value).c_str()); }

uint32_t LocalisedString(PPCContext &ctx, uint8_t *base, int32_t id) {
  return Call(ctx, base, __imp__sub_825531D8, Load32(base, kStringTable), uint32_t(id)).r3.u32;
}

// The game's line with its visible text swapped for 'label', keeping its {font tags} such as centring.
uint32_t StyledLike(uint8_t *base, uint32_t game_text, const std::u16string &label) {
  if (!game_text)
    return GuestText(base, label);
  std::u16string source;
  std::vector<bool> tag;
  bool in_tag = false;
  for (uint32_t i = 0; i < 512; ++i) {
    const char16_t c = char16_t(Load16(base, game_text + i * 2));
    if (!c)
      break;
    const bool opens = !in_tag && c == u'{';
    source += c;
    tag.push_back(in_tag || opens);
    in_tag = (in_tag || opens) && c != u'}';
  }
  size_t first = source.size(), last = 0;
  for (size_t i = 0; i < source.size(); ++i) {
    if (tag[i] || source[i] == u' ' || source[i] == u'\t')
      continue;
    first = std::min(first, i);
    last = i;
  }
  if (first == source.size())
    return GuestText(base, source + label);
  std::u16string styled;
  for (size_t i = 0; i < source.size(); ++i) {
    if (i == first)
      styled += label;
    if (tag[i] || i < first || i > last)
      styled += source[i];
  }
  return GuestText(base, styled);
}

void AddTextItem(PPCContext &ctx, uint8_t *base, uint32_t window, uint32_t id, uint32_t format, uint32_t text, uint32_t callback,
                 bool selectable) {
  PPCContext call = ctx;
  call.r3.u64 = window;
  call.r4.u64 = id;
  call.r5.u64 = format;
  call.r6.u64 = text;
  call.r7.u64 = callback;
  call.f1.f64 = 0.0;
  call.r8.u64 = 0;
  call.r9.s64 = -1;
  call.r10.u64 = selectable ? 1 : 0;
  __imp__sub_8210C6F0(call, base);
  ctx.fpscr = call.fpscr;
}

void AddPageTitle(PPCContext &ctx, uint8_t *base, uint32_t window, const std::u16string &title) {
  if (const uint32_t text = StyledLike(base, LocalisedString(ctx, base, kPageTitleString), title))
    AddTextItem(ctx, base, window, 0, 0, text, 0, false);
}
void AddLine(PPCContext &ctx, uint8_t *base, uint32_t window, uint32_t id, const std::u16string &label) {
  if (const uint32_t text = StyledLike(base, LocalisedString(ctx, base, kCentredLineString), label))
    AddTextItem(ctx, base, window, id, 0, text, kPressACallback, true);
}
// A label and its value on one row, like a setting's.
void AddRow(PPCContext &ctx, uint8_t *base, uint32_t window, uint32_t id, const std::u16string &label, const std::u16string &value) {
  const uint32_t label_text = GuestText(base, label), value_text = GuestText(base, value);
  if (!label_text || !value_text)
    return;
  AddTextItem(ctx, base, window, 0, LocalisedString(ctx, base, kLabelFormatString), label_text, 0, false);
  Store8(base, kOverwritePreviousLine, 1);
  AddTextItem(ctx, base, window, id, LocalisedString(ctx, base, kValueFormatString), value_text, kPressACallback, true);
}

// The selectable item at 'index', as the game's A-press callback counts them.
uint32_t SelectedItemId(uint8_t *base, uint32_t window, uint32_t index) {
  uint32_t seen = 0;
  for (uint32_t item = Load32(base, TextObjectOf(base, window) + kTextFirstItem); item; item = Load32(base, item + kItemNext)) {
    if (Load32(base, item + kItemSelectable) != 1)
      continue;
    if (seen++ == index)
      return Load32(base, item + kItemId);
  }
  return 0;
}
uint32_t SelectableCount(uint8_t *base, uint32_t window) {
  uint32_t seen = 0;
  for (uint32_t item = Load32(base, TextObjectOf(base, window) + kTextFirstItem); item; item = Load32(base, item + kItemNext))
    seen += Load32(base, item + kItemSelectable) == 1;
  return seen;
}
uint32_t SelectedIndex(uint8_t *base, uint32_t window) { return Load8(base, MenuOf(base, TextObjectOf(base, window)) + kMenuSelectedIndex); }

void KeepCursor(PPCContext &ctx, uint8_t *base, uint32_t window, uint32_t index) {
  Store8(base, MenuOf(base, TextObjectOf(base, window)) + kMenuSelectedIndex, uint8_t(index));
  PPCContext scroll = ctx;
  scroll.r3.u64 = window;
  scroll.r4.u64 = index;
  scroll.f1.f64 = 0.0;
  __imp__sub_8210CEE0(scroll, base);
  ctx.fpscr = scroll.fpscr;
}

// A scroll that stops part way through a line moves on to that line's end, so no cut-off half line shows at the top.
void ScrollToWholeLine(uint8_t *base, uint32_t window) {
  constexpr float kPastBoundary = 0.25f;
  const uint32_t text = TextObjectOf(base, window);
  const uint32_t menu = MenuOf(base, text);
  if (!menu)
    return;
  const float target = LoadFloat(base, menu + kMenuScrollTarget);
  if (target <= 0.0f)
    return;
  for (uint32_t item = Load32(base, text + kTextFirstItem); item; item = Load32(base, item + kItemNext)) {
    const float top = LoadFloat(base, item + kItemTop);
    const float height = LoadFloat(base, item + kItemHeight);
    const float line = height / float(std::max<uint32_t>(1, Load8(base, item + kItemLineCount)));
    if (line <= 0.0f || target < top || target >= top + height)
      continue;
    const float lines = (target - top) / line - kPastBoundary - 0.01f;
    const float whole = float(int32_t(lines)) + (lines > float(int32_t(lines)) ? 1.0f : 0.0f);
    const float snapped = top + (whole + kPastBoundary) * line;
    if (snapped - target < 0.5f && target - snapped < 0.5f)
      return;
    StoreFloat(base, menu + kMenuScrollTarget, snapped);
    return;
  }
}

// --- The codes and what they have unlocked ----------------------------------------------------------------------------------
enum Unlock : uint32_t { kLevelSelect = 1, kExtras = 2, kWarriors = 4, kCollectibles = 8 };

struct Code {
  Unlock feature;
  const char *name;
  std::array<uint32_t, 8> buttons;
};
constexpr Code kCodes[] = {
    {kLevelSelect, "Level Select", {kButtonLB, kButtonRB, kButtonLB, kButtonRB, kButtonX, kButtonY, kButtonX, kButtonY}},
    {kExtras, "Unlock All Extras", {kButtonX, kButtonX, kButtonY, kButtonY, kButtonLB, kButtonLB, kButtonRB, kButtonRB}},
    {kWarriors, "All Warriors & Upgrades", {kButtonY, kButtonX, kButtonY, kButtonX, kButtonRB, kButtonLB, kButtonRB, kButtonLB}},
    {kCollectibles, "Collectibles", {kButtonX, kButtonY, kButtonLB, kButtonRB, kButtonX, kButtonY, kButtonLB, kButtonRB}},
};
// The tribe shops' purchase sound.
constexpr uint32_t kCodeAcceptedSound = 0x679D554;

struct Saved {
  uint32_t unlocked = 0;
  bool extras = false, warriors = false;
};
Saved g_saved;
std::array<uint32_t, 8> g_entered{};

std::string CodesPath() { return (rex::filesystem::GetExecutableFolder() / "kameo_codes.txt").string(); }

void ApplyEventCheats() {
  g_kameo_event_cheats = uint8_t(((g_saved.extras && (g_saved.unlocked & kExtras)) ? 1 : 0) |
                                 ((g_saved.warriors && (g_saved.unlocked & kWarriors)) ? 2 : 0));
}

void SaveCodes() {
  if (FILE *f = std::fopen(CodesPath().c_str(), "w")) {
    std::fprintf(f, "unlocked %u\nextras %d\nwarriors %d\n", g_saved.unlocked & 15u, g_saved.extras ? 1 : 0, g_saved.warriors ? 1 : 0);
    std::fclose(f);
  }
  ApplyEventCheats();
}

void LoadCodesOnce() {
  static bool loaded = false;
  if (loaded)
    return;
  loaded = true;
  if (FILE *f = std::fopen(CodesPath().c_str(), "r")) {
    unsigned unlocked = 0;
    int extras = 0, warriors = 0;
    if (std::fscanf(f, "unlocked %u extras %d warriors %d", &unlocked, &extras, &warriors) == 3) {
      g_saved.unlocked = unlocked & 15u;
      g_saved.extras = extras != 0;
      g_saved.warriors = warriors != 0;
    }
    std::fclose(f);
  }
  g_saved.unlocked |= uint32_t(REXCVAR_GET(kameo_codes_test_unlock)) & 15u;
  ApplyEventCheats();
}

// --- The pages --------------------------------------------------------------------------------------------------------------
enum class Page { None, Extras, LevelSelect, LevelChapter, ConfirmJump, Collectibles };

// Item ids the game's own dispatch never uses (its are hashes below 2^28).
constexpr uint32_t kItemExtrasPage = 0x7A5E0E00;
constexpr uint32_t kItemToggleExtras = 0x7A5E0E01;
constexpr uint32_t kItemToggleWarriors = 0x7A5E0E02;
constexpr uint32_t kItemLevelSelectPage = 0x7A5E0700;
constexpr uint32_t kItemLevelChapter = 0x7A5E0710;
constexpr uint32_t kItemLevelTask = 0x7A5E0800;
constexpr uint32_t kItemJumpStart = 0x7A5E0606;
constexpr uint32_t kItemJumpCancel = 0x7A5E0607;
constexpr uint32_t kItemCollectiblesPage = 0x7A5E0900;
constexpr uint32_t kItemCollectibleRow = 0x7A5E0910;

// The task table taskStartLevel reads, 48 bytes each: level name at +0, checkpoint name string at +40.
constexpr uint32_t kTaskTable = 0x8205E93C;
constexpr uint32_t kTaskEntryBytes = 48;
constexpr uint32_t kTaskNameString = 40;

struct Chapter {
  const char *title;
  uint16_t first_task, last_task;
};
constexpr Chapter kChapters[] = {
    {"Thorn's Castle", 1, 13},       {"Enchanted Kingdom", 14, 31}, {"The Badlands", 32, 36},   {"Forgotten Forest", 37, 64},
    {"Forest Temple", 65, 68},       {"Mountain Falls", 69, 84},    {"Mountain Rise", 85, 107}, {"Water Temple", 108, 125},
    {"Snow Top Village", 126, 152},  {"Snow Temple", 153, 168},     {"Ogre's Swamp", 169, 184}, {"Thorn's Pass", 185, 202},
    {"Thorn's Airship", 203, 225},
};

enum class Kind : uint8_t { Fruit, Elixir, CrystalEye, Costume };
enum class Source : uint8_t { Level, Shop, Trainer };
struct Collectible {
  Kind kind;
  uint32_t level;  // ELF hash of the level's name
  uint32_t event;
  uint8_t value;  // fruit come as singles and triples; the game's total of 100 counts them by value
  Source source;
  int16_t last_task;      // the last story task set in the level, or -1 for a side area that stays reachable
  int32_t warrior_event;  // a costume's warrior, whose story event must be done first; -1 otherwise
};
const Collectible kCollectibleTable[] = {
#include "kameo_collectibles.inc"
};

Page g_page = Page::None;
uint32_t g_chapter = 0;
uint32_t g_picked_task = 0;
int32_t g_jump_task = -1;
bool g_close_requested = false;
bool g_in_game = false;  // Level Select and the tracker only appear in a game's pause menu, not the main menu's options
std::string g_level;     // the name last passed to LevelChange
std::array<uint32_t, 4> g_sacks{};  // the warrior sacks filled since the last level change

bool IsOptionsObject(uint8_t *base, uint32_t pause) {
  const uint32_t header = pause ? Load32(base, pause + 8) : 0;
  if (!header)
    return false;
  static const char kName[] = "StoryBookSimpleOptionsObj";
  for (uint32_t i = 0; i < sizeof(kName); ++i)  // the name's place in the update's object record
    if (Load8(base, header + 196 + i) != uint8_t(kName[i]))
      return false;
  return true;
}

// The task's checkpoint name as the game's save list shows it, without its {tags} or line breaks.
std::u16string TaskName(PPCContext &ctx, uint8_t *base, uint32_t task) {
  const uint32_t text = LocalisedString(ctx, base, int32_t(Load32(base, kTaskTable + kTaskEntryBytes * task + kTaskNameString)));
  std::u16string name;
  bool in_tag = false;
  for (uint32_t i = 0; text && i < 256; ++i) {
    const char16_t c = char16_t(Load16(base, text + i * 2));
    if (!c)
      break;
    if (c == u'{')
      in_tag = true;
    else if (c == u'}')
      in_tag = false;
    else if (!in_tag)
      name.push_back(c == u'\n' ? u' ' : c);
  }
  while (!name.empty() && name.front() == u' ')
    name.erase(name.begin());
  while (!name.empty() && name.back() == u' ')
    name.pop_back();
  return name.empty() ? Number(int32_t(task)) : name;
}

// Most task names end in their area's name, which the chapter page already shows as its title: whichever " - " ending most share.
std::vector<std::u16string> WithoutChapter(std::vector<std::u16string> names) {
  std::map<std::u16string, size_t> endings;
  for (const std::u16string &name : names)
    if (const size_t dash = name.rfind(u" - "); dash != std::u16string::npos && dash > 0)
      ++endings[name.substr(dash)];
  const auto common = std::max_element(endings.begin(), endings.end(), [](const auto &a, const auto &b) { return a.second < b.second; });
  if (common == endings.end() || common->second * 2 < names.size())
    return names;
  for (std::u16string &name : names)
    if (name.size() > common->first.size() && name.compare(name.size() - common->first.size(), common->first.size(), common->first) == 0)
      name.resize(name.size() - common->first.size());
  return names;
}

bool OriginalEventDone(PPCContext &ctx, uint8_t *base, uint32_t event) { return Call(ctx, base, __imp__sub_822F3CB8, event).r3.u32 != 0; }

uint32_t CurrentLevelToken() {
  uint32_t hash = 0;
  for (const char c : g_level) {
    hash = (hash << 4) + uint8_t(c);
    const uint32_t high = hash & 0xF0000000u;
    if (high)
      hash ^= high >> 24;
    hash &= ~high;
  }
  return hash;
}

bool CollectibleMissed(PPCContext &ctx, uint8_t *base, const Collectible &item, bool found) {
  if (item.source != Source::Level || item.last_task < 0 || found)
    return false;
  if (item.warrior_event >= 0 && !OriginalEventDone(ctx, base, uint32_t(item.warrior_event)))
    return false;
  return Call(ctx, base, __imp__sub_822F87E0, uint32_t(item.last_task)).r3.u32 != 0;
}

void AddCollectibleRows(PPCContext &ctx, uint8_t *base, uint32_t window) {
  struct Tally {
    uint32_t found = 0, total = 0;
  };
  static const char *const kHere[4] = {"Fruit in This Area", "Elixirs in This Area", "Crystal Eye in This Area", "Costumes in This Area"};
  static const char *const kAll[4] = {"All Elemental Fruit", "All Elixirs of Life", "All Crystal Eyes", "All Costumes"};
  std::array<Tally, 4> here{}, all{};
  uint32_t missed = 0;
  const uint32_t level = CurrentLevelToken();
  for (const Collectible &item : kCollectibleTable) {
    const size_t kind = size_t(item.kind);
    const uint32_t weight = item.kind == Kind::Fruit ? item.value : 1;
    const bool found = OriginalEventDone(ctx, base, item.event);
    all[kind].total += weight;
    all[kind].found += found ? weight : 0;
    if (item.level == level) {
      here[kind].total += weight;
      here[kind].found += found ? weight : 0;
    }
    missed += CollectibleMissed(ctx, base, item, found);
  }
  const auto out_of = [](const Tally &t) { return Number(int32_t(t.found)) + u" / " + Number(int32_t(t.total)); };
  AddPageTitle(ctx, base, window, u"Collectibles");
  uint32_t row = 0;
  bool anything_here = false;
  for (size_t kind = 0; kind < 4; ++kind) {
    if (here[kind].total == 0)
      continue;
    anything_here = true;
    AddRow(ctx, base, window, kItemCollectibleRow + row++, Wide(kHere[kind]), out_of(here[kind]));
  }
  if (!anything_here)
    AddRow(ctx, base, window, kItemCollectibleRow + row++, u"This Area", u"Nothing to find");
  for (size_t kind = 0; kind < 4; ++kind)
    AddRow(ctx, base, window, kItemCollectibleRow + row++, Wide(kAll[kind]), out_of(all[kind]));
  AddRow(ctx, base, window, kItemCollectibleRow + row++, u"Missed", Number(int32_t(missed)));
}

void AddPageRows(PPCContext &ctx, uint8_t *base, uint32_t window, Page page) {
  switch (page) {
  case Page::Extras:
    AddPageTitle(ctx, base, window, u"Extras");
    if (g_saved.unlocked & kExtras)
      AddRow(ctx, base, window, kItemToggleExtras, u"Unlock All Extras", g_saved.extras ? u"On" : u"Off");
    if (g_saved.unlocked & kWarriors)
      AddRow(ctx, base, window, kItemToggleWarriors, u"All Warriors & Upgrades", g_saved.warriors ? u"On" : u"Off");
    if (g_in_game && (g_saved.unlocked & kLevelSelect))
      AddLine(ctx, base, window, kItemLevelSelectPage, u"Level Select");
    if (g_in_game && (g_saved.unlocked & kCollectibles))
      AddLine(ctx, base, window, kItemCollectiblesPage, u"Collectibles");
    break;
  case Page::LevelSelect:
    AddPageTitle(ctx, base, window, u"Level Select");
    for (uint32_t i = 0; i < std::size(kChapters); ++i)
      AddLine(ctx, base, window, kItemLevelChapter + i, Wide(kChapters[i].title));
    break;
  case Page::LevelChapter: {
    const Chapter &chapter = kChapters[g_chapter];
    AddPageTitle(ctx, base, window, Wide(chapter.title));
    std::vector<std::u16string> names;
    for (uint32_t task = chapter.first_task; task <= chapter.last_task; ++task)
      names.push_back(TaskName(ctx, base, task));
    names = WithoutChapter(std::move(names));
    for (uint32_t i = 0; i < names.size(); ++i)
      AddLine(ctx, base, window, kItemLevelTask + chapter.first_task + i, Number(int32_t(i + 1)) + u". " + names[i]);
    break;
  }
  case Page::ConfirmJump:
    AddPageTitle(ctx, base, window, u"Start from \"" + TaskName(ctx, base, g_picked_task) + u"\"?");
    if (const uint32_t note = GuestText(base, u"The story is set to this point;\ncollectibles are kept."))
      AddTextItem(ctx, base, window, 0, GuestText(base, u"{alignment='centre' size='21'}"), note, 0, false);
    AddLine(ctx, base, window, kItemJumpStart, u"Start");
    AddLine(ctx, base, window, kItemJumpCancel, u"Cancel");
    break;
  case Page::Collectibles:
    AddCollectibleRows(ctx, base, window);
    break;
  case Page::None:
    break;
  }
}

// The same page reset activateSimplePauseSettingsMenu performs, then our page's lines instead of the game's settings.
void ActivatePage(PPCContext &ctx, uint8_t *base, uint32_t pause, uint32_t window, Page page) {
  const uint32_t text = TextObjectOf(base, window);
  const uint32_t menu = MenuOf(base, text);
  for (const uint32_t field : {144u, kMenuScroll, kMenuScrollTarget, kMenuChangedCallback, kMenuScrollStart})
    Store32(base, menu + field, 0);
  Store32(base, menu + kMenuBackCallback, kBackCallback);
  Store32(base, menu + kMenuCallbackData, pause);
  Store8(base, menu + kMenuSelectedIndex, 0);
  Store16(base, menu + kMenuPreviousIndex, 0xFFFF);
  Store16(base, menu, uint16_t(Load16(base, menu) & 0xA));
  Call(ctx, base, __imp__sub_821095C0, text);
  Store32(base, text + kTextColourCallback, kDefaultColourCallback);
  Store32(base, text + kTextColourCallbackData, 0);
  g_in_game = !IsOptionsObject(base, pause);
  AddPageRows(ctx, base, window, page);
  g_page = page;
  Store32(base, Load32(base, pause + 512) + 120, 1);
}

void TickCodes(PPCContext &ctx, uint8_t *base, uint32_t pause, uint32_t window, uint32_t pressed) {
  bool unlocked = false;
  for (const uint32_t button : {kButtonX, kButtonY, kButtonLB, kButtonRB}) {
    if ((pressed & button) == 0)
      continue;
    for (size_t i = 0; i + 1 < g_entered.size(); ++i)
      g_entered[i] = g_entered[i + 1];
    g_entered.back() = button;
    for (const Code &code : kCodes) {
      if (g_entered != code.buttons || (g_saved.unlocked & code.feature))
        continue;
      g_saved.unlocked |= code.feature;
      SaveCodes();
      REXLOG_INFO("[kameo] cheat code entered: {}", code.name);
      g_entered = {};
      unlocked = true;
    }
  }
  if (!unlocked)
    return;
  Call(ctx, base, __imp__sub_82317EA8, kCodeAcceptedSound);
  // The newly revealed line shows at once on the page it belongs to; the game's settings page is rebuilt by the game.
  const uint32_t index = SelectedIndex(base, window);
  if (g_page == Page::Extras) {
    ActivatePage(ctx, base, pause, window, Page::Extras);
    KeepCursor(ctx, base, window, index);
  } else if (g_page == Page::None && Load32(base, Load32(base, pause + 512) + 120) == 1) {
    Call(ctx, base, __imp__sub_82445870, pause, window);
    KeepCursor(ctx, base, window, index);
  }
}

// Every warrior, by the names kaiSackPopulate passes for its story events (43-59). The add refuses a name already in the sack.
void AddAllWarriors(PPCContext &ctx, uint8_t *base, uint32_t sack) {
  static constexpr uint32_t kWarriorNames[] = {0x82074C18, 0x82071FF8, 0x82074BDC, 0x82074BE8, 0x82074BF8,
                                               0x82074C0C, 0x82074BD4, 0x82071FF0, 0x82072000, 0x82074BC8};
  for (const uint32_t name : kWarriorNames)
    Call(ctx, base, __imp__sub_821A3C30, sack, name);
}

// --- Level Select's jump ----------------------------------------------------------------------------------------------------
// The four tables eventRememberEventsDone (822F3930) snapshots: elemental fruit and warrior moves, Battle Point unlocks, Crystal
// Eyes and money pouches, health upgrades. The wipe of a task jump would clear them.
struct EventTable {
  uint32_t address, rows, stride;
};
constexpr EventTable kRememberedTables[] = {{0x82783398, 114, 16}, {0x82783EF0, 42, 24}, {0x82783358, 8, 8}, {0x8279F4F8, 12, 8}};
// eventSetEx awards an event's score only while this is clear.
constexpr uint32_t kDoingTaskStart = 0x82BCF2B4;

void StartPendingJump(PPCContext &ctx, uint8_t *base) {
  const int32_t task = g_jump_task;
  g_jump_task = -1;
  if (task < 0 || Call(ctx, base, __imp__sub_822B1090).r3.u32 != 0)  // not in a network game
    return;
  // Neither cheat may answer for an event while its real state is read, cleared and set back.
  const uint8_t cheats = g_kameo_event_cheats;
  g_kameo_event_cheats = 0;
  std::vector<uint32_t> kept;
  for (const EventTable &table : kRememberedTables) {
    for (uint32_t row = 0; row < table.rows; ++row) {
      const uint32_t event = Load32(base, table.address + row * table.stride);
      if (OriginalEventDone(ctx, base, event))
        kept.push_back(event);
    }
  }
  // 82441588, the storybook's task jump, for one player: the starting stats and the per-player counters.
  for (uint32_t i = 0; i < 8; ++i)
    Store32(base, 0x828E1BB8u + 4 * i, Load32(base, 0x8278537Cu + 8 * i));
  for (const uint32_t address : {0x828E1F0Cu, 0x828E1F10u, 0x828E1F14u, 0x828E1F18u})
    Store32(base, address, 1);
  for (const uint32_t first : {0x828F8D9Cu, 0x828F8148u, 0x828F8DA4u, 0x828E19A0u, 0x828E1BB0u, 0x828E1978u})
    for (uint32_t i = 0; i < 4; ++i)
      Store16(base, first + 2 * i, 0);
  Call(ctx, base, __imp__sub_822F8958, uint32_t(task), 1);
  // The wipe also cleared collected fruit, upgrades and unlocks: set back before the level loads and hides what was picked up.
  const uint32_t doing = Load32(base, kDoingTaskStart);
  Store32(base, kDoingTaskStart, 1);
  for (const uint32_t event : kept)
    Call(ctx, base, __imp__sub_822F3F00, event, 1, 0);
  Store32(base, kDoingTaskStart, doing);
  Call(ctx, base, __imp__sub_820BD3B0, 1);
  Call(ctx, base, __imp__sub_822F69E0, uint32_t(task));
  Store64(base, 0x828F8850, 0);
  Store64(base, 0x828F8858, 0);
  g_kameo_event_cheats = cheats;
  REXLOG_INFO("[kameo] level select: started task {}, {} events kept", task, kept.size());
}

}  // namespace

// --- The hooks --------------------------------------------------------------------------------------------------------------
// The generated code keeps r1 in a local and can leave its last frame's value in ctx, so a hook that goes on to call guest code
// after an original puts it back first.
extern "C" REX_FUNC(sub_824454B0) {
  const uint32_t pause = ctx.r3.u32;
  const uint64_t r1 = ctx.r1.u64;
  __imp__sub_824454B0(ctx, base);
  ctx.r1.u64 = r1;
  if (!REXCVAR_GET(kameo_cheat_codes) || !pause || !Load32(base, pause + 8))
    return;
  LoadCodesOnce();
  const uint32_t record = Load32(base, pause + 512);
  const uint32_t window = record ? Load32(base, record + 68) : 0;
  if (!window)
    return;
  if (g_page != Page::None)
    ScrollToWholeLine(base, window);
  TickCodes(ctx, base, pause, window, Load32(base, PlayerStick(base, PausePlayer(base, pause)) + kStickPressed));
}

// The game's Game Options page gains the Extras line once a code has been entered.
extern "C" REX_FUNC(sub_82445BD8) {
  const uint32_t window = ctx.r3.u32;
  const uint32_t pause = ctx.r4.u32;
  const uint64_t r1 = ctx.r1.u64;
  g_page = Page::None;
  __imp__sub_82445BD8(ctx, base);
  ctx.r1.u64 = r1;
  if (!window || !REXCVAR_GET(kameo_cheat_codes))
    return;
  LoadCodesOnce();
  // On the main menu's options only the two toggles have a place.
  const uint32_t shown = IsOptionsObject(base, pause) ? g_saved.unlocked & (kExtras | kWarriors) : g_saved.unlocked;
  if (shown)
    AddLine(ctx, base, window, kItemExtrasPage, u"Extras");
}

extern "C" REX_FUNC(sub_82447080) {
  const uint32_t window = ctx.r3.u32;
  const uint32_t pause = ctx.r5.u32;
  switch (g_page) {
  case Page::ConfirmJump:
    ActivatePage(ctx, base, pause, window, Page::LevelChapter);
    KeepCursor(ctx, base, window, g_picked_task - kChapters[g_chapter].first_task);
    return;
  case Page::LevelChapter:
    ActivatePage(ctx, base, pause, window, Page::LevelSelect);
    KeepCursor(ctx, base, window, g_chapter);
    return;
  case Page::LevelSelect:
  case Page::Collectibles:
    ActivatePage(ctx, base, pause, window, Page::Extras);
    return;
  case Page::Extras: {
    g_page = Page::None;
    Call(ctx, base, __imp__sub_82445870, pause, window);
    KeepCursor(ctx, base, window, SelectableCount(base, window) - 1);  // back on the Extras line, the page's last
    return;
  }
  case Page::None:
    break;
  }
  __imp__sub_82447080(ctx, base);
}

extern "C" REX_FUNC(sub_824470D0) {
  const uint32_t window = ctx.r3.u32;
  const uint32_t index = ctx.r4.u32 & 0xFF;
  const uint32_t pause = ctx.r5.u32;
  const uint32_t id = window ? SelectedItemId(base, window, index) : 0;
  const auto toggle = [&](bool &value) {
    value = !value;
    SaveCodes();
    ActivatePage(ctx, base, pause, window, Page::Extras);
    KeepCursor(ctx, base, window, index);
  };
  if (id == kItemExtrasPage) {
    ActivatePage(ctx, base, pause, window, Page::Extras);
  } else if (id == kItemToggleExtras) {
    toggle(g_saved.extras);
  } else if (id == kItemToggleWarriors) {
    toggle(g_saved.warriors);
    // Sacks are only filled when a level builds the player, so a level already running gets the warriors here.
    if (g_saved.warriors) {
      for (uint32_t player = 0; player < g_sacks.size(); ++player)
        if (g_sacks[player] && Load32(base, kPlayerList + 4 * player))
          AddAllWarriors(ctx, base, g_sacks[player]);
    }
  } else if (id == kItemLevelSelectPage) {
    ActivatePage(ctx, base, pause, window, Page::LevelSelect);
  } else if (id >= kItemLevelChapter && id < kItemLevelChapter + std::size(kChapters)) {
    g_chapter = id - kItemLevelChapter;
    ActivatePage(ctx, base, pause, window, Page::LevelChapter);
  } else if (id > kItemLevelTask && id <= kItemLevelTask + kChapters[std::size(kChapters) - 1].last_task) {
    g_picked_task = id - kItemLevelTask;
    ActivatePage(ctx, base, pause, window, Page::ConfirmJump);
    KeepCursor(ctx, base, window, 1);  // on Cancel, so a stray second press replaces nothing
  } else if (id == kItemJumpStart) {
    g_jump_task = int32_t(g_picked_task);
    g_close_requested = true;
  } else if (id == kItemJumpCancel) {
    ActivatePage(ctx, base, pause, window, Page::LevelChapter);
    KeepCursor(ctx, base, window, g_picked_task - kChapters[g_chapter].first_task);
  } else if (id == kItemCollectiblesPage) {
    ActivatePage(ctx, base, pause, window, Page::Collectibles);
  } else if (id >= kItemCollectibleRow && id < kItemCollectibleRow + 16) {
    // The tracker's rows do nothing when pressed.
  } else {
    __imp__sub_824470D0(ctx, base);
  }
}

// Level Select's Start closes the pause as its owner's Start press does, read here from the pressed word, then the jump runs from
// here, outside the pause object's own tick.
extern "C" REX_FUNC(sub_82278C30) {
  constexpr uint32_t kPauseRequestFlags = 0x82BCEE90;
  constexpr uint32_t kStoredPauseValue = 0x82BCEE94;
  constexpr uint32_t kSimplePauseObject = 0x828E12EC;
  constexpr uint32_t kSimplePauseBit = 0x10;
  const uint32_t pause = g_close_requested ? Load32(base, kSimplePauseObject) : 0;
  if (!pause || (Load32(base, kStoredPauseValue) & kSimplePauseBit) == 0) {
    g_close_requested = false;
    g_jump_task = -1;
    __imp__sub_82278C30(ctx, base);
    return;
  }
  const uint64_t r1 = ctx.r1.u64;
  const uint32_t word = PlayerStick(base, PausePlayer(base, pause)) + kStickPressed;
  const uint32_t before = Load32(base, word);
  Store32(base, word, before | kButtonStart);
  __imp__sub_82278C30(ctx, base);
  Store32(base, word, before);
  ctx.r1.u64 = r1;
  if (Load32(base, kPauseRequestFlags) & kSimplePauseBit)
    return;  // the game was not taking input on this frame: the next one tries again
  g_close_requested = false;
  g_page = Page::None;
  StartPendingJump(ctx, base);
}

extern "C" REX_FUNC(sub_820B7BA8) {
  std::string name;
  for (uint32_t i = 0; i <= 31; ++i) {
    const char c = char(Load8(base, ctx.r3.u32 + i));
    if (!c)
      break;
    name.push_back(c);
  }
  if (!name.empty()) {
    g_level = name;
    g_sacks = {};
  }
  LoadCodesOnce();  // the first level starts before any pause: the cheats hold from then on
  __imp__sub_820B7BA8(ctx, base);
}

// The warriors go in after the game's own population, so its order stands and its adds of those it allows come first.
extern "C" REX_FUNC(sub_821A3FF8) {
  const uint32_t sack = ctx.r3.u32;
  const uint64_t r1 = ctx.r1.u64;
  __imp__sub_821A3FF8(ctx, base);
  ctx.r1.u64 = r1;
  if (!sack)
    return;
  for (uint32_t &slot : g_sacks) {
    if (slot == sack)
      break;
    if (!slot) {
      slot = sack;
      break;
    }
  }
  if (g_kameo_event_cheats & 2) {
    const uint64_t result = ctx.r3.u64;
    AddAllWarriors(ctx, base, sack);
    ctx.r3.u64 = result;
  }
}

extern "C" REX_FUNC(sub_822F3200) {
  t_in_fruit_balance = true;
  __imp__sub_822F3200(ctx, base);
  t_in_fruit_balance = false;
}
