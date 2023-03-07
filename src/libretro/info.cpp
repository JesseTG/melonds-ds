//
// Created by Jesse on 3/6/2023.
//

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
            {"NDS Rom (Slot 1)", "nds", false, false, true, nds_memory, 0},
            {"GBA Rom (Slot 2)", "gba", false, false, true, gba_memory, 1},
            {}
    };

    const struct retro_subsystem_info subsystems[] = {
            {"Slot 1/2 Boot", "gba", slot_1_2_roms, 2, SLOT_1_2_BOOT},
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
}