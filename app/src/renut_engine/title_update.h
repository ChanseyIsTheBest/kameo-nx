#pragma once
// banjo-nx (2026-10-29), kameo-nx (2026-11-02): kameo-nx is recompiled from Kameo: Elements of Power with title update 2
// (v0.0.2.1, the XEX patch default.xexp applied to default.xex). The runtime applies the patch it finds next to
// default.xex (sdk/src/system/user_module.cpp, "<xex>p"), and mounts update_data_root as update:, where the update's
// code finds its string tables (update:\English\...).
//
//   game folder:   default.xex, default.xexp (the update's), and the disc's files
//   update folder: the update's language folders (update_data_root; by default "update" next to the NRO / exe, or next
//                  to the game folder)

#include <rex/rex_app.h>

namespace banjo::title_update {

// RenutApp::OnConfigurePaths: the update folder when update_data_root is not set, and the check that default.xexp is
// there (the code is the update's: without the patch the game would run the base XEX's data through it).
void ConfigurePaths(rex::PathConfig &paths);

}  // namespace banjo::title_update
