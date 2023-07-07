/*
    Copyright 2023 Jesse Talavera-Greenberg

    melonDS DS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS DS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS DS. If not, see http://www.gnu.org/licenses/.
*/

#include "dsi.hpp"

#include <optional>
#include <string>

#ifdef HAVE_NETWORKING
#include <net/net_http.h>
#endif

#include <DSi_NAND.h>
#include <DSi_TMD.h>
#include <retro_assert.h>
#include <file/file_path.h>
#include <compat/strl.h>
#include <streams/file_stream.h>
#include <retro_timers.h>
#include <DSi.h>

#include "environment.hpp"
#include "exceptions.hpp"

using std::nullopt;
using std::optional;
using std::string;

using retro::info;
using retro::warn;
using retro::error;

using DSi_TMD::TitleMetadata;

const char *TMD_DIR_NAME = "tmd";
constexpr u32 RSA256_SIGNATURE_TYPE = 16777472;
namespace melonds::dsi {
    static bool _was_dsiware_title_installed;

    static void get_tmd_path(const retro_game_info &nds_info, char *buffer, size_t buffer_length);

    bool get_cached_tmd(const char *tmd_path, TitleMetadata &tmd) noexcept;

    bool validate_tmd(const TitleMetadata &tmd) noexcept;

#ifdef HAVE_NETWORKING
    bool download_tmd(const NDSHeader &header, TitleMetadata &tmd) noexcept;
#endif

    void cache_tmd(const char *tmd_path, const TitleMetadata &tmd) noexcept;

    void import_savedata(const retro_game_info &nds_info, const NDSHeader& header, int type) noexcept;

    void export_savedata(const retro_game_info &nds_info, const NDSHeader& header, int type) noexcept;
}

void melonds::dsi::install_dsiware(const retro_game_info &nds_info, const NdsCart &cart) try {
    info("Temporarily installing DSiWare title \"%s\" onto DSi NAND image", nds_info.path);
    const NDSHeader &header = cart.GetHeader();
    retro_assert(header.IsDSiWare());
    retro_assert(DSi_NAND::GetFile() == nullptr);

    if (!DSi_NAND::Init(&DSi::ARM7iBIOS[0x8308])) {
        throw emulator_exception("Failed to open DSi NAND for installation");
    }

    if (DSi_NAND::TitleExists(header.DSiTitleIDHigh, header.DSiTitleIDLow)) {
        retro::info("Title already exists on loaded NAND; skipping installation, and won't uninstall it later.");
        _was_dsiware_title_installed = true;
        // TODO: Allow player to forcibly install the title anyway

        // TODO: Import Game.public.sav if it exists, unless the internal save file is newer
        // TODO: Import Game.private.sav if it exists, unless the internal save file is newer
        // TODO: Import Game.banner.sav if it exists, unless the internal save file is newer
    } else {
        retro::info("Title is not on loaded NAND; will install it for the duration of this session.");

        char tmd_path[PATH_MAX];
        get_tmd_path(nds_info, tmd_path, sizeof(tmd_path));

        TitleMetadata tmd{};

        if (!get_cached_tmd(tmd_path, tmd)) {
            // If the TMD isn't available locally...

#ifdef HAVE_NETWORKING
            if (download_tmd(header, tmd)) {
                // ...then download it. If that worked...
                cache_tmd(tmd_path, tmd);
            } else {
                throw missing_metadata_exception("Cannot get title metadata for installation");
            }
#else
            throw missing_metadata_exception("Cannot get title metadata for installation, and this build does not support downloading it");
#endif
        }

        if (!DSi_NAND::ImportTitle(static_cast<const u8 *>(nds_info.data), nds_info.size, tmd, false)) {
            throw emulator_exception("Failed to import DSiWare title into NAND image");
        }

        import_savedata(nds_info, header, DSi_NAND::TitleData_PublicSav);
        import_savedata(nds_info, header, DSi_NAND::TitleData_PrivateSav);
        import_savedata(nds_info, header, DSi_NAND::TitleData_BannerSav);
    }

    DSi_NAND::DeInit();
}
catch (...) {
    DSi_NAND::DeInit();
    throw;
}

static void melonds::dsi::get_tmd_path(const retro_game_info &nds_info, char *buffer, size_t buffer_length) {
    char tmd_name[PATH_MAX]; // "/path/to/game.zip#game.nds"
    memset(tmd_name, 0, sizeof(tmd_name));
    const char *ptr = path_basename(nds_info.path);  // "game.nds"
    strlcpy(tmd_name, ptr ? ptr : nds_info.path, sizeof(tmd_name));
    path_remove_extension(tmd_name); // "game"
    strlcat(tmd_name, ".tmd", sizeof(tmd_name)); // "game.tmd"

    const optional<string> &system_subdir = retro::get_system_subdirectory();
    if (!system_subdir) {
        throw std::runtime_error("System directory not set");
    }

    char tmd_dir[PATH_MAX];
    memset(tmd_dir, 0, sizeof(tmd_dir));
    fill_pathname_join_special(tmd_dir, system_subdir->c_str(), TMD_DIR_NAME, sizeof(tmd_dir));
    // "/libretro/system/melonDS DS/tmd"

    memset(buffer, 0, buffer_length);
    fill_pathname_join_special(buffer, tmd_dir, tmd_name, buffer_length);
    // "/libretro/system/melonDS DS/tmd/game.tmd"
}

bool melonds::dsi::get_cached_tmd(const char *tmd_path, TitleMetadata &tmd) noexcept {
    RFILE *tmd_file = filestream_open(tmd_path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!tmd_file) {
        info("Could not find local copy of title metadata at \"%s\"", tmd_path);
        return false;
    }

    info("Found title metadata at \"%s\"", tmd_path);
    int64_t bytes_read = filestream_read(tmd_file, &tmd, sizeof(TitleMetadata));
    filestream_close(tmd_file); // Not null, so it always succeeds

    if (bytes_read < 0) {
        // If there was an error reading the file...
        error("Error reading title metadata");
        return false;
    }

    if (static_cast<size_t>(bytes_read) < sizeof(TitleMetadata)) {
        // If the file was too small...
        error("Title metadata file is too small, it may be corrupt");
        return false;
    }

    if (!validate_tmd(tmd)) {
        // If the file is corrupt...
        error("Title metadata validation failed; the file is corrupt");
        return false;
    }

    info("Title metadata OK");

    return true;
}


bool melonds::dsi::validate_tmd(const TitleMetadata &tmd) noexcept {
    if (tmd.SignatureType != RSA256_SIGNATURE_TYPE) {
        error("Invalid signature type %08x", tmd.SignatureType);
        return false;
    }

    return true;
}

#ifdef HAVE_NETWORKING
bool melonds::dsi::download_tmd(const NDSHeader &header, TitleMetadata &tmd) noexcept {
    char url[256];
    snprintf(url, sizeof(url), "http://nus.cdn.t.shop.nintendowifi.net/ccs/download/%08x%08x/tmd",
             header.DSiTitleIDHigh, header.DSiTitleIDLow);
    // The URL comes from here https://problemkaputt.de/gbatek.htm#dsisdmmcdsiwarefilesfromnintendosserver

    info("Downloading title metadata from \"%s\"", url);

    // Create the HTTP request
    struct http_connection_t *connection = net_http_connection_new(url, "GET", nullptr);
    if (!connection) {
        error("Failed to create HTTP connection");
        return false;
    }

    // Parse the URL (always succeeds since connection is not NULL)
    net_http_connection_iterate(connection);

    // Variable declarations; we use goto for error handling here,
    // but goto can't skip variable initializations
    bool ok = false;
    size_t progress = 0;
    size_t total = 0;
    size_t payload_length = 0;
    void *payload = nullptr;
    struct http_t *http = nullptr;

    // Signify that we're ready to send the request
    if (!net_http_connection_done(connection)) {
        // If initializing the connection failed...
        error("Failed to initialize HTTP connection");
        goto done;
    }

    // And send it
    http = net_http_new(connection);
    if (!http) {
        error("Failed to open HTTP connection");
        goto done;
    }

    while (!net_http_update(http, &progress, &total)) {
        retro_sleep(20);
        // TODO: Use select with a timeout instead of a busy loop
    }

    if (net_http_error(http)) {
        // If there was a problem...
        int status = net_http_status(http);
        if (status > 0) {
            // ...but we did manage to get a status code...
            retro::error("HTTP request failed with %d", net_http_status(http));
        } else {
            retro::error("HTTP request failed with unknown error");
        }
        goto done;
    }

    // If the request succeeded, get the payload
    payload = net_http_data(http, &payload_length, false);
    if (payload == nullptr || payload_length == 0) {
        // If there was no payload...
        retro::error("HTTP request succeeded, but it sent no data");
        goto done;
    }

    if (payload_length < sizeof(TitleMetadata)) {
        // Or if the payload was too small...
        retro::error("Expected a payload of at least %u bytes, got %u bytes", sizeof(TitleMetadata), payload_length);
        goto done;
    }

    // It's okay if the payload is too big; we don't need the entire TMD
    retro::info("HTTP request succeeded with %u bytes", payload_length);
    memcpy(&tmd, payload, sizeof(TitleMetadata));

    if (!validate_tmd(tmd)) {
        // If the TMD isn't what we expected...
        retro::error("Title metadata validation failed; the server sent invalid data");
        goto done;
    }

    retro::info("Downloaded TMD successfully");
    ok = true;

done:
    net_http_delete(http);
    net_http_connection_free(connection);

    return ok;
}
#endif

// TODO: Cache the whole file, not just the TitleMetadata object
void melonds::dsi::cache_tmd(const char *tmd_path, const TitleMetadata &tmd) noexcept {
    char tmd_dir[PATH_MAX];
    strlcpy(tmd_dir, tmd_path, sizeof(tmd_dir));
    path_basedir(tmd_dir);

    if (!path_mkdir(tmd_dir)) {
        error("Error creating TMD directory \"%s\"", tmd_dir);
        return;
    }

    if (filestream_write_file(tmd_path, &tmd, sizeof(TitleMetadata))) {
        info("Cached title metadata to \"%s\"", tmd_path);
    } else {
        error("Error writing title metadata to \"%s\"", tmd_path);
    }
}

bool get_savedata_path(char *buffer, size_t length, const retro_game_info &nds_info, int type) noexcept {
    if (!buffer || !length) {
        return false;
    }

    const optional<string> &save_directory = retro::get_save_directory();
    if (!save_directory) {
        error("Save directory not available, cannot import DSiWare save data");
        return false;
    }

    char sav_name[PATH_MAX]; // "/path/to/game.zip#game.nds"
    memset(sav_name, 0, sizeof(sav_name));
    const char *ptr = path_basename(nds_info.path);  // "game.nds"
    strlcpy(sav_name, ptr ? ptr : nds_info.path, sizeof(sav_name));
    path_remove_extension(sav_name); // "game"
    switch (type) {
        case DSi_NAND::TitleData_PublicSav:
            strlcat(sav_name, ".public.sav", sizeof(sav_name)); // "game.public.sav"
            break;
        case DSi_NAND::TitleData_PrivateSav:
            strlcat(sav_name, ".private.sav", sizeof(sav_name)); // "game.private.sav"
            break;
        case DSi_NAND::TitleData_BannerSav:
            strlcat(sav_name, ".banner.sav", sizeof(sav_name)); // "game.banner.sav"
            break;
        default:
            error("Unknown save type %d", type);
            return false;
    }

    fill_pathname_join_special(buffer, save_directory->c_str(), sav_name, length);
    // "/path/to/saves/game.public.sav"
    return true;
}

void melonds::dsi::import_savedata(const retro_game_info &nds_info, const NDSHeader& header, int type) noexcept {
    retro_assert(DSi_NAND::GetFile() != nullptr);

    if (type == DSi_NAND::TitleData_PublicSav && header.DSiPublicSavSize == 0) {
        // If there's no public save data...
        info("Game does not use public save data");
        return;
    } else if (type == DSi_NAND::TitleData_PrivateSav && header.DSiPrivateSavSize == 0) {
        // If this game doesn't use private save data...
        info("Game does not use private save data");
        return;
    }
    else if (type == DSi_NAND::TitleData_BannerSav && !(header.AppFlags & 0x4)) {
        // If there's no banner save data...
        info("Game does not use banner save data");
        return;
    }

    char sav_file[PATH_MAX]; // "/path/to/game.zip#game.nds"
    if (!get_savedata_path(sav_file, sizeof(sav_file), nds_info, type)) {
        return;
    }

    if (path_stat(sav_file) != RETRO_VFS_STAT_IS_VALID) {
        // If this path is not a valid file...
        info("No DSiWare save data found at \"%s\"", sav_file);
    } else if (DSi_NAND::ImportTitleData(header.DSiTitleIDHigh, header.DSiTitleIDLow, type, sav_file)) {
        info("Imported DSiWare save data from \"%s\"", sav_file);
    } else {
        warn("Couldn't import DSiWare save data from \"%s\"", sav_file);
    }
}


void melonds::dsi::export_savedata(const retro_game_info &nds_info, const NDSHeader& header, int type) noexcept {
    retro_assert(DSi_NAND::GetFile() != nullptr);

    if (type == DSi_NAND::TitleData_PublicSav && header.DSiPublicSavSize == 0) {
        // If there's no public save data...
        info("Game does not use public save data");
        return;
    } else if (type == DSi_NAND::TitleData_PrivateSav && header.DSiPrivateSavSize == 0) {
        // If this game doesn't use private save data...
        info("Game does not use private save data");
        return;
    } else if (type == DSi_NAND::TitleData_BannerSav && !(header.AppFlags & 0x4)) {
        // If there's no banner save data...
        info("Game does not use banner save data");
        return;
    }

    char sav_file[PATH_MAX]; // "/path/to/game.zip#game.nds"
    if (!get_savedata_path(sav_file, sizeof(sav_file), nds_info, type)) {
        return;
    }

    if (DSi_NAND::ExportTitleData(header.DSiTitleIDHigh, header.DSiTitleIDLow, type, sav_file)) {
        info("Exported DSiWare save data to \"%s\"", sav_file);
    } else {
        warn("Couldn't export DSiWare save data to \"%s\"", sav_file);
    }
}

// Reset for the next time
void melonds::dsi::uninstall_dsiware(const retro_game_info &nds_info, const NdsCart &cart) noexcept {
    info("Removing temporarily-installed DSiWare title \"%s\" from NAND image", nds_info.path);
    const NDSHeader &header = cart.GetHeader();
    retro_assert(header.IsDSiWare());
    retro_assert(DSi_NAND::GetFile() == nullptr);

    if (!DSi_NAND::Init(&DSi::ARM7iBIOS[0x8308])) {
        retro::error("Failed to open DSi NAND for uninstallation");
    } else {
        // TODO: Report an error if the title doesn't exist
        export_savedata(nds_info, header, DSi_NAND::TitleData_PublicSav);
        export_savedata(nds_info, header, DSi_NAND::TitleData_PrivateSav);
        export_savedata(nds_info, header, DSi_NAND::TitleData_BannerSav);

        DSi_NAND::DeleteTitle(header.DSiTitleIDHigh, header.DSiTitleIDLow);
        info("Removed temporarily-installed DSiWare title \"%s\" from NAND image", nds_info.path);
    }

    DSi_NAND::DeInit();
    _was_dsiware_title_installed = false;
}