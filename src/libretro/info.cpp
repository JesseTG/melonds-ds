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

const struct retro_system_content_info_override MelonDsDs::content_overrides[] = {
    {
        "srm|sav",
        true,
        false
        // We don't want the frontend to maintain an open handle the GBA save data,
        // as we may want to write back changes later.
    },
    {
        "nds|dsi|ids|gba",
        false,
        true
        // We need to keep the ROM around for reloads
    },
    {}
};


static const struct retro_subsystem_memory_info nds_memory[] = {
    {"srm", RETRO_MEMORY_SAVE_RAM},
};

static const struct retro_subsystem_rom_info slot_1_2_roms[] = {
    {"Nintendo DS (Slot 1)", "nds",     false, false, true,  nds_memory, ARRAY_SIZE(nds_memory)},
    {"GBA (Slot 2)",         "gba",     false, false, true,  nullptr, 0},
    {"GBA Save Data",        "srm|sav", true, true, false, nullptr, 0},
};

const struct retro_subsystem_info MelonDsDs::subsystems[] = {
    {"Slot 1 & 2 Boot", "gba", slot_1_2_roms, ARRAY_SIZE(slot_1_2_roms), MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT},
    {"Slot 1 & 2 Boot (No GBA Save Data)", "gbanosav", slot_1_2_roms, ARRAY_SIZE(slot_1_2_roms) - 1, MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT_NO_SRAM},
    {}
};

const struct retro_controller_description MelonDsDs::controllers[] = {
    {"Nintendo DS", RETRO_DEVICE_JOYPAD},
    {"Nintendo DS (with solar sensor)", MELONDSDS_DEVICE_JOYPAD_WITH_PHOTOSENSOR},
    {},
};

const struct retro_controller_info MelonDsDs::ports[] = {
    {controllers, 2},
    {},
};

const char* MelonDsDs::get_game_type_name(unsigned game_type) {
    switch (game_type) {
        case MELONDSDS_GAME_TYPE_NDS:
            return "MELONDSDS_GAME_TYPE_NDS";
        case MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT:
            return "MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT";
        case MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT_NO_SRAM:
            return "MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT_NO_SRAM";
        default:
            return "<unknown>";
    }
}