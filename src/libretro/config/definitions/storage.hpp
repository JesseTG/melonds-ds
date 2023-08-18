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

#ifndef MELONDS_DS_STORAGE_HPP
#define MELONDS_DS_STORAGE_HPP

#include <array>
#include <libretro.h>

#include "../constants.hpp"

namespace melonds::config::definitions {
    template<retro_language L>
    constexpr std::array StorageOptionDefinitions {
        retro_core_option_v2_definition {
            config::storage::DSI_SD_SAVE_MODE,
            "Virtual SD Card (DSi)",
            nullptr,
            "If enabled, a virtual SD card will be made available to the emulated DSi. "
            "The card image must be within the frontend's system directory and be named dsi_sd_card.bin. "
            "If no image exists, a 4GB virtual SD card will be created. "
            "Ignored when in DS mode. "
            "Changes take effect at next boot.",
            nullptr,
            config::storage::CATEGORY,
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
            config::storage::CATEGORY,
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
            config::storage::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
        },
        retro_core_option_v2_definition {
            config::storage::DSI_NAND_PATH,
            "DSi NAND Path",
            nullptr,
            "Select a DSi NAND image to use. "
            "Files listed here must be:\n"
            "\n"
            "- Placed inside the frontend's system directory, or a subdirectory named \"melonDS DS\" or \"melonDS\".\n"
            "- Exactly 251,658,304 bytes (240MB) in size.\n"
            "\n"
            "DSi mode requires a NAND image, or else it won't start. "
            "Ignored in DS mode. "
            "Takes effect at the next boot (not reset).",
            nullptr,
            config::storage::CATEGORY,
            {
                {melonds::config::values::NOT_FOUND, "None Found"},
                {nullptr, nullptr},
            },
            melonds::config::values::NOT_FOUND
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
            config::storage::CATEGORY,
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
            config::storage::CATEGORY,
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
            config::storage::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
        },
    };
}

#endif //MELONDS_DS_STORAGE_HPP
