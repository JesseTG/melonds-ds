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


#ifndef MELONDS_DS_CONSTANTS_HPP
#define MELONDS_DS_CONSTANTS_HPP

#include <array>
#include <charconv>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <system_error>

#include <libretro.h>

#include "../config.hpp"

namespace retro {
    struct dirent;
}

namespace melonds::config {
    constexpr unsigned DS_NAME_LIMIT = 10;

    namespace audio {
        static constexpr const char *const CATEGORY = "audio";
        static constexpr const char *const AUDIO_BITDEPTH = "melonds_audio_bitdepth";
        static constexpr const char *const AUDIO_INTERPOLATION = "melonds_audio_interpolation";
        static constexpr const char *const MIC_INPUT = "melonds_mic_input";
        static constexpr const char *const MIC_INPUT_BUTTON = "melonds_mic_input_active";
    }

    namespace cpu {
        static constexpr const char* const CATEGORY = "cpu";
        static constexpr const char *const JIT_BLOCK_SIZE = "melonds_jit_block_size";
        static constexpr const char *const JIT_BRANCH_OPTIMISATIONS = "melonds_jit_branch_optimisations";
        static constexpr const char *const JIT_ENABLE = "melonds_jit_enable";
        static constexpr const char *const JIT_FAST_MEMORY = "melonds_jit_fast_memory";
        static constexpr const char *const JIT_LITERAL_OPTIMISATIONS = "melonds_jit_literal_optimisations";
    }

    namespace firmware {
        static constexpr const char *const CATEGORY = "firmware";
        static constexpr const char *const ALARM_HOUR = "melonds_firmware_alarm_hour";
        static constexpr const char *const ALARM_MINUTE = "melonds_firmware_alarm_minute";
        static constexpr const char *const BIRTH_MONTH = "melonds_firmware_birth_month";
        static constexpr const char *const BIRTH_DAY = "melonds_firmware_birth_day";
        static constexpr const char *const ENABLE_ALARM = "melonds_firmware_enable_alarm";
        static constexpr const char *const FAVORITE_COLOR = "melonds_firmware_favorite_color";
        static constexpr const char *const LANGUAGE = "melonds_firmware_language";
        static constexpr const char *const USERNAME = "melonds_firmware_username";
        static constexpr const char *const WFC_DNS = "melonds_firmware_wfc_dns";
    }

    namespace network {
        static constexpr const char *const CATEGORY = "network";
        static constexpr const char *const NETWORK_MODE = "melonds_network_mode";
        static constexpr const char *const DIRECT_NETWORK_INTERFACE = "melonds_direct_network_interface";
    }

    namespace osd {
        static constexpr const char *const CATEGORY = "osd";
        static constexpr const char *const POINTER_COORDINATES = "melonds_show_pointer_coordinates";
        static constexpr const char *const UNSUPPORTED_FEATURES = "melonds_show_unsupported_features";
        static constexpr const char *const MIC_STATE = "melonds_show_mic_state";
        static constexpr const char *const CAMERA_STATE = "melonds_show_camera_state";
        static constexpr const char *const BIOS_WARNINGS = "melonds_show_bios_warnings";
        static constexpr const char *const CURRENT_LAYOUT = "melonds_show_current_layout";
        static constexpr const char *const LID_STATE = "melonds_show_lid_state";
        static constexpr const char *const BRIGHTNESS_STATE = "melonds_show_brightness_state";
    }

    namespace screen {
        static constexpr const char *const CATEGORY = "screen";
        static constexpr const char *const CURSOR_TIMEOUT = "melonds_cursor_timeout";
        static constexpr const char *const HYBRID_RATIO = "melonds_hybrid_ratio";
        static constexpr const char *const HYBRID_SMALL_SCREEN = "melonds_hybrid_small_screen";
        static constexpr const char *const NUMBER_OF_SCREEN_LAYOUTS = "melonds_number_of_screen_layouts";
        static constexpr const char *const SCREEN_GAP = "melonds_screen_gap";
        static constexpr const char *const SCREEN_LAYOUT1 = "melonds_screen_layout1";
        static constexpr const char *const SCREEN_LAYOUT2 = "melonds_screen_layout2";
        static constexpr const char *const SCREEN_LAYOUT3 = "melonds_screen_layout3";
        static constexpr const char *const SCREEN_LAYOUT4 = "melonds_screen_layout4";
        static constexpr const char *const SCREEN_LAYOUT5 = "melonds_screen_layout5";
        static constexpr const char *const SCREEN_LAYOUT6 = "melonds_screen_layout6";
        static constexpr const char *const SCREEN_LAYOUT7 = "melonds_screen_layout7";
        static constexpr const char *const SCREEN_LAYOUT8 = "melonds_screen_layout8";
        static constexpr const char *const SHOW_CURSOR = "melonds_show_cursor";
        static constexpr const char *const TOUCH_MODE = "melonds_touch_mode";
        static constexpr std::array SCREEN_LAYOUTS = {
                SCREEN_LAYOUT1,
                SCREEN_LAYOUT2,
                SCREEN_LAYOUT3,
                SCREEN_LAYOUT4,
                SCREEN_LAYOUT5,
                SCREEN_LAYOUT6,
                SCREEN_LAYOUT7,
                SCREEN_LAYOUT8
        };
    }

    namespace system {
        static constexpr const char *const CATEGORY = "system";
        static constexpr const char *const BATTERY_UPDATE_INTERVAL = "melonds_battery_update_interval";
        static constexpr const char *const BOOT_MODE = "melonds_boot_mode";
        static constexpr const char *const CONSOLE_MODE = "melonds_console_mode";
        static constexpr const char *const DS_POWER_OK = "melonds_ds_battery_ok_threshold";
        static constexpr const char *const FIRMWARE_PATH = "melonds_firmware_nds_path";
        static constexpr const char *const FIRMWARE_DSI_PATH = "melonds_firmware_dsi_path";
        static constexpr const char *const OVERRIDE_FIRMWARE_SETTINGS = "melonds_override_fw_settings";
        static constexpr const char *const SYSFILE_MODE = "melonds_sysfile_mode";
    }

    namespace storage {
        static constexpr const char *const DSI_SD_READ_ONLY = "melonds_dsi_sdcard_readonly";
        static constexpr const char *const DSI_SD_SAVE_MODE = "melonds_dsi_sdcard";
        static constexpr const char *const DSI_SD_SYNC_TO_HOST = "melonds_dsi_sdcard_sync_sdcard_to_host";
        static constexpr const char *const DSI_NAND_PATH = "melonds_dsi_nand_path";
        static constexpr const char *const GBA_FLUSH_DELAY = "melonds_gba_flush_delay";
        static constexpr const char *const HOMEBREW_READ_ONLY = "melonds_homebrew_readonly";
        static constexpr const char *const HOMEBREW_SAVE_MODE = "melonds_homebrew_sdcard";
        static constexpr const char *const HOMEBREW_SYNC_TO_HOST = "melonds_homebrew_sync_sdcard_to_host";
    }

    namespace video {
        static constexpr const char *const CATEGORY = "video";
        static constexpr const char *const OPENGL_BETTER_POLYGONS = "melonds_opengl_better_polygons";
        static constexpr const char *const OPENGL_FILTERING = "melonds_opengl_filtering";
        static constexpr const char *const OPENGL_RESOLUTION = "melonds_opengl_resolution";
        static constexpr const char *const RENDER_MODE = "melonds_render_mode";
        static constexpr const char *const THREADED_RENDERER = "melonds_threaded_renderer";
    }

    namespace values {
        namespace firmware {
            static constexpr const char *const FIRMWARE_USERNAME = "default_username"; // longer than 10 chars so it's not a valid DS username
            static constexpr const char *const GUESS_USERNAME = "guess_username";
            static constexpr const char *const DEFAULT_USERNAME = "melonDS DS";
        }
        namespace wfc {
            static constexpr const char *const DEFAULT = "0.0.0.0";
            static constexpr const char *const ALTWFC = "172.104.88.237";
            static constexpr const char *const KAERU = "178.62.43.212";
            static constexpr const char *const WIIMMFI = "95.217.77.181";
        }
        static constexpr const char *const _10BIT = "10bit";
        static constexpr const char *const _16BIT = "16bit";
        static constexpr const char *const ALWAYS = "always";
        static constexpr const char *const AUTO = "auto";
        static constexpr const char *const BLOW = "blow";
        static constexpr const char *const BOTTOM_TOP = "bottom-top";
        static constexpr const char *const BOTH = "both";
        static constexpr const char *const BOTTOM = "bottom";
        static constexpr const char *const BUILT_IN = "builtin";
        static constexpr const char *const COSINE = "cosine";
        static constexpr const char *const CUBIC = "cubic";
        static constexpr const char *const DEDICATED = "dedicated";
        static constexpr const char *const DEFAULT = "default";
        static constexpr const char *const DIRECT = "direct";
        static constexpr const char *const DISABLED = "disabled";
        static constexpr const char *const DS = "ds";
        static constexpr const char *const DSI = "dsi";
        static constexpr const char *const ENABLED = "enabled";
        static constexpr const char *const ENGLISH = "en";
        static constexpr const char *const EXISTING = "existing";
        static constexpr const char *const FRENCH = "fr";
        static constexpr const char *const GERMAN = "de";
        static constexpr const char *const HOLD = "hold";
        static constexpr const char *const HYBRID_BOTTOM = "hybrid-bottom";
        static constexpr const char *const HYBRID_TOP = "hybrid-top";
        static constexpr const char *const INDIRECT = "indirect";
        static constexpr const char *const ITALIAN = "it";
        static constexpr const char *const JAPANESE = "ja";
        static constexpr const char *const JOYSTICK = "joystick";
        static constexpr const char *const LEFT_RIGHT = "left-right";
        static constexpr const char *const LINEAR = "linear";
        static constexpr const char *const NATIVE = "native";
        static constexpr const char *const NEAREST = "nearest";
        static constexpr const char *const MICROPHONE = "microphone";
        static constexpr const char *const MOUSE = "mouse";
        static constexpr const char *const NOISE = "noise";
        static constexpr const char *const NOT_FOUND = "/notfound";
        static constexpr const char *const ONE = "one";
        static constexpr const char *const OPENGL = "opengl";
        static constexpr const char *const RIGHT_LEFT = "right-left";
        static constexpr const char *const ROTATE_LEFT = "rotate-left";
        static constexpr const char *const ROTATE_RIGHT = "rotate-right";
        static constexpr const char *const SHARED = "shared";
        static constexpr const char *const SILENCE = "silence";
        static constexpr const char *const SOFTWARE = "software";
        static constexpr const char *const SPANISH = "es";
        static constexpr const char *const TIMEOUT = "timeout";
        static constexpr const char *const TOGGLE = "toggle";
        static constexpr const char *const TOP_BOTTOM = "top-bottom";
        static constexpr const char *const TOP = "top";
        static constexpr const char *const TOUCH = "touch";
        static constexpr const char *const TOUCHING = "touching";
        static constexpr const char *const UPSIDE_DOWN = "rotate-180";
    }

    std::optional<bool> ParseBoolean(const char *value) noexcept;
    std::optional<BootMode> ParseBootMode(const char *value) noexcept;
    std::optional<melonds::SysfileMode> ParseSysfileMode(const char *value) noexcept;
    std::optional<AlarmMode> ParseAlarmMode(const char *value) noexcept;
    std::optional<UsernameMode> ParseUsernameMode(const char* value) noexcept;

    std::string GetUsername(UsernameMode mode) noexcept;
    template<typename T>
    std::optional<T> ParseIntegerInRange(const char *value, T min, T max) noexcept {
        if (min > max) return std::nullopt;
        if (!value) return std::nullopt;

        T parsed_number = 0;
        std::from_chars_result result = std::from_chars(value, value + strlen(value), parsed_number);

        if (result.ec != std::errc()) return std::nullopt;
        if (parsed_number < min || parsed_number > max) return std::nullopt;

        return parsed_number;
    }

    template<typename T>
    std::optional<T> ParseIntegerInList(const char *value, const std::initializer_list<T> &list) noexcept {
        if (!value) return std::nullopt;

        T parsed_number = 0;
        std::from_chars_result result = std::from_chars(value, value + strlen(value), parsed_number);

        if (result.ec != std::errc()) return std::nullopt;
        for (T t: list) {
            if (parsed_number == t) return parsed_number;
        }

        return std::nullopt;
    }

    std::optional<melonds::Renderer> ParseRenderer(const char *value) noexcept;

    std::optional<melonds::CursorMode> ParseCursorMode(const char *value) noexcept;

    std::optional<melonds::ConsoleType> ParseConsoleType(const char *value) noexcept;

    std::optional<melonds::NetworkMode> ParseNetworkMode(const char *value) noexcept;

    std::optional<melonds::ScreenLayout> ParseScreenLayout(const char *value) noexcept;

    std::optional<melonds::HybridSideScreenDisplay> ParseHybridSideScreenDisplay(const char *value) noexcept;

    std::optional<melonds::FirmwareLanguage> ParseLanguage(const char* value) noexcept;
    std::optional<melonds::MicInputMode> ParseMicInputMode(const char* value) noexcept;
    std::optional<melonds::TouchMode> ParseTouchMode(const char* value) noexcept;

    std::optional<SPI_Firmware::IpAddress> ParseIpAddress(const char* value) noexcept;

    constexpr size_t DSI_NAND_SIZE = 251658304;
    constexpr std::array<size_t, 3> FIRMWARE_SIZES = { 131072, 262144, 524288 };

    bool IsDsiNandImage(const retro::dirent &file) noexcept;
    bool IsFirmwareImage(const retro::dirent &file, SPI_Firmware::FirmwareHeader& header) noexcept;
}

#endif //MELONDS_DS_CONSTANTS_HPP
