// banjo-nx (2026-10-29): title update 3's files (title_update.h). kameo-nx (2026-11-02): Kameo's title update 2, whose
// data is one folder per language (update:\English\...), read by the update's code; no db_index.txt.
#include "renut_engine/title_update.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/platform.h>
#include <rex/system.h>

#if REX_PLATFORM_SWITCH
#include <switch.h>  // setGetSystemLanguage
#endif

REXCVAR_DECLARE(uint32_t, user_language);  // sdk xam_user.cpp
REXCVAR_DEFINE_STRING(language, "system", "Kameo",
                      "The game's language: system (the Switch's system language; English elsewhere, and when the game "
                      "has no text for it), english, french, german, spanish, italian, japanese, korean or tradchinese")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace banjo::title_update {

namespace fs = std::filesystem;

namespace {

// kameo-nx (2026-11-04, the user's options; banjo-nx's design): the languages title update 2 has folders for, by the console's
// language number. Spanish on this USA game is the update's "mexican" folder: the game itself turns language 5 with
// region 0xFF (XGetGameRegion) into it. A language whose file is missing, the game itself loads English for.
struct Language {
  const char *name;
  uint32_t xlanguage;
};
constexpr Language kLanguages[] = {
    {"english", 1}, {"japanese", 2}, {"german", 3},  {"french", 4},
    {"spanish", 5}, {"italian", 6},  {"korean", 7},  {"tradchinese", 8},
};

// language = "system": the Switch's system language as the name of one of the game's. The codes are the system's
// (setGetSystemLanguage: "en-US", "fr-CA", ...), so a language a newer firmware adds is just not found. Portuguese, Dutch,
// Russian, Polish and simplified Chinese have no text in the game: English.
const char *SystemLanguage() {
#if REX_PLATFORM_SWITCH
  struct Code {
    const char *code, *name;
  };
  static constexpr Code kCodes[] = {
      {"ja", "japanese"}, {"en-US", "english"},     {"en-GB", "english"},     {"fr", "french"},    {"fr-CA", "french"},
      {"de", "german"},   {"it", "italian"},        {"es", "spanish"},        {"es-419", "spanish"}, {"ko", "korean"},
      {"zh-TW", "tradchinese"}, {"zh-Hant", "tradchinese"},
  };
  u64 value = 0;
  const Result init = setInitialize();
  if (R_FAILED(init)) {
    REXLOG_WARN("[language] the system language could not be read (0x{:X}): English", init);
    return "english";
  }
  const Result got = setGetSystemLanguage(&value);
  setExit();
  if (R_FAILED(got)) {
    REXLOG_WARN("[language] the system language could not be read (0x{:X}): English", got);
    return "english";
  }
  char code[9] = {};
  std::memcpy(code, &value, 8);
  for (const Code &c : kCodes)
    if (std::strcmp(code, c.code) == 0) {
      REXLOG_INFO("[language] the Switch's system language is {}", code);
      return c.name;
    }
  REXLOG_INFO("[language] the game has no text for the Switch's system language {}: English", code);
#endif
  return "english";
}

}  // namespace

void ApplyLanguage() {
  std::string name = REXCVAR_GET(language);
  for (char &c : name)
    c = (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c;
  if (name == "system")
    name = SystemLanguage();
  for (const Language &l : kLanguages) {
    if (name == l.name) {
      REXCVAR_SET(user_language, l.xlanguage);
      REXLOG_INFO("[language] {}: the console's language {}", l.name, l.xlanguage);
      return;
    }
  }
  REXLOG_WARN("[language] language = \"{}\" is not one of the game's: the game stays in English", name);
}

void ConfigurePaths(rex::PathConfig &paths) {
  std::error_code ec;
  if (paths.update_data_root.empty()) {
    const fs::path candidates[] = {
        rex::filesystem::GetExecutableFolder() / "update",
        paths.game_data_root.empty() ? fs::path() : paths.game_data_root.parent_path() / "update",
        paths.game_data_root.empty() ? fs::path() : paths.game_data_root / "update",
    };
    for (const fs::path &c : candidates) {
      if (!c.empty() && fs::is_directory(c / "English", ec)) {
        paths.update_data_root = c;
        break;
      }
    }
  }
  if (paths.update_data_root.empty())
    REXLOG_WARN("[title update] no update folder (update_data_root, or \"update\" next to kameo-nx or the game "
                "folder, holding title update 2's language folders): the update's text is not there");
  else
    REXLOG_INFO("[title update] update data: {}", rex::path_to_utf8(paths.update_data_root));
  if (!paths.game_data_root.empty() && !fs::exists(paths.game_data_root / "default.xexp", ec)) {
    // The recompiled code is title update 2's: the base XEX's image would not match it at all.
    REXLOG_ERROR("[title update] {} has no default.xexp. kameo-nx is title update 2 of the game: put the update's "
                 "default.xexp next to default.xex (and its language folders in the update folder)",
                 rex::path_to_utf8(paths.game_data_root));
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
                              "kameo-nx needs title update 2: put the update's default.xexp next to default.xex in the "
                              "game folder, and its language folders in an \"update\" folder next to kameo-nx.");
  }
}

}  // namespace banjo::title_update
