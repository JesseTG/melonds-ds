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

#ifndef MELONDS_DS_SYSTEM_HPP
#define MELONDS_DS_SYSTEM_HPP

#include <array>
#include <libretro.h>

#include "../constants.hpp"

namespace melonds::config::definitions {

    // If you ever get these translated, turn the variable into a template and
    // make the translated strings variable templates too.
    // Here's an example:
    template<retro_language L>
    constexpr const char* EnglishLabel = "English";

    template<retro_language L>
    constexpr std::array SystemOptionDefinitions {
        retro_core_option_v2_definition {
            config::system::CONSOLE_MODE,
            "Console Type",
            nullptr,
            "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. "
            "Some features may not be available in DSi mode. "
            "DSi mode will be used if loading a DSiWare application.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::DS, "DS"},
                {melonds::config::values::DSI, "DSi (experimental)"},
                {nullptr, nullptr},
            },
            melonds::config::values::DS
        },
        retro_core_option_v2_definition {
            config::system::BOOT_DIRECTLY,
            "Boot Game Directly",
            nullptr,
            "If enabled, melonDS will bypass the native DS menu and boot the loaded game directly. "
            "If disabled, native BIOS and firmware files must be provided in the system directory. "
            "Ignored if any of the following is true:\n"
            "\n"
            "- The core is loaded without a game\n"
            "- Native BIOS/firmware files weren't found\n"
            "- The loaded game is a DSiWare game\n",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::ENABLED
        },
        retro_core_option_v2_definition {
            config::system::OVERRIDE_FIRMWARE_SETTINGS,
            "Override Firmware Settings",
            nullptr,
            "Use language and username specified in the frontend, "
            "rather than those provided by the firmware itself. "
            "If disabled or the firmware is unavailable, these values will be provided by the frontend. "
            "If a name couldn't be found, \"melonDS\" will be used as the default.",
            nullptr,
            melonds::config::system::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
        },
        retro_core_option_v2_definition {
            config::system::LANGUAGE,
            "Language",
            nullptr,
            "The language mode of the emulated console. "
            "Not every game honors this setting. "
            "Automatic uses the frontend's language if supported by the DS, or English if not.",
            nullptr,
            melonds::config::system::CATEGORY,
            {
                {melonds::config::values::AUTO, "Automatic"},
                {melonds::config::values::ENGLISH, EnglishLabel<L>},
                {melonds::config::values::JAPANESE, "Japanese"},
                {melonds::config::values::FRENCH, "French"},
                {melonds::config::values::GERMAN, "German"},
                {melonds::config::values::ITALIAN, "Italian"},
                {melonds::config::values::SPANISH, "Spanish"},
                {nullptr, nullptr},
            },
            melonds::config::values::AUTO
        },
        retro_core_option_v2_definition {
            config::system::FAVORITE_COLOR,
            "Favorite Color",
            nullptr,
            "The theme (\"favorite color\") of the emulated console.",
            nullptr,
            melonds::config::system::CATEGORY,
            {
                {"0", "Gray"},
                {"1", "Brown"},
                {"2", "Red"},
                {"3", "Light Pink"},
                {"4", "Orange"},
                {"5", "Yellow"},
                {"6", "Lime"},
                {"7", "Light Green"},
                {"8", "Dark Green"},
                {"9", "Turquoise"},
                {"10", "Light Blue"},
                {"11", "Blue"},
                {"12", "Dark Blue"},
                {"13", "Dark Purple"},
                {"14", "Light Purple"},
                {"15", "Dark Pink"},
                {nullptr, nullptr},
            },
            "0"
        },
        retro_core_option_v2_definition {
            config::system::USE_EXTERNAL_BIOS,
            "Use external BIOS if available",
            nullptr,
            "If enabled, melonDS will attempt to load a BIOS file from the system directory. "
            "If no valid BIOS is present, melonDS will fall back to its built-in FreeBIOS. "
            "Note that GBA connectivity requires a native BIOS. "
            "Takes effect at the next restart. "
            "If unsure, leave this enabled.",
            nullptr,
            melonds::config::system::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::ENABLED
        },
        retro_core_option_v2_definition {
            config::system::BATTERY_UPDATE_INTERVAL,
            "Battery Update Interval",
            nullptr,
            "How often the emulated console's battery should be updated.",
            nullptr,
            config::system::CATEGORY,
            {
                {"1", "1 second"},
                {"2", "2 seconds"},
                {"3", "3 seconds"},
                {"5", "5 seconds"},
                {"10", "10 seconds"},
                {"15", "15 seconds"},
                {"20", "20 seconds"},
                {"30", "30 seconds"},
                {"60", "60 seconds"},
                {nullptr, nullptr}
            },
            "15"
        },
        retro_core_option_v2_definition {
            config::system::DS_POWER_OK,
            "DS Low Battery Threshold",
            nullptr,
            "If the host's battery level falls below this percentage, "
            "the emulated DS will report that its battery level is low. "
            "Ignored if running in DSi mode, "
            "no battery is available, "
            "or the frontend can't query the power status.",
            nullptr,
            config::system::CATEGORY,
            {
                {"0", "Always OK"},
                {"10", "10%"},
                {"20", "20%"},
                {"30", "30%"},
                {"40", "40%"},
                {"50", "50%"},
                {"60", "60%"},
                {"70", "70%"},
                {"80", "80%"},
                {"90", "90%"},
                {"100", "Always Low"},
                {nullptr, nullptr},
            },
            "20"
        },
    };
}

#endif //MELONDS_DS_SYSTEM_HPP
