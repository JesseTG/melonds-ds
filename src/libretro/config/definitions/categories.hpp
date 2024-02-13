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

#ifndef MELONDS_DS_CATEGORIES_HPP
#define MELONDS_DS_CATEGORIES_HPP

#include <array>
#include <libretro.h>

#include "../constants.hpp"

namespace MelonDsDs::config::definitions {
    constexpr std::array OptionCategories {
        retro_core_option_v2_category {
            MelonDsDs::config::system::CATEGORY,
            "System",
            "Change system settings."
        },
        retro_core_option_v2_category {
            MelonDsDs::config::time::CATEGORY,
            "Date/Time",
            "Change date and time settings."
        },
        retro_core_option_v2_category {
            MelonDsDs::config::video::CATEGORY,
            "Video",
            "Change video settings."
        },
        retro_core_option_v2_category {
            MelonDsDs::config::audio::CATEGORY,
            "Audio",
            "Settings for audio output and microphone input."
        },
        retro_core_option_v2_category {
            MelonDsDs::config::screen::CATEGORY,
            "Screen",
            "Change screen settings."
        },
        retro_core_option_v2_category {
            MelonDsDs::config::firmware::CATEGORY,
            "Firmware",
            "Override the emulated firmware's settings."
        },
        retro_core_option_v2_category {
            MelonDsDs::config::network::CATEGORY,
            "Network",
            "Change Nintendo Wi-Fi emulation settings."
        },
#ifdef JIT_ENABLED
        retro_core_option_v2_category {
            MelonDsDs::config::cpu::CATEGORY,
            "CPU Emulation",
            "Change CPU emulation settings."
        },
#endif
        retro_core_option_v2_category {
            MelonDsDs::config::osd::CATEGORY,
            "On-Screen Display & Notifications",
            "Change what extra information is shown on-screen."
        },
        retro_core_option_v2_category {nullptr, nullptr, nullptr},
    };
}
#endif //MELONDS_DS_CATEGORIES_HPP
