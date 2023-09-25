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

#include <initializer_list>
#include <libretro.h>

#include "../constants.hpp"

namespace melonds::config::definitions {
    template<retro_language L>
    constexpr std::initializer_list<retro_core_option_v2_definition> SystemOptionDefinitions {
        retro_core_option_v2_definition {
            config::system::CONSOLE_MODE,
            "Console Type",
            nullptr,
            "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. "
            "DSi mode requires a native DSi NAND image, "
            "a native DSi firmware image, "
            "and native BIOS images for the DS and DSi. "
            "Place them in the system directory or its \"melonDS DS\" subdirectory "
            " and name them as follows:\n"
            "\n"
            "- DS BIOS: bios7.bin, bios9.bin\n"
            "- DSi BIOS: dsi_bios7.bin, dsi_bios9.bin\n"
            "- DSi firmware: Anything, set it with the \"DSi Firmware\" option.\n"
            "- DSi NAND: Anything, set it with the \"DSi NAND Path\" option.\n"
            "\n"
            "Ignored if loading a DSiWare game (DSi mode will be forced). "
            "Some features are not available in DSi mode. "
            "If unsure, set to DS mode unless playing a DSi game."
            "Changes take effect at the next restart.",
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
            config::system::BOOT_MODE,
            "Boot Mode",
            nullptr,
            "Determines how melonDS boots games.\n"
            "\n"
            "Native: Load games through the system menu, "
            "similar to the real DS/DSi boot process. "
            "Requires native BIOS and firmware files in the system directory.\n"
            "Direct: Skip the system menu and go straight to the game. "
            "Required if native BIOS/firmware isn't available.\n"
            "\n"
            "Ignored if loaded without a game, "
            "the loaded game is DSiWare, "
            "or native BIOS/firmware files weren't found. "
            "Changes take effect at next restart.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::DIRECT, "Direct"},
                {melonds::config::values::NATIVE, "Native"},
                {nullptr, nullptr},
            },
            melonds::config::values::DIRECT
        },
        retro_core_option_v2_definition {
            config::system::SYSFILE_MODE,
            "BIOS/Firmware Mode",
            nullptr,
            "Determines whether melonDS uses native BIOS/firmware dumps "
            "or its own built-in replacements.\n"
            "\n"
            "Native: Use the same BIOS and firmware files that would be used on a real DS. "
            "Place these in the system directory or its \"melonDS DS\" subdirectory. "
            "Falls back to Built-In if any BIOS/firmware file isn't found."
            "Built-In: Use melonDS's built-in BIOS and firmware. "
            "Suitable for most games, "
            "but some features (notably GBA connectivity and the DS menu) are not available.\n"
            "\n"
            "Ignored in DSi mode, as that requires native BIOS and firmware files. "
            "Changes take effect at next restart.",
            nullptr,
            melonds::config::system::CATEGORY,
            {
                {melonds::config::values::NATIVE, "Native (or Fallback)"},
                {melonds::config::values::BUILT_IN, "Built-In"},
                {nullptr, nullptr},
            },
            melonds::config::values::NATIVE
        },
        retro_core_option_v2_definition {
            config::system::FIRMWARE_PATH,
            "Firmware Path",
            nullptr,
            "Select a firmware image to use for DS mode. "
            "Files listed here must be:\n"
            "\n"
            "- Placed inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
            "- Exactly 131,072 bytes (128KB), 262,144 bytes (256KB), or 524,288 bytes (512KB).\n"
            "\n"
            "Nintendo WFC IDs are saved to firmware, "
            "so switching firmware images may result in the loss of some WFC data. "
            "Ignored in DSi mode or if BIOS/Firmware Mode is Built-In."
            "Changes take effect at next restart.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::NOT_FOUND, "None found..."},
                {nullptr, nullptr},
            },
            melonds::config::values::NOT_FOUND
        },
        retro_core_option_v2_definition {
            config::system::FIRMWARE_DSI_PATH,
            "Firmware Path (DSi)",
            nullptr,
            "Select a firmware image to use for DSi mode. "
            "Files listed here must be:\n"
            "\n"
            "- Placed inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
            "- Exactly 131,072 bytes (128KB), 262,144 bytes (256KB), or 524,288 bytes (512KB).\n"
            "\n"
            "Nintendo WFC IDs are saved to firmware, "
            "so switching firmware images may result in the loss of some WFC data. "
            "DSi mode only. "
            "Changes take effect at next restart.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::NOT_FOUND, "None found..."},
                {nullptr, nullptr},
            },
            melonds::config::values::NOT_FOUND
        },
        retro_core_option_v2_definition {
            config::system::BATTERY_UPDATE_INTERVAL,
            "Battery Update Interval",
            nullptr,
            "How often the emulated console's battery should be updated. "
            "Ignored if the frontend can't get the device's battery level.",
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
