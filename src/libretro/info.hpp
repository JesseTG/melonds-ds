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

#ifndef MELONDS_DS_INFO_HPP
#define MELONDS_DS_INFO_HPP

#include <libretro.h>

namespace melonds {
    constexpr int SLOT_1_2_BOOT = 1;
    constexpr int MELONDSDS_GAME_TYPE_NDS = 0;
    constexpr int MELONDSDS_GAME_TYPE_GBA = 1;
    constexpr int MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT = 1;
    extern const struct retro_system_content_info_override content_overrides[];
    extern const struct retro_subsystem_memory_info gba_memory[];
    extern const struct retro_subsystem_memory_info nds_memory[];
    extern const struct retro_subsystem_rom_info slot_1_2_roms[];
    extern const struct retro_subsystem_info subsystems[];
    extern const struct retro_controller_description controllers[];
    extern const struct retro_controller_info ports[];

    const char* get_game_type_name(unsigned game_type);
}

#endif //MELONDS_DS_INFO_HPP
