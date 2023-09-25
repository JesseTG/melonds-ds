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
            "Console Mode",
            nullptr,
            "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. "
            "DSi mode has some limits:\n"
            "\n"
            "- Native BIOS/firmware/NAND files must be provided, including for the regular DS.\n"
            "- Some features (such as savestates) are not available in DSi mode.\n"
            "\n"
            "See the DSi-specific options in this category for more information. "
            "If unsure, set to DS mode unless playing a DSi game. "
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
            config::system::SYSFILE_MODE,
            "BIOS/Firmware Mode (DS Mode)",
            nullptr,
            "Determines whether melonDS uses native BIOS/firmware dumps "
            "or its own built-in replacements. "
            "Only applies to DS mode.\n"
            "\n"
            "Native mode uses BIOS and firmware files from real DS. "
            "Place your dumps of these in the system directory or its \"melonDS DS\" subdirectory "
            "and name them as follows:\n"
            "\n"
            "- DS BIOS: bios7.bin, bios9.bin\n"
            "- DSi BIOS: dsi_bios7.bin, dsi_bios9.bin\n"
            "- Firmware: See the \"DS Firmware\" and \"DSi Firmware\" options.\n"
            "- DSi NAND: See the \"DSi NAND \" option.\n"
            "\n"
            "Falls back to Built-In if any BIOS/firmware file isn't found.\n"
            "\n"
            "Built-In mode uses melonDS's built-in BIOS and firmware. "
            "Suitable for most games, "
            "but some features (notably GBA connectivity and the DS menu) are not available. "
            "Also used as a fallback from Native mode if any required file isn't found.\n"
            "\n"
            "Changes take effect at next restart.",
            nullptr,
            melonds::config::system::CATEGORY,
            {
                {melonds::config::values::NATIVE, "Native"},
                {melonds::config::values::BUILT_IN, "Built-In"},
                {nullptr, nullptr},
            },
            melonds::config::values::NATIVE
        },
        retro_core_option_v2_definition {
            config::system::FIRMWARE_PATH,
            "DS Firmware",
            nullptr,
            "Select a firmware image to use for DS mode. "
            "Files are listed here if they:\n"
            "\n"
            "- Are inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
            "- Are exactly 131,072 bytes (128KB), 262,144 bytes (256KB), or 524,288 bytes (512KB) long.\n"
            "- Contain valid header data for DS firmware.\n"
            "\n"
            "Nintendo WFC IDs are saved to firmware, "
            "so switching firmware images may result in the loss of some WFC data. "
            "Ignored in DSi mode or if BIOS/Firmware Mode is Built-In. "
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
            "DSi Firmware",
            nullptr,
            "Select a firmware image to use for DSi mode. "
            "Files are listed here if they:\n"
            "\n"
            "- Are inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
            "- Are exactly 131,072 bytes (128KB), 262,144 bytes (256KB), or 524,288 bytes (512KB) long.\n"
            "- Contain valid header data for DSi firmware.\n"
            "\n"
            "Nintendo WFC IDs are saved to firmware, "
            "so switching firmware images may result in the loss of some WFC data. "
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
            config::storage::DSI_NAND_PATH,
            "DSi NAND Path",
            nullptr,
            "Select a DSi NAND image to use. "
            "Required when using DSi mode. "
            "Files are listed here if they:\n"
            "\n"
            "- Are inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
            "- Are exactly 251,658,304 bytes (240MB) long.\n"
            "- Contain valid footer data for DSi NAND images.\n"
            "\n"
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
            config::system::BOOT_MODE,
            "Boot Mode",
            nullptr,
            "Determines how melonDS boots games.\n"
            "\n"
            "Native: Load games through the system menu, "
            "similar to the real DS/DSi boot process. "
            "Requires native BIOS and firmware files in the system directory.\n"
            "Direct: Skip the system menu and go straight to the game.\n"
            "\n"
            "Ignored if loaded without a game (Native is forced), "
            "the loaded game is DSiWare (Native is forced), "
            "or if using Built-In BIOS/Firmware (Direct is forced). "
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
            config::storage::DSI_SD_SAVE_MODE,
            "Virtual SD Card (DSi)",
            nullptr,
            "If enabled, a virtual SD card will be made available to the emulated DSi. "
            "The card image must be within the frontend's system directory and be named dsi_sd_card.bin. "
            "If no image exists, a 4GB virtual SD card will be created. "
            "Ignored in DS mode. "
            "Changes take effect at next boot.",
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
            config::storage::DSI_SD_READ_ONLY,
            "Read-Only Mode (DSi)",
            nullptr,
            "If enabled, the emulated DSi sees the virtual SD card as read-only. "
            "Changes take effect with next restart.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
        },
        retro_core_option_v2_definition {
            config::storage::DSI_SD_SYNC_TO_HOST,
            "Sync SD Card to Host (DSi)",
            nullptr,
            "If enabled, the virtual SD card's files will be synced to this core's save directory. "
            "Enable this if you want to add files to the virtual SD card from outside the core. "
            "Syncing happens when loading and unloading a game, "
            "so external changes won't have any effect while the core is running. "
            "Takes effect at the next boot. "
            "Adjusting this setting may overwrite existing save data.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
        },
        retro_core_option_v2_definition {
            config::storage::HOMEBREW_SAVE_MODE,
            "Virtual SD Card",
            nullptr,
            "If enabled, a virtual SD card will be made available to homebrew DS games. "
            "The card image must be within the frontend's system directory and be named dldi_sd_card.bin. "
            "If no image exists, a 4GB virtual SD card will be created. "
            "Ignored for retail games. "
            "Changes take effect at next boot.",
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
            config::storage::HOMEBREW_READ_ONLY,
            "Read-Only Mode",
            nullptr,
            "If enabled, homebrew applications will see the virtual SD card as read-only. "
            "Changes take effect with next restart.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
        },
        retro_core_option_v2_definition {
            config::storage::HOMEBREW_SYNC_TO_HOST,
            "Sync SD Card to Host",
            nullptr,
            "If enabled, the virtual SD card's files will be synced to this core's save directory. "
            "Enable this if you want to add files to the virtual SD card from outside the core. "
            "Syncing happens when loading and unloading a game, "
            "so external changes won't have any effect while the core is running. "
            "Takes effect at the next boot. "
            "Adjusting this setting may overwrite existing save data.",
            nullptr,
            config::system::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
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
