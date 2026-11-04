// banjo-nx (2026-10-29): reNut upstream's DLC installer (ab60251, src/renut_engine/app/dlc_install.cpp; GPL-3.0, credit reNut),
// the DLC folder next to the NRO / exe (sdmc:/switch/banjo-nx/dlc on the Switch). katamari-nx (2026-10-31): the same
// file, game-agnostic (the running title's packages only): sdmc:/switch/kameo-nx/dlc.
#include "renut_engine/dlc_install.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_manager.h>
#include <rex/system/xam/user_profile.h>
#include <rex/system/xmemory.h>


#include "renut_engine/renut_logging.h"

REXCVAR_DEFINE_STRING(renut_dlc_folder, "dlc", "Nuts&Bolts/DLC",
    "Folder of DLC packages (Xbox Live content files) reNut installs when the game starts, relative to "
    "reNut's folder.");
REXCVAR_DEFINE_BOOL(kameo_dlc_no_rescan, true, "Kameo",
    "The game knows from the start that the player is signed in, so the sign-in notice at startup does not make its "
    "DLC scan start over (SeedSignInState). Off brings back closing and remounting the packs at boot")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);
namespace renut::dlc {

namespace {

namespace fs = std::filesystem;

// An Xbox 360 package's header (big-endian): the magic, then at 0x344 the
// content type, 0x360 the title ID and 0x411 the display name (UTF-16).
constexpr size_t kHeaderBytes = 0x500;
constexpr size_t kContentTypeOffset = 0x344;
constexpr size_t kTitleIdOffset = 0x360;
constexpr size_t kDisplayNameOffset = 0x411;
constexpr size_t kDisplayNameChars = 64;
constexpr uint32_t kContentTypeMarketplace = 2;
// Where an installed package's header goes in the user data folder: the
// default profile's content, by title and content type.
constexpr const char kProfileFolder[] = "0000000000000000";
constexpr const char kContentTypeFolder[] = "00000002";
// Packages installed before, by file name and size, so an unchanged one is
// not installed again when the user data folder isn't known.
constexpr const char kInstalledList[] = "installed.txt";

fs::path g_user_data_root;

uint32_t LoadBe32(const uint8_t *p) {
    return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

struct Package {
    uint32_t content_type = 0;
    uint32_t title_id = 0;
    std::string name;  // display name (ASCII part)
};

// The package's header, or nothing if the file isn't an Xbox 360 package.
bool ReadPackage(const fs::path &path, Package &out) {
    std::ifstream f(path, std::ios::binary);
    std::array<uint8_t, kHeaderBytes> h{};
    if (!f.read(reinterpret_cast<char *>(h.data()), h.size()))
        return false;
    const std::string magic(reinterpret_cast<const char *>(h.data()), 4);
    if (magic != "LIVE" && magic != "PIRS" && magic != "CON ")
        return false;
    out.content_type = LoadBe32(h.data() + kContentTypeOffset);
    out.title_id = LoadBe32(h.data() + kTitleIdOffset);
    out.name.clear();
    for (size_t i = 0; i < kDisplayNameChars; ++i) {
        const uint8_t *c = h.data() + kDisplayNameOffset + 2 * i;
        const uint16_t ch = uint16_t(c[0] << 8 | c[1]);
        if (!ch)
            break;
        out.name.push_back(ch < 0x80 ? char(ch) : '?');
    }
    return true;
}

std::string HexTitle(uint32_t title_id) {
    char s[9];
    std::snprintf(s, sizeof(s), "%08X", title_id);
    return s;
}

// The header the install writes for 'file_name', if the user data folder is known.
fs::path InstalledHeader(uint32_t title_id, const std::string &file_name) {
    if (g_user_data_root.empty())
        return {};
    return g_user_data_root / kProfileFolder / HexTitle(title_id) / "Headers" / kContentTypeFolder /
           (file_name + ".header");
}

std::vector<std::string> ReadInstalledList(const fs::path &list) {
    std::vector<std::string> lines;
    std::ifstream f(list);
    for (std::string line; std::getline(f, line);)
        if (!line.empty())
            lines.push_back(line);
    return lines;
}

std::string ListEntry(const std::string &file_name, uintmax_t size) {
    return file_name + " " + std::to_string(size);
}

} // namespace

void SetUserDataRoot(const fs::path &root) {
    g_user_data_root = root;
}

void InstallPackages(rex::Runtime *runtime) {
    if (!runtime || !runtime->kernel_state())
        return;
    // kameo-nx (2026-11-02): several folders, separated by ';' (each of Kameo's packs came in its own: the PC points at
    // them as they are). installed.txt stays in the first.
    std::vector<fs::path> folders;
    {
        const std::string text = REXCVAR_GET(renut_dlc_folder);
        for (size_t at = 0; at < text.size();) {
            size_t end = text.find(';', at);
            if (end == std::string::npos)
                end = text.size();
            if (end > at) {
                fs::path one = rex::to_path(text.substr(at, end - at));
                if (one.is_relative())
                    one = rex::filesystem::GetExecutableFolder() / one;  // banjo-nx: next to the NRO / exe
                folders.push_back(std::move(one));
            }
            at = end + 1;
        }
    }
    if (folders.empty())
        return;
    const fs::path &folder = folders.front();
    std::error_code ec;
    fs::create_directories(folder, ec);

    std::vector<fs::path> files;
    for (const fs::path &one : folders)
        for (const auto &entry : fs::directory_iterator(one, ec))
            if (entry.is_regular_file(ec) && entry.path().filename() != kInstalledList)
                files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    if (files.empty())
        return;

    const uint32_t title_id = runtime->kernel_state()->title_id();
    const fs::path list_path = folder / kInstalledList;
    std::vector<std::string> installed = ReadInstalledList(list_path);
    bool list_changed = false;
    int newly = 0, present = 0;

    for (const fs::path &path : files) {
        const std::string file_name = rex::path_to_utf8(path.filename());
        Package package;
        if (!ReadPackage(path, package)) {
            RNUT_WARN("DLC: {} is not an Xbox 360 package; left alone", file_name);
            continue;
        }
        if (package.title_id != title_id) {
            RNUT_WARN("DLC: {} ({}) is for another game (title {:08X}); left alone", file_name, package.name,
                      package.title_id);
            continue;
        }
        if (package.content_type != kContentTypeMarketplace) {
            RNUT_WARN("DLC: {} ({}) is not DLC (content type {:X}); left alone", file_name, package.name,
                      package.content_type);
            continue;
        }
        const uintmax_t size = fs::file_size(path, ec);
        const std::string entry = ListEntry(file_name, size);
        const fs::path header = InstalledHeader(title_id, file_name);
        const bool listed = std::find(installed.begin(), installed.end(), entry) != installed.end();
        const bool header_there = !header.empty() && fs::exists(header, ec);
        if (header_there || (header.empty() && listed)) {
            ++present;
            continue;
        }

        RNUT_INFO("DLC: installing {} ({})...", package.name, file_name);
        const std::string utf8 = rex::path_to_utf8(path);
        const auto result = runtime->kernel_state()->content_manager()->InstallContent(utf8.c_str());
        if (result != 0) {
            RNUT_ERROR("DLC: {} ({}) did not install (error {:08X})", package.name, file_name, unsigned(result));
            continue;
        }
        RNUT_INFO("DLC: {} installed", package.name);
        ++newly;
        if (!listed) {
            installed.push_back(entry);
            list_changed = true;
        }
    }

    if (list_changed) {
        std::ofstream f(list_path, std::ios::trunc);
        for (const std::string &line : installed)
            f << line << "\n";
    }
    if (newly || present)
        RNUT_INFO("DLC: {} package(s) installed now, {} already installed", newly, present);
}

/*
 * Why Kameo closed and remounted its DLC packs at every boot, and what that broke.
 *
 * sub_820B0878 polls a notification listener that the DLC scan creates. A sign-in notice counts as a
 * change only if its word (0x82BCFE94) says nobody was signed in, and the word starts at 0. Our kernel
 * sends every title's first listener XN_SYS_SIGNINCHANGED at startup (KernelState::RegisterNotifyListener),
 * so the notice always counted, during the scan. When the scan ended, sub_820B0D70 saw it and started
 * over: close the packs, unload the DLC audio (sub_82315AF0), scan again.
 *
 * That unload only removes the cues of banks that have finished loading. Those from a bank still
 * loading stay in the table, so the second scan adds them again, the insert in sub_82313330 fails, and
 * the game calls that a bad disc. The file thread also still had requests for the closed packs: the
 * failed open of dlc2:\audio\M_DLCsfx.xwb on every console boot, and possibly the damaged heap of the
 * crash at the Microsoft logo. Other ports don't hit this: their games don't restart a scan on this notice.
 *
 * So the game is told at the start that the player is signed in, and the notice is no change. The packs
 * then stay mounted from the first scan. Written before the guest starts.
 */
void SeedSignInState(rex::Runtime *runtime) {
    if (!REXCVAR_GET(kameo_dlc_no_rescan))
        return;
    constexpr uint32_t kDlcListenerSignIn = 0x82BCFE94;
    auto *kernel = runtime->kernel_state();
    const uint32_t state = kernel->user_profile()->signin_state();
    rex::memory::store_and_swap<uint32_t>(kernel->memory()->TranslateVirtual(kDlcListenerSignIn), state);
    RNUT_INFO("DLC: sign-in state {} known from the start, so the scan does not restart (kameo_dlc_no_rescan)", state);
}

} // namespace renut::dlc
