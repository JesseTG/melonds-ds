//
// Created by Jesse on 3/6/2023.
//

#ifndef MELONDS_DS_INFO_HPP
#define MELONDS_DS_INFO_HPP

#include <libretro.h>

namespace melonds {
    constexpr int SLOT_1_2_BOOT = 1;
    extern const struct retro_system_content_info_override content_overrides[];
    extern const struct retro_subsystem_memory_info gba_memory[];
    extern const struct retro_subsystem_memory_info nds_memory[];
    extern const struct retro_subsystem_rom_info slot_1_2_roms[];
    extern const struct retro_subsystem_info subsystems[];
    extern const struct retro_controller_description controllers[];
    extern const struct retro_controller_info ports[];
}

#endif //MELONDS_DS_INFO_HPP
