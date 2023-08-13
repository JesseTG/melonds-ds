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

#include <charconv>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <system_error>

#include "../config.hpp"

namespace melonds::config {
    namespace audio {
        static const char *const CATEGORY = "audio";
        static const char *const AUDIO_BITDEPTH = "melonds_audio_bitdepth";
        static const char *const AUDIO_INTERPOLATION = "melonds_audio_interpolation";
        static const char *const MIC_INPUT = "melonds_mic_input";
        static const char *const MIC_INPUT_BUTTON = "melonds_mic_input_active";
    }

    namespace cpu {
        static const char* const CATEGORY = "cpu";
        static const char *const JIT_BLOCK_SIZE = "melonds_jit_block_size";
        static const char *const JIT_BRANCH_OPTIMISATIONS = "melonds_jit_branch_optimisations";
        static const char *const JIT_ENABLE = "melonds_jit_enable";
        static const char *const JIT_FAST_MEMORY = "melonds_jit_fast_memory";
        static const char *const JIT_LITERAL_OPTIMISATIONS = "melonds_jit_literal_optimisations";
    }

    namespace network {
        static const char *const CATEGORY = "network";
        static const char *const NETWORK_MODE = "melonds_network_mode";
    }

    namespace screen {
        static const char *const CATEGORY = "screen";
        static const char *const CURSOR_TIMEOUT = "melonds_cursor_timeout";
        static const char *const HYBRID_RATIO = "melonds_hybrid_ratio";
        static const char *const HYBRID_SMALL_SCREEN = "melonds_hybrid_small_screen";
        static const char *const NUMBER_OF_SCREEN_LAYOUTS = "melonds_number_of_screen_layouts";
        static const char *const SCREEN_GAP = "melonds_screen_gap";
        static const char *const SCREEN_LAYOUT1 = "melonds_screen_layout1";
        static const char *const SCREEN_LAYOUT2 = "melonds_screen_layout2";
        static const char *const SCREEN_LAYOUT3 = "melonds_screen_layout3";
        static const char *const SCREEN_LAYOUT4 = "melonds_screen_layout4";
        static const char *const SCREEN_LAYOUT5 = "melonds_screen_layout5";
        static const char *const SCREEN_LAYOUT6 = "melonds_screen_layout6";
        static const char *const SCREEN_LAYOUT7 = "melonds_screen_layout7";
        static const char *const SCREEN_LAYOUT8 = "melonds_screen_layout8";
        static const char *const SHOW_CURSOR = "melonds_show_cursor";
        static const char *const TOUCH_MODE = "melonds_touch_mode";
        static const std::array<const char *const, melonds::config::screen::MAX_SCREEN_LAYOUTS> SCREEN_LAYOUTS = {
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
        static const char *const CATEGORY = "system";
        static const char *const BATTERY_UPDATE_INTERVAL = "melonds_battery_update_interval";
        static const char *const BOOT_DIRECTLY = "melonds_boot_directly";
        static const char *const CONSOLE_MODE = "melonds_console_mode";
        static const char *const DSI_SD_READ_ONLY = "melonds_dsi_sdcard_readonly";
        static const char *const DSI_SD_SAVE_MODE = "melonds_dsi_sdcard";
        static const char *const DSI_SD_SYNC_TO_HOST = "melonds_dsi_sdcard_sync_sdcard_to_host";
        static const char *const DS_POWER_OK = "melonds_ds_battery_ok_threshold";
        static const char *const FAVORITE_COLOR = "melonds_firmware_favorite_color";
        static const char *const GBA_FLUSH_DELAY = "melonds_gba_flush_delay";
        static const char *const HOMEBREW_READ_ONLY = "melonds_homebrew_readonly";
        static const char *const HOMEBREW_SAVE_MODE = "melonds_homebrew_sdcard";
        static const char *const HOMEBREW_SYNC_TO_HOST = "melonds_homebrew_sync_sdcard_to_host";
        static const char *const LANGUAGE = "melonds_language";
        static const char *const OVERRIDE_FIRMWARE_SETTINGS = "melonds_override_fw_settings";
        static const char *const USE_EXTERNAL_BIOS = "melonds_use_external_bios";
    }

    namespace video {
        static const char *const CATEGORY = "video";
        static const char *const OPENGL_BETTER_POLYGONS = "melonds_opengl_better_polygons";
        static const char *const OPENGL_FILTERING = "melonds_opengl_filtering";
        static const char *const OPENGL_RESOLUTION = "melonds_opengl_resolution";
        static const char *const RENDER_MODE = "melonds_render_mode";
        static const char *const THREADED_RENDERER = "melonds_threaded_renderer";
    }

    namespace values {
        static const char *const _10BIT = "10bit";
        static const char *const _16BIT = "16bit";
        static const char *const ALWAYS = "always";
        static const char *const AUTO = "auto";
        static const char *const BLOW = "blow";
        static const char *const BOTTOM_TOP = "bottom-top";
        static const char *const BOTH = "both";
        static const char *const BOTTOM = "bottom";
        static const char *const COSINE = "cosine";
        static const char *const CUBIC = "cubic";
        static const char *const DEDICATED = "dedicated";
        static const char *const DEFAULT = "default";
        static const char *const DIRECT = "direct";
        static const char *const DISABLED = "disabled";
        static const char *const DS = "ds";
        static const char *const DSI = "dsi";
        static const char *const ENABLED = "enabled";
        static const char *const ENGLISH = "en";
        static const char *const FRENCH = "fr";
        static const char *const GERMAN = "de";
        static const char *const HOLD = "hold";
        static const char *const HYBRID_BOTTOM = "hybrid-bottom";
        static const char *const HYBRID_TOP = "hybrid-top";
        static const char *const INDIRECT = "indirect";
        static const char *const ITALIAN = "it";
        static const char *const JAPANESE = "ja";
        static const char *const JOYSTICK = "joystick";
        static const char *const LEFT_RIGHT = "left-right";
        static const char *const LINEAR = "linear";
        static const char *const NEAREST = "nearest";
        static const char *const MICROPHONE = "microphone";
        static const char *const MOUSE = "mouse";
        static const char *const NOISE = "noise";
        static const char *const ONE = "one";
        static const char *const OPENGL = "opengl";
        static const char *const RIGHT_LEFT = "right-left";
        static const char *const ROTATE_LEFT = "rotate-left";
        static const char *const ROTATE_RIGHT = "rotate-right";
        static const char *const SHARED = "shared";
        static const char *const SILENCE = "silence";
        static const char *const SOFTWARE = "software";
        static const char *const SPANISH = "es";
        static const char *const TIMEOUT = "timeout";
        static const char *const TOGGLE = "toggle";
        static const char *const TOP_BOTTOM = "top-bottom";
        static const char *const TOP = "top";
        static const char *const TOUCH = "touch";
        static const char *const TOUCHING = "touching";
        static const char *const UPSIDE_DOWN = "rotate-180";
    }

    std::optional<bool> ParseBoolean(const char *value) noexcept;

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


}

#endif //MELONDS_DS_CONSTANTS_HPP
