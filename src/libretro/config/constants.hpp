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
#include <system_error>
#include <SPI_Firmware.h>

#ifdef RELATIVE
#undef RELATIVE
#endif

namespace MelonDsDs {
    enum class UsernameMode;
}

namespace retro {
    struct dirent;
}

namespace MelonDsDs::config {
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
        static constexpr const char *const SENSOR_READING = "melonds_show_sensor_reading";
        static constexpr const char *const BRIGHTNESS_STATE = "melonds_show_brightness_state";
    }

    namespace screen {
        constexpr unsigned MAX_HYBRID_RATIO = 3;
        constexpr unsigned MAX_SCREEN_LAYOUTS = 8; // Chosen arbitrarily; if you need more, open a PR
        constexpr unsigned MAX_SCREEN_GAP = 126;
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
        static constexpr const char *const RUMBLE_INTENSITY = "melonds_rumble_intensity";
        static constexpr const char *const RUMBLE_TYPE = "melonds_rumble_type";
        static constexpr const char *const SLOT2_DEVICE = "melonds_slot2_device";
        static constexpr const char *const SOLAR_SENSOR_INPUT_MODE = "melonds_solar_sensor_input_mode";
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

    namespace time {
        static constexpr const char *const CATEGORY = "time";
        static constexpr const char *const SYNC_TIME_MODE = "melonds_sync_time_mode";
        static constexpr const char *const START_TIME_MODE = "melonds_start_time_mode";
        static constexpr const char *const RELATIVE_YEAR_OFFSET = "melonds_start_time_relative_year_offset";
        static constexpr const char *const RELATIVE_DAY_OFFSET = "melonds_start_time_relative_day_offset";
        static constexpr const char *const RELATIVE_HOUR_OFFSET = "melonds_start_time_relative_hour_offset";
        static constexpr const char *const RELATIVE_MINUTE_OFFSET = "melonds_start_time_relative_minute_offset";
        static constexpr const char *const ABSOLUTE_YEAR = "melonds_start_time_absolute_year";
        static constexpr const char *const ABSOLUTE_MONTH = "melonds_start_time_absolute_month";
        static constexpr const char *const ABSOLUTE_DAY = "melonds_start_time_absolute_day";
        static constexpr const char *const ABSOLUTE_HOUR = "melonds_start_time_absolute_hour";
        static constexpr const char *const ABSOLUTE_MINUTE = "melonds_start_time_absolute_minute";
    }

    namespace video {
        constexpr unsigned INITIAL_MAX_OPENGL_SCALE = 4;
        constexpr unsigned MAX_OPENGL_SCALE = 8;
        static constexpr const char *const CATEGORY = "video";
        static constexpr const char *const OPENGL_BETTER_POLYGONS = "melonds_opengl_better_polygons";
        static constexpr const char *const OPENGL_FILTERING = "melonds_opengl_filtering";
        static constexpr const char *const OPENGL_RESOLUTION = "melonds_opengl_resolution";
        static constexpr const char *const RENDER_MODE = "melonds_render_mode";
        static constexpr const char *const THREADED_RENDERER = "melonds_threaded_renderer";
    }

    namespace values {
        namespace firmware {
            static constexpr const char *const FIRMWARE_USERNAME = "existing_username"; // longer than 10 chars so it's not a valid DS username
            static constexpr const char *const GUESS_USERNAME = "guess_username";
            static constexpr const char *const DEFAULT_USERNAME = "melonDS DS";
        }
        namespace system {
            static constexpr const char *const SOLAR_SENSOR_1 = "solar1";
            static constexpr const char *const SOLAR_SENSOR_2 = "solar2";
            static constexpr const char *const SOLAR_SENSOR_3 = "solar3";
        }
        namespace wfc {
            static constexpr const char *const DEFAULT = "0.0.0.0";
            static constexpr const char *const ALTWFC = "172.104.88.237";
            static constexpr const char *const KAERU = "178.62.43.212";
            static constexpr const char *const WIIMMFI = "95.217.77.181";
        }
        static constexpr const char *const _10BIT = "10bit";
        static constexpr const char *const _16BIT = "16bit";
        static constexpr const char *const ABSOLUTE_TIME = "absolute";
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
        static constexpr const char *const EXPANSION_PAK = "expansion-pak";
        static constexpr const char *const FIRMWARE = "firmware";
        static constexpr const char *const FLIPPED_HYBRID_BOTTOM = "flipped-hybrid-bottom";
        static constexpr const char *const FLIPPED_HYBRID_TOP = "flipped-hybrid-top";
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
        static constexpr const char *const REAL = "real";
        static constexpr const char *const RELATIVE_TIME = "relative";
        static constexpr const char *const RIGHT_LEFT = "right-left";
        static constexpr const char *const ROTATE_LEFT = "rotate-left";
        static constexpr const char *const ROTATE_RIGHT = "rotate-right";
        static constexpr const char *const RUMBLE_PAK = "rumble-pak";
        static constexpr const char *const SENSOR = "sensor";
        static constexpr const char *const SHARED = "shared";
        static constexpr const char *const SILENCE = "silence";
        static constexpr const char *const SOFTWARE = "software";
        static constexpr const char *const SPANISH = "es";
        static constexpr const char *const START = "start";
        static constexpr const char *const STRONG = "strong";
        static constexpr const char *const SYNC = "sync";
        static constexpr const char *const TIMEOUT = "timeout";
        static constexpr const char *const TOGGLE = "toggle";
        static constexpr const char *const TOP_BOTTOM = "top-bottom";
        static constexpr const char *const TOP = "top";
        static constexpr const char *const TOUCH = "touch";
        static constexpr const char *const TOUCHING = "touching";
        static constexpr const char *const UPSIDE_DOWN = "rotate-180";
        static constexpr const char *const WEAK = "weak";
    }

    constexpr size_t NOCASH_FOOTER_SIZE = 0x40;
    constexpr size_t NOCASH_FOOTER_OFFSET = 0xFF800;
    constexpr std::array<size_t, 2> DSI_NAND_SIZES_NOFOOTER = { 0xF000000, 0xF580000 }; // Taken from GBATek
    constexpr const char *const NOCASH_FOOTER_MAGIC = "DSi eMMC CID/CPU";
    constexpr size_t NOCASH_FOOTER_MAGIC_SIZE = 16;
    constexpr std::array<size_t, 3> FIRMWARE_SIZES = { 131072, 262144, 524288 };

    bool IsDsiNandImage(const retro::dirent &file) noexcept;
    bool IsFirmwareImage(const retro::dirent &file, melonDS::Firmware::FirmwareHeader& header) noexcept;

    // Source: https://github.com/DS-Homebrew/TWiLightMenu/blob/a836b7d30b3582d57af848dde2277ded9dfe3a50/romsel_r4theme/arm9/source/graphics/uvcoord_small_font.h#L451-L461
    static constexpr char16_t NdsCharacterSet[] = {
        u' ', u'!', u'"', u'#', u'$', u'%', u'&', u'\'', u'(', u')', u'*', u'+', u',', u'-', u'.', u'/', u'0', u'1',
        u'2', u'3', u'4', u'5', u'6', u'7', u'8', u'9', u':', u';', u'<', u'=', u'>', u'?', u'@', u'A', u'B', u'C',
        u'D', u'E', u'F', u'G', u'H', u'I', u'J', u'K', u'L', u'M', u'N', u'O', u'P', u'Q', u'R',
        u'S', u'T', u'U', u'V', u'W', u'X', u'Y', u'Z', u'[', u'\\', u']', u'^', u'_', u'`', u'a', u'b', u'c', u'd',
        u'e', u'f', u'g', u'h', u'i', u'j', u'k', u'l', u'm', u'n', u'o', u'p', u'q', u'r', u's', u't', u'u', u'v',
        u'w', u'x', u'y', u'z', u'[', u'|', u']', u'~', u'¡', u'¢', u'£', u'¤', u'¥', u'¦', u'§',
        u'¨', u'©', u'ª', u'«', u'¬', u'®', u'°', u'±', u'²', u'³', u'´', u'µ', u'¶', u'·', u'»', u'¿', u'À', u'Á',
        u'Â', u'Ã', u'Ä', u'Å', u'Æ', u'Ç', u'È', u'É', u'Ê', u'Ë', u'Ì', u'Í', u'Î', u'Ï', u'Ð', u'Ñ', u'Ò', u'Ó',
        u'Ô', u'Õ', u'Ö', u'×', u'Ø', u'Ù', u'Ú', u'Û', u'Ü', u'Ý', u'Þ', u'ß', u'à', u'á', u'â',
        u'ã', u'ä', u'å', u'æ', u'ç', u'è', u'é', u'ê', u'ë', u'ì', u'í', u'î', u'ï', u'ð', u'ñ', u'ò', u'ó', u'ô',
        u'õ', u'ö', u'÷', u'ø', u'ù', u'ú', u'û', u'ü', u'ý', u'þ', u'ÿ', u'Ÿ', u'ẞ', u'‘', u'’', u'‚', u'“', u'“',
        u'„', u'•', u'…', u'‹', u'›', u'€', u'™', u'←', u'↑', u'→', u'↓', u'\u2427', u'\u2428', u'\u2429', u'\u242A',
        u'\u242B', u'\u242C', u'\u242D', u'\u242E', u'\u242F', u'\u2430', u'■', u'□', u'▲', u'△', u'▼', u'▽', u'◆',
        u'◇', u'○', u'◎', u'●', u'\u2600', u'\u2601', u'\u2602', u'\u2603', u'\u2605', u'\u2606', u'\u260E', u'\u2613',
        u'\u2639', u'☹', u'☻', u'♠', u'♣', u'♥', u'♦', u'\u3041', u'\u3042', u'\u3043', u'\u3044', u'\u3045', u'\u3046',
        u'\u3047', u'\u3048', u'\u3049', u'\u304A', u'\u304B', u'\u304C', u'\u304D', u'\u304E', u'\u304F', u'\u3050',
        u'\u3051', u'\u3052', u'\u3053',
        u'\u3054', u'\u3055', u'\u3056', u'\u3057', u'\u3058', u'\u3059', u'\u305A', u'\u305B', u'\u305C', u'\u305D',
        u'\u305E', u'\u305F', u'\u3060', u'\u3061', u'\u3062', u'\u3063', u'\u3064', u'\u3065', u'\u3066', u'\u3067',
        u'\u3068', u'\u3069', u'\u306A', u'\u306B', u'\u306C', u'\u306D', u'\u306E', u'\u306F', u'\u3070', u'\u3071',
        u'\u3072', u'\u3073', u'\u3074', u'\u3075', u'\u3076', u'\u3077', u'\u3078', u'\u3079', u'\u307A', u'\u307B',
        u'\u307C', u'\u307D', u'\u307E', u'\u307F', u'\u3080', u'\u3081', u'\u3082', u'\u3083', u'\u3084', u'\u3085',
        u'\u3086',
        u'\u3087', u'\u3088', u'\u3089', u'\u308A', u'\u308B', u'\u308C', u'\u308D', u'\u308E', u'\u308F', u'\u3090',
        u'\u3091', u'\u3092', u'\u3093', u'\u3094', u'\u3095', u'\u3096', u'\u3099', u'\u309A', u'\u309B', u'\u309C',
        u'\u309D', u'\u309E', u'\u309F', u'\u30A0', u'\u30A1', u'\u30A2', u'\u30A3', u'\u30A4', u'\u30A5', u'\u30A6',
        u'\u30A7', u'\u30A8', u'\u30A9', u'\u30AA', u'\u30AB', u'\u30AC', u'\u30AD', u'\u30AE', u'\u30AF', u'\u30B0',
        u'\u30B1', u'\u30B2', u'\u30B3', u'\u30B4', u'\u30B5', u'\u30B6', u'\u30B7', u'\u30B8', u'\u30B9', u'\u30BA',
        u'\u30BB',
        u'\u30BC', u'\u30BD', u'\u30BE', u'\u30BF', u'\u30C0', u'\u30C1', u'\u30C2', u'\u30C3', u'\u30C4', u'\u30C5',
        u'\u30C6', u'\u30C7', u'\u30C8', u'\u30C9', u'\u30CA', u'\u30CB', u'\u30CC', u'\u30CD', u'\u30CE', u'\u30CF',
        u'\u30D0', u'\u30D1', u'\u30D2', u'\u30D3', u'\u30D4', u'\u30D5', u'\u30D6', u'\u30D7', u'\u30D8', u'\u30D9',
        u'\u30DA', u'\u30DB', u'\u30DC', u'\u30DD', u'\u30DE', u'\u30DF', u'\u30E0', u'\u30E1', u'\u30E2', u'\u30E3',
        u'\u30E4', u'\u30E5', u'\u30E6', u'\u30E7', u'\u30E8', u'\u30E9', u'\u30EA', u'\u30EB', u'\u30EC', u'\u30ED',
        u'\u30EE',
        u'\u30EF', u'\u30F0', u'\u30F1', u'\u30F2', u'\u30F3', u'\u30F4', u'\u30F5', u'\u30F6', u'\u30F7', u'\u30F8',
        u'\u30F9', u'\u30FA', u'\u30FB', u'\u30FC', u'\u30FD', u'\u30FE', u'\u30FF', u'\uFFFF', u'\0'
    };
}

#endif //MELONDS_DS_CONSTANTS_HPP
