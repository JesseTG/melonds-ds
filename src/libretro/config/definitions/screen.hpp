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

#ifndef MELONDS_DS_SCREEN_HPP
#define MELONDS_DS_SCREEN_HPP

#include <initializer_list>
#include <libretro.h>

#include "../constants.hpp"

namespace melonds::config::definitions {
    template<retro_language L>
    constexpr std::initializer_list<retro_core_option_v2_definition> ScreenOptionDefinitions {
        retro_core_option_v2_definition {
            config::screen::SHOW_CURSOR,
            "Cursor Mode",
            nullptr,
            "Determines when a cursor should appear on the bottom screen. "
            "Never is recommended for touch screens; "
            "the other settings are best suited for mouse or joystick input.",
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::DISABLED, "Never"},
                {melonds::config::values::TOUCHING, "While Touching"},
                {melonds::config::values::TIMEOUT, "Until Timeout"},
                {melonds::config::values::ALWAYS, "Always"},
                {nullptr, nullptr},
            },
            melonds::config::values::ALWAYS
        },
        retro_core_option_v2_definition {
            config::screen::CURSOR_TIMEOUT,
            "Cursor Timeout",
            nullptr,
            "If Cursor Mode is set to \"Until Timeout\", "
            "then the cursor will be hidden if the pointer hasn't moved for a certain time.",
            nullptr,
            config::screen::CATEGORY,
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
                {nullptr, nullptr},
            },
            "3"
        },
        retro_core_option_v2_definition {
            config::screen::HYBRID_RATIO,
            "Hybrid Ratio",
            nullptr,
            "The size of the larger screen relative to the smaller ones when using a hybrid layout.",
            nullptr,
            config::screen::CATEGORY,
            {
                {"2", "2:1"},
                {"3", "3:1"},
                {nullptr, nullptr},
            },
            "2"
        },
        retro_core_option_v2_definition {
            config::screen::HYBRID_SMALL_SCREEN,
            "Hybrid Small Screen Mode",
            nullptr,
            "Choose which screens will be shown when using a hybrid layout.",
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::ONE, "Show Opposite Screen"},
                {melonds::config::values::BOTH, "Show Both Screens"},
                {nullptr, nullptr},
            },
            melonds::config::values::BOTH
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_GAP,
            "Screen Gap",
            nullptr,
            "Choose how large the gap between the 2 screens should be.",
            nullptr,
            config::screen::CATEGORY,
            {
                {"0", "None"},
                {"1", "1px"},
                {"2", "2px"},
                {"8", "8px"},
                {"16", "16px"},
                {"24", "24px"},
                {"32", "32px"},
                {"48", "48px"},
                {"64", "64px"},
                {"72", "72px"},
                {"88", "88px"},
                {"90", "90px"},
                {"128", "128px"},
                {nullptr, nullptr},
            },
            "0"
        },
        retro_core_option_v2_definition {
            config::screen::NUMBER_OF_SCREEN_LAYOUTS,
            "# of Screen Layouts",
            nullptr,
            "The number of screen layouts to cycle through with the Next Layout button.",
            nullptr,
            config::screen::CATEGORY,
            {
                {"1", nullptr},
                {"2", nullptr},
                {"3", nullptr},
                {"4", nullptr},
                {"5", nullptr},
                {"6", nullptr},
                {"7", nullptr},
                {"8", nullptr},
                {nullptr, nullptr},
            },
            "2"
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT1,
            "Screen Layout #1",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::TOP_BOTTOM
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT2,
            "Screen Layout #2",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::LEFT_RIGHT
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT3,
            "Screen Layout #3",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::TOP
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT4,
            "Screen Layout #4",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::BOTTOM
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT5,
            "Screen Layout #5",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::HYBRID_TOP
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT6,
            "Screen Layout #6",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::HYBRID_BOTTOM
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT7,
            "Screen Layout #7",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::BOTTOM_TOP
        },
        retro_core_option_v2_definition {
            config::screen::SCREEN_LAYOUT8,
            "Screen Layout #8",
            nullptr,
            nullptr,
            nullptr,
            config::screen::CATEGORY,
            {
                {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
                {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
                {melonds::config::values::LEFT_RIGHT, "Left/Right"},
                {melonds::config::values::RIGHT_LEFT, "Right/Left"},
                {melonds::config::values::TOP, "Top Only"},
                {melonds::config::values::BOTTOM, "Bottom Only"},
                {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
                {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
                {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
                {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
                {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
                {nullptr, nullptr},
            },
            melonds::config::values::RIGHT_LEFT
        },
    };
}

#endif //MELONDS_DS_SCREEN_HPP
