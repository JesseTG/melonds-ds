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

#include <SPI_Firmware.h>
#include <algorithm>
#include <fmt/format.h>
#include <string>
#include <fmt/ranges.h>
#include <net/net_compat.h>
#include <string/stdstring.h>
#include <retro_assert.h>
#include <file/file_path.h>
#include <streams/file_stream.h>

#include "retro/file.hpp"
#include "types.hpp"
#include "environment.hpp"
#include "retro/dirent.hpp"
#include "tracy.hpp"

using std::find;
using std::optional;
using std::nullopt;
using std::string;
using namespace melonDS;

// We verify the filesize of the NAND image and the presence of the no$gba footer (since melonDS needs it)
bool MelonDsDs::config::IsDsiNandImage(const retro::dirent &file) noexcept {
    ZoneScopedN(TracyFunction);
    ZoneText(file.path, strnlen(file.path, sizeof(file.path)));

    if (!file.is_regular_file())
        return false;

    switch (file.size) {
        case DSI_NAND_SIZES_NOFOOTER[0] + NOCASH_FOOTER_SIZE: // 240MB + no$gba footer
        case DSI_NAND_SIZES_NOFOOTER[1] + NOCASH_FOOTER_SIZE: // 245.5MB + no$gba footer
        case DSI_NAND_SIZES_NOFOOTER[0]: // 240MB
        case DSI_NAND_SIZES_NOFOOTER[1]: // 245.5MB
            break; // the size is good, let's look for the footer!
        default:
            return false;
    }

    RFILE* stream = filestream_open(file.path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!stream)
        return false;

    if (filestream_seek(stream, -static_cast<int64_t>(NOCASH_FOOTER_SIZE), RETRO_VFS_SEEK_POSITION_END) < 0) {
        filestream_close(stream);
        return false;
    }

    std::array<uint8_t , NOCASH_FOOTER_SIZE> footer;
    if (filestream_read(stream, footer.data(), footer.size()) != NOCASH_FOOTER_SIZE) {
        filestream_close(stream);
        return false;
    }

    if (filestream_seek(stream, NOCASH_FOOTER_OFFSET, RETRO_VFS_SEEK_POSITION_START) < 0) {
        filestream_close(stream);
        return false;
    }

    std::array<uint8_t , NOCASH_FOOTER_SIZE> unusedArea;
    if (filestream_read(stream, unusedArea.data(), unusedArea.size()) != NOCASH_FOOTER_SIZE) {
        filestream_close(stream);
        return false;
    }

    filestream_close(stream);

    if (memcmp(footer.data(), NOCASH_FOOTER_MAGIC, NOCASH_FOOTER_MAGIC_SIZE) == 0) {
        // If the no$gba footer is present at the end of the file and correctly starts with the magic bytes...
        return true;
    }

    if (memcmp(unusedArea.data(), NOCASH_FOOTER_MAGIC, NOCASH_FOOTER_MAGIC_SIZE) == 0) {
        // If the no$gba footer is present in a normally-unused section of the DSi NAND, and it starts with the magic bytes...
        return true;
    }

    return false;
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

    bool isDsFirmware = loadedHeader.ConsoleType == Firmware::FirmwareConsoleType::DS || loadedHeader.ConsoleType == Firmware::FirmwareConsoleType::DSLite;
    if (isDsFirmware && strncmp(reinterpret_cast<const char*>(loadedHeader.Identifier.data()), "MAC", 3) != 0) {
        retro::debug("{} doesn't look like valid NDS firmware (unrecognized identifier {})", file.path, fmt::join(loadedHeader.Identifier, ""));
        return false;
    }
    // TODO: Validate the checksum of the userdata region

    memcpy(&header, &buffer, sizeof(buffer));
    return true;
}

// A MAC address has 6 bytes, each with two hexadecimal characters,
// and 5 colons (:) for separators
constexpr int MacAddressStringSize = 2*6 + 5;

std::optional<melonDS::MacAddress> MelonDsDs::config::ParseMacAddressFile(const retro::dirent &file) noexcept {
    ZoneScopedN(TracyFunction);
    ZoneText(file.path, strnlen(file.path, sizeof(file.path)));
    retro::debug("Reading file {}", file.path);

    if(!file.is_regular_file()) {
        retro::debug("{} is not a regular file, it's not a mac address file", file.path);
        return std::nullopt;
    }

    if(!string_ends_with(file.path, ".txt")) {
        retro::debug("{} is not a mac address file, it does not end with .txt", file.path);
        return std::nullopt;
    }
    if (file.size < MacAddressStringSize) {
        retro::debug("{} is not a mac address file, it is too small", file.path);
        return std::nullopt;
    }

    char buffer[MacAddressStringSize];
    retro::rfile_ptr stream = retro::make_rfile(file.path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

    int64_t bytesRead = filestream_read(stream.get(), &buffer, sizeof(buffer));
    if (bytesRead < MacAddressStringSize) {
        if (bytesRead < 0) {
            retro::warn("Failed to read {}", file.path);
        } else {
            retro::warn("Tried to read {} bytes, ended up reading {} bytes instead", MacAddressStringSize, bytesRead);
        }
        return std::nullopt;
    }

    std::optional<melonDS::MacAddress> ret =  MelonDsDs::config::ParseMacAddress(std::string_view{buffer, sizeof(buffer)});
    if (!ret.has_value()) {
        retro::debug("Could not read the mac address from \"{}\"", std::string_view{buffer, sizeof(buffer)});
    }
    return ret;
}

std::optional<melonDS::MacAddress> MelonDsDs::config::ParseMacAddress(std::string_view s) noexcept {
    // This would be 5 lines if scanf worked on string_view
    melonDS::MacAddress ret;
    int i = 0;
    int readOctets = 0;
    const char *data = s.data();
    if (s.size() < MacAddressStringSize) {
        return std::nullopt;
    }
    do {
        char octet[3];
        octet[0] = data[i++];
        octet[1] = data[i++];
        octet[2] = '\0';
        char *end = nullptr;
        unsigned long octetValue = std::strtoul(octet, &end, 16);
        if (end != octet + 2) {
            return std::nullopt;
        }
        if (octetValue > 255) {
            // Not sure how this would be possible
            return std::nullopt;
        }
        ret[readOctets++] = octetValue;
        if (i >= MacAddressStringSize) {
            break;
        }
        if (readOctets != 6 && data[i] != ':') {
            return std::nullopt;
        }
    } while (++i < MacAddressStringSize - 1 && readOctets < 6);
    if (readOctets != 6) {
        return std::nullopt;
    }
    return ret;
}

std::string MelonDsDs::config::PrintMacAddress(const melonDS::MacAddress &address) noexcept {
    return fmt::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", address[0], address[1], address[2], address[3], address[4], address[5]);
}
