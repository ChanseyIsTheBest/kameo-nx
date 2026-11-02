// banjo-nx (2026-10-29): title update 3's files (title_update.h). kameo-nx (2026-11-02): Kameo's title update 2, whose
// data is one folder per language (update:\English\...), read by the update's code; no db_index.txt.
#include "renut_engine/title_update.h"

#include <cstdlib>
#include <system_error>

#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/system.h>

namespace banjo::title_update {

namespace fs = std::filesystem;

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
