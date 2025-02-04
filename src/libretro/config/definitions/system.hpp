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

#include <libretro.h>

#include "../constants.hpp"

namespace MelonDsDs::config::definitions {
    constexpr retro_core_option_v2_definition ConsoleMode {
        config::system::CONSOLE_MODE,
        "Console Mode",
        nullptr,
        "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. "
        "DSi mode has some limits:\n"
        "\n"
        "- Native BIOS/firmware/NAND files must be provided, including for the regular DS.\n"
        "- Some features (such as savestates) are not available in DSi mode.\n"
        "- Direct boot mode cannot be used for DSiWare.\n"
        "\n"
        "See the DSi-specific options in this category for more information. "
        "If unsure, set to DS mode unless playing a DSi game. "
        "Changes take effect at the next restart.",
        nullptr,
        config::system::CATEGORY,
        {
            {MelonDsDs::config::values::DS, "DS"},
            {MelonDsDs::config::values::DSI, "DSi (experimental)"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DS
    };

    constexpr retro_core_option_v2_definition SysfileMode {
        config::system::SYSFILE_MODE,
        "BIOS/Firmware Mode (DS Mode)",
        nullptr,
        "Determines whether melonDS uses native BIOS/firmware dumps "
        "or its own built-in replacements. "
        "Only applies to DS mode, as DSi mode always requires native BIOS and firmware dumps.\n"
        "\n"
        "Native mode uses BIOS and firmware files from a real DS. "
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
        "Built-In mode uses melonDS's built-in BIOS and firmware, "
        "and is suitable for most games.\n"
        "\n"
        "Changes take effect at next restart.",
        nullptr,
        MelonDsDs::config::system::CATEGORY,
        {
            {MelonDsDs::config::values::NATIVE, "Native"},
            {MelonDsDs::config::values::BUILT_IN, "Built-In"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::NATIVE
    };

    constexpr retro_core_option_v2_definition FirmwarePath {
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
            {MelonDsDs::config::values::NOT_FOUND, "None found..."},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::NOT_FOUND
    };

    constexpr retro_core_option_v2_definition DsiFirmwarePath {
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
            {MelonDsDs::config::values::NOT_FOUND, "None found..."},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::NOT_FOUND
    };

    constexpr retro_core_option_v2_definition NandPath {
        config::storage::DSI_NAND_PATH,
        "DSi NAND Path",
        nullptr,
        "Select a DSi NAND image to use. "
        "Required when using DSi mode. "
        "Files are listed here if they:\n"
        "\n"
        "- Are inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
        "- Are exactly 251,658,304 bytes (240MB) or 257,425,472 bytes (245.5MB) long with valid footer data, OR;\n"
        "- Are 64 bytes shorter than these lengths and contain equivalent data at file offset 0xFF800.\n"
        "\n"
        "Changes take effect at next restart.",
        nullptr,
        config::system::CATEGORY,
        {
            {MelonDsDs::config::values::NOT_FOUND, "None found..."},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::NOT_FOUND
    };

    constexpr retro_core_option_v2_definition BootMode {
        config::system::BOOT_MODE,
        "Boot Mode",
        nullptr,
        "Determines how melonDS boots games.\n"
        "\n"
        "Native: Load games through the system menu, "
        "similar to the real DS/DSi boot process. "
        "Requires native BIOS and firmware files in the system directory.\n"
        "Direct: Skip the system menu and go straight to the game. "
        "Supported for DSiWare if native DSi system files are available.\n"
        "\n"
        "Ignored if loaded without a game (Native is forced) "
        "or if using Built-In BIOS/Firmware (Direct is forced). "
        "Changes take effect at next restart.",
        nullptr,
        config::system::CATEGORY,
        {
            {MelonDsDs::config::values::DIRECT, "Direct"},
            {MelonDsDs::config::values::NATIVE, "Native"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DIRECT
    };

    constexpr retro_core_option_v2_definition DsiSdCardSaveMode {
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
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };
    constexpr retro_core_option_v2_definition DsiSdCardReadOnly {
        config::storage::DSI_SD_READ_ONLY,
        "Read-Only Mode (DSi)",
        nullptr,
        "If enabled, the emulated DSi sees the virtual SD card as read-only. "
        "Changes take effect with next restart.",
        nullptr,
        config::system::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };

    constexpr retro_core_option_v2_definition DsiSdCardSyncToHost {
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
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };

    constexpr retro_core_option_v2_definition HomebrewSdCard {
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
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition HomebrewSdCardReadOnly {
        config::storage::HOMEBREW_READ_ONLY,
        "Read-Only Mode",
        nullptr,
        "If enabled, homebrew applications will see the virtual SD card as read-only. "
        "Changes take effect with next restart.",
        nullptr,
        config::system::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };

    constexpr retro_core_option_v2_definition HomebrewSdCardSyncToHost {
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
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };
    constexpr retro_core_option_v2_definition BatteryUpdateInterval {
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
    };

    constexpr retro_core_option_v2_definition NdsPowerOkThreshold {
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
    };

    constexpr retro_core_option_v2_definition Slot2Device {
        config::system::SLOT2_DEVICE,
        "Slot-2 Device",
        nullptr,
        "The peripheral that will be inserted into the emulated console's Slot-2. "
        "Ignored in DSi mode, or if a GBA ROM is explicitly loaded. "
        "If a GBA ROM for any game in the Boktai trilogy is inserted, "
        "the corresponding Solar Sensor setting will be forced. "
        "In supported NDS games, each Boktai cart will have different effects. "
        "\n"
        "Changes take effect at next core start.",
        nullptr,
        config::system::CATEGORY,
        {
            {values::AUTO, "Auto"},
            {values::RUMBLE_PAK, "Rumble Pak"},
            {values::EXPANSION_PAK, "Memory Expansion Pak"},
            {values::system::SOLAR_SENSOR_1, "Solar Sensor (Boktai 1)"},
            {values::system::SOLAR_SENSOR_2, "Solar Sensor (Boktai 2)"},
            {values::system::SOLAR_SENSOR_3, "Solar Sensor (Boktai 3)"},
            {nullptr, nullptr},
        },
        values::AUTO
    };

    constexpr retro_core_option_v2_definition SolarSensorMode {
        config::system::SOLAR_SENSOR_HOST_SENSOR,
        "Use Host Light Sensor",
        nullptr,
        "If enabled, the emulated solar sensor's light input "
        "will come from your device's real luminance sensor if available. "
        "If disabled or no sensor is available, "
        "the emulated sensor can be manually controlled with the following button combos:\n"
        "\n"
        "- L+A+Up or L+A+Down\n"
        "- Select+Up or Select+Down\n"
        "- Your mouse wheel\n"
        "\n"
        "Ignored unless \"Slot-2 Device\" is set to Solar Sensor, "
        "or if a ROM for any game in the Boktai trilogy (any region) "
        "is inserted in the emulated console's Slot-2. "
        "If unsure, leave enabled.",
        nullptr,
        config::system::CATEGORY,
        {
            {values::DISABLED, nullptr},
            {values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        values::ENABLED
    };

    constexpr std::initializer_list<retro_core_option_v2_definition> SystemOptionDefinitions {
        ConsoleMode,
        SysfileMode,
        FirmwarePath,
        DsiFirmwarePath,
        NandPath,
        BootMode,
        DsiSdCardSaveMode,
        DsiSdCardReadOnly,
        DsiSdCardSyncToHost,
        Slot2Device,
        HomebrewSdCard,
        HomebrewSdCardReadOnly,
        HomebrewSdCardSyncToHost,
        BatteryUpdateInterval,
        NdsPowerOkThreshold,
    };
}

#endif //MELONDS_DS_SYSTEM_HPP
