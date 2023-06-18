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

namespace melonds {
    const struct retro_system_content_info_override content_overrides[] = {
            {
                    "nds|dsi|gba",
                    false,
                           true
            },
            {NULL,  false, false}
    };

    const struct retro_subsystem_memory_info gba_memory[] = {
            {"srm", 0x101},
    };

    const struct retro_subsystem_memory_info nds_memory[] = {
            {"sav", 0x102},
    };

    const struct retro_subsystem_rom_info slot_1_2_roms[]{
            {"Nintendo DS (Slot 1)", "nds", false, false, true, nds_memory, 0},
            {"GBA (Slot 2)", "gba", false, false, true, gba_memory, 1},
            {}
    };

    const struct retro_subsystem_info subsystems[] = {
            {"Slot 1 & 2 Boot", "gba", slot_1_2_roms, 2, MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT},
            {}
    };

    const struct retro_controller_description controllers[] = {
            {"Nintendo DS", RETRO_DEVICE_JOYPAD},
            {NULL, 0},
    };

    const struct retro_controller_info ports[] = {
            {controllers, 1},
            {NULL,        0},
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