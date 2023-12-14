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

#include "constants.hpp"

#include <algorithm>
#include <string>
#include <net/net_compat.h>
#include <string/stdstring.h>
#include <retro_assert.h>
#include <file/file_path.h>
#include <streams/file_stream.h>

#include "types.hpp"
#include "environment.hpp"
#include "retro/dirent.hpp"
#include "tracy.hpp"

using std::find;
using std::optional;
using std::nullopt;
using std::string;
using namespace melonDS;

bool MelonDsDs::config::IsDsiNandImage(const retro::dirent &file) noexcept {
    ZoneScopedN(TracyFunction);
    ZoneText(file.path, strnlen(file.path, sizeof(file.path)));

    // TODO: Validate the NoCash footer
    if (!file.is_regular_file())
        return false;

    if (find(DSI_NAND_SIZES.begin(), DSI_NAND_SIZES.end(), file.size) == DSI_NAND_SIZES.end())
        return false;

    return true;
}

bool MelonDsDs::config::IsFirmwareImage(const retro::dirent& file, Firmware::FirmwareHeader& header) noexcept {
    ZoneScopedN(TracyFunction);
    ZoneText(file.path, strnlen(file.path, sizeof(file.path)));

    retro_assert(path_is_absolute(file.path));

    if (!file.is_regular_file()) {
        retro::debug("{} is not a regular file, it's not firmware", file.path);
        return false;
    }

    if (find(FIRMWARE_SIZES.begin(), FIRMWARE_SIZES.end(), file.size) == FIRMWARE_SIZES.end()) {
        retro::debug(
            "{} is not a known firmware size (found {} bytes, must be one of {})",
            file.path,
            file.size,
            fmt::join(FIRMWARE_SIZES, ", ")
        );
        return false;
    }

    if (string_ends_with(file.path, ".bak")) {
        retro::debug("{} is a backup file, not counting it as firmware", file.path);
        return false;
    }

    RFILE* stream = filestream_open(file.path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!stream)
        return false;

    char buffer[sizeof(header)];

    int64_t bytesRead = filestream_read(stream, &buffer, sizeof(buffer));
    filestream_close(stream);

    if (bytesRead < (int64_t)sizeof(buffer)) {
        if (bytesRead < 0) {
            retro::warn("Failed to read {}", file.path);
        } else {
            retro::warn("Failed to read {} (expected {} bytes, got {})", file.path, sizeof(buffer), bytesRead);
        }

        return false;
    }

    Firmware::FirmwareHeader& loadedHeader = *reinterpret_cast<Firmware::FirmwareHeader*>(&buffer);

    switch (loadedHeader.ConsoleType) {
        case Firmware::FirmwareConsoleType::DS:
        case Firmware::FirmwareConsoleType::DSi:
        case Firmware::FirmwareConsoleType::iQueDSLite:
        case Firmware::FirmwareConsoleType::iQueDS:
        case Firmware::FirmwareConsoleType::DSLite:
            break;
        default:
            retro::debug("{} doesn't look like valid firmware (unrecognized ConsoleType 0x{:02X})", file.path, (u8)loadedHeader.ConsoleType);
            return false;
    }

    if (loadedHeader.Unused0[0] != 0xFF || loadedHeader.Unused0[1] != 0xFF) {
        // Primarily used to eliminate Sega CD BIOS files (same size but these bytes are different)
        retro::debug("{} doesn't look like valid firmware (unused 2-byte region at 0x1E is 0x{:02X})", file.path, fmt::join(loadedHeader.Unused0, ""));
        return false;
    }

    memcpy(&header, &buffer, sizeof(buffer));
    return true;
}