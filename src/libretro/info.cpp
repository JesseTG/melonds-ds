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

#include "info.hpp"

#include <retro_miscellaneous.h>

namespace melonds {
    const struct retro_system_content_info_override content_overrides[] = {
            {
                    "srm|sav",
                    false,
                           false
                           // persistent_data is set to false so that the frontend releases the opened file handle.
                           // (We're gonna write back to the file later, so we don't want it to be locked.)
            },
            {
                "nds|dsi|ids|gba",
                false,
                true
                // We need to keep the ROM around for reloads
            },
            {}
    };

    // NOTE: "srm" is a RetroArch convention for save RAM.
    // If other frontends don't support this, we'll have to add a workaround.
    const struct retro_subsystem_memory_info gba_memory[] = {
            {"srm", 0x101},
    };

    const struct retro_subsystem_memory_info nds_memory[] = {
            {"srm", RETRO_MEMORY_SAVE_RAM},
    };

    const struct retro_subsystem_rom_info slot_1_2_roms[] = {
            {"Nintendo DS (Slot 1)", "nds", false, false, true, nds_memory, ARRAY_SIZE(melonds::nds_memory)},
            {"GBA (Slot 2)", "gba", false, false, true, nullptr, 0},
            {"GBA Save Data", "srm", false, false, false, nullptr, 0},
    };

    const struct retro_subsystem_info subsystems[] = {
            {"Slot 1 & 2 Boot", "gba", slot_1_2_roms, ARRAY_SIZE(melonds::slot_1_2_roms), MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT},
            {}
    };

    const struct retro_controller_description controllers[] = {
            {"Nintendo DS", RETRO_DEVICE_JOYPAD},
            {},
    };

    const struct retro_controller_info ports[] = {
            {controllers, 1},
            {},
    };

    const char* get_game_type_name(unsigned game_type)
    {
        switch (game_type) {
            case MELONDSDS_GAME_TYPE_NDS:
                return "MELONDSDS_GAME_TYPE_NDS";
            case MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT:
                return "MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT";
            default:
                return "<unknown>";
        }
    }
}