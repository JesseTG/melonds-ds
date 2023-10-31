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

#include "environment.hpp"
#include "retro/dirent.hpp"
#include "tracy.hpp"

using std::find;
using std::optional;
using std::nullopt;
using std::string;

optional<melonds::Renderer> melonds::config::ParseRenderer(const char* value) noexcept {
    if (string_is_equal(value, values::SOFTWARE)) return melonds::Renderer::Software;
    if (string_is_equal(value, values::OPENGL)) return melonds::Renderer::OpenGl;
    return nullopt;
}

optional<melonds::CursorMode> melonds::config::ParseCursorMode(const char* value) noexcept {
    if (string_is_equal(value, values::DISABLED)) return melonds::CursorMode::Never;
    if (string_is_equal(value, values::TOUCHING)) return melonds::CursorMode::Touching;
    if (string_is_equal(value, values::TIMEOUT)) return melonds::CursorMode::Timeout;
    if (string_is_equal(value, values::ALWAYS)) return melonds::CursorMode::Always;
    return nullopt;
}

optional<melonds::ConsoleType> melonds::config::ParseConsoleType(const char* value) noexcept {
    if (string_is_equal(value, values::DS)) return melonds::ConsoleType::DS;
    if (string_is_equal(value, values::DSI)) return melonds::ConsoleType::DSi;
    return nullopt;
}

optional<melonds::NetworkMode> melonds::config::ParseNetworkMode(const char* value) noexcept {
    if (string_is_equal(value, values::DISABLED)) return melonds::NetworkMode::None;
    if (string_is_equal(value, values::DIRECT)) return melonds::NetworkMode::Direct;
    if (string_is_equal(value, values::INDIRECT)) return melonds::NetworkMode::Indirect;
    return nullopt;
}

optional<bool> melonds::config::ParseBoolean(const char* value) noexcept {
    ZoneScopedN("melonds::config::ParseBoolean");
    if (string_is_equal(value, values::ENABLED)) return true;
    if (string_is_equal(value, values::DISABLED)) return false;
    return nullopt;
}

optional<melonds::BootMode> melonds::config::ParseBootMode(const char *value) noexcept {
    if (string_is_equal(value, values::NATIVE)) return BootMode::Native;
    if (string_is_equal(value, values::DIRECT)) return BootMode::Direct;
    return nullopt;
}

optional<melonds::SysfileMode> melonds::config::ParseSysfileMode(const char *value) noexcept {
    if (string_is_equal(value, values::NATIVE)) return SysfileMode::Native;
    if (string_is_equal(value, values::BUILT_IN)) return SysfileMode::BuiltIn;
    return nullopt;
}

optional<melonds::AlarmMode> melonds::config::ParseAlarmMode(const char* value) noexcept {
    if (string_is_equal(value, values::DISABLED)) return AlarmMode::Disabled;
    if (string_is_equal(value, values::ENABLED)) return AlarmMode::Enabled;
    if (string_is_equal(value, values::DEFAULT)) return AlarmMode::Default;
    return nullopt;
}

optional<melonds::UsernameMode> melonds::config::ParseUsernameMode(const char* value) noexcept {
    if (string_is_empty(value) || string_is_equal(value, values::firmware::DEFAULT_USERNAME)) return UsernameMode::MelonDSDS;
    if (string_is_equal(value, values::firmware::FIRMWARE_USERNAME)) return UsernameMode::Firmware;
    if (string_is_equal(value, values::firmware::GUESS_USERNAME)) return UsernameMode::Guess;
    return nullopt;
}

string melonds::config::GetUsername(melonds::UsernameMode mode) noexcept {
    ZoneScopedN("melonds::config::GetUsername");
    char result[DS_NAME_LIMIT + 1];
    result[DS_NAME_LIMIT] = '\0';

    switch (mode) {
        case melonds::UsernameMode::Firmware:
            return values::firmware::FIRMWARE_USERNAME;
        case melonds::UsernameMode::Guess: {
            if (optional<string> frontendGuess = retro::username(); frontendGuess && !frontendGuess->empty()) {
                return *frontendGuess;
            } else if (const char* user = getenv("USER"); !string_is_empty(user)) {
                strncpy(result, user, DS_NAME_LIMIT);
            } else if (const char* username = getenv("USERNAME"); !string_is_empty(username)) {
                strncpy(result, username, DS_NAME_LIMIT);
            } else if (const char* logname = getenv("LOGNAME"); !string_is_empty(logname)) {
                strncpy(result, logname, DS_NAME_LIMIT);
            } else {
                strncpy(result, values::firmware::DEFAULT_USERNAME, DS_NAME_LIMIT);
            }

            return result;
        }
        case melonds::UsernameMode::MelonDSDS:
        default:
            return values::firmware::DEFAULT_USERNAME;
    }


}

optional<melonds::ScreenLayout> melonds::config::ParseScreenLayout(const char* value) noexcept {
    ZoneScopedN("melonds::config::ParseScreenLayout");
    using melonds::ScreenLayout;
    if (string_is_equal(value, values::TOP_BOTTOM)) return ScreenLayout::TopBottom;
    if (string_is_equal(value, values::BOTTOM_TOP)) return ScreenLayout::BottomTop;
    if (string_is_equal(value, values::LEFT_RIGHT)) return ScreenLayout::LeftRight;
    if (string_is_equal(value, values::RIGHT_LEFT)) return ScreenLayout::RightLeft;
    if (string_is_equal(value, values::TOP)) return ScreenLayout::TopOnly;
    if (string_is_equal(value, values::BOTTOM)) return ScreenLayout::BottomOnly;
    if (string_is_equal(value, values::HYBRID_TOP)) return ScreenLayout::HybridTop;
    if (string_is_equal(value, values::HYBRID_BOTTOM)) return ScreenLayout::HybridBottom;
    if (string_is_equal(value, values::ROTATE_LEFT)) return ScreenLayout::TurnLeft;
    if (string_is_equal(value, values::ROTATE_RIGHT)) return ScreenLayout::TurnRight;
    if (string_is_equal(value, values::UPSIDE_DOWN)) return ScreenLayout::UpsideDown;

    return nullopt;
}

optional<melonds::HybridSideScreenDisplay> melonds::config::ParseHybridSideScreenDisplay(const char* value) noexcept {
    using melonds::ScreenLayout;
    if (string_is_equal(value, values::ONE)) return melonds::HybridSideScreenDisplay::One;
    if (string_is_equal(value, values::BOTH)) return melonds::HybridSideScreenDisplay::Both;

    return nullopt;
}

std::optional<melonds::FirmwareLanguage> melonds::config::ParseLanguage(const char* value) noexcept {
    if (string_is_equal(value, values::AUTO)) return melonds::FirmwareLanguage::Auto;
    if (string_is_equal(value, values::DEFAULT)) return melonds::FirmwareLanguage::Default;
    if (string_is_equal(value, values::JAPANESE)) return melonds::FirmwareLanguage::Japanese;
    if (string_is_equal(value, values::ENGLISH)) return melonds::FirmwareLanguage::English;
    if (string_is_equal(value, values::FRENCH)) return melonds::FirmwareLanguage::French;
    if (string_is_equal(value, values::GERMAN)) return melonds::FirmwareLanguage::German;
    if (string_is_equal(value, values::ITALIAN)) return melonds::FirmwareLanguage::Italian;
    if (string_is_equal(value, values::SPANISH)) return melonds::FirmwareLanguage::Spanish;

    return nullopt;
}

optional<melonds::MicInputMode> melonds::config::ParseMicInputMode(const char* value) noexcept {
    if (string_is_equal(value, values::MICROPHONE)) return MicInputMode::HostMic;
    if (string_is_equal(value, values::BLOW)) return MicInputMode::BlowNoise;
    if (string_is_equal(value, values::NOISE)) return MicInputMode::WhiteNoise;
    if (string_is_equal(value, values::SILENCE)) return MicInputMode::None;

    return nullopt;
}

std::optional<melonds::TouchMode> melonds::config::ParseTouchMode(const char* value) noexcept {
    if (string_is_equal(value, values::AUTO)) return TouchMode::Auto;
    if (string_is_equal(value, values::TOUCH)) return TouchMode::Pointer;
    if (string_is_equal(value, values::JOYSTICK)) return TouchMode::Joystick;

    return nullopt;
}

optional<SPI_Firmware::IpAddress> melonds::config::ParseIpAddress(const char* value) noexcept {
    ZoneScopedN("melonds::config::ParseIpAddress");
    if (string_is_empty(value))
        return nullopt;

    in_addr address;
    if (inet_pton(AF_INET, value, &address) == 1) {
        // Both in_addr and ip represent an IPv4 address,
        // but they may have different alignment requirements.
        // Better safe than sorry.
        SPI_Firmware::IpAddress ip;
        memcpy(&ip, &address, sizeof(address));
        return ip;
    }

    return nullopt;
}

bool melonds::config::IsDsiNandImage(const retro::dirent &file) noexcept {
    ZoneScopedN("melonds::config::IsDsiNandImage");
    ZoneText(file.path, strnlen(file.path, sizeof(file.path)));

    // TODO: Validate the NoCash footer
    if (!file.is_regular_file())
        return false;

    if (find(DSI_NAND_SIZES.begin(), DSI_NAND_SIZES.end(), file.size) == DSI_NAND_SIZES.end())
        return false;

    return true;
}

bool melonds::config::IsFirmwareImage(const retro::dirent& file, SPI_Firmware::FirmwareHeader& header) noexcept {
    ZoneScopedN("melonds::config::IsFirmwareImage");
    ZoneText(file.path, strnlen(file.path, sizeof(file.path)));

    retro_assert(path_is_absolute(file.path));

    if (!file.is_regular_file())
        return false;

    if (find(FIRMWARE_SIZES.begin(), FIRMWARE_SIZES.end(), file.size) == FIRMWARE_SIZES.end())
        return false;

    if (string_ends_with(file.path, ".bak"))
        return false;

    RFILE* stream = filestream_open(file.path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!stream)
        return false;

    char buffer[sizeof(header)];

    int64_t bytesRead = filestream_read(stream, &buffer, sizeof(buffer));
    filestream_close(stream);

    if (bytesRead < (int64_t)sizeof(buffer))
        return false;

    SPI_Firmware::FirmwareHeader& loadedHeader = *reinterpret_cast<SPI_Firmware::FirmwareHeader*>(&buffer);

    switch (loadedHeader.ConsoleType) {
        case SPI_Firmware::FirmwareConsoleType::DS:
        case SPI_Firmware::FirmwareConsoleType::DSi:
        case SPI_Firmware::FirmwareConsoleType::iQueDSLite:
        case SPI_Firmware::FirmwareConsoleType::iQueDS:
        case SPI_Firmware::FirmwareConsoleType::DSLite:
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