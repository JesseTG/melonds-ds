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

namespace MelonDsDs::config::definitions {
    constexpr retro_core_option_v2_definition ShowCursor {
        config::screen::SHOW_CURSOR,
        "Cursor Mode",
        nullptr,
        "Determines when a cursor should appear on the bottom screen. "
        "Never is recommended for touch screens; "
        "the other settings are best suited for mouse or joystick input.",
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, "Never"},
            {MelonDsDs::config::values::TOUCHING, "While Touching"},
            {MelonDsDs::config::values::TIMEOUT, "Until Timeout"},
            {MelonDsDs::config::values::ALWAYS, "Always"},
            {nullptr, nullptr},
        },
#if defined(ANDROID) || defined(IOS)
                MelonDsDs::config::values::DISABLED // mobile users won't want to see a cursor by default
#else
        MelonDsDs::config::values::TIMEOUT
#endif
    };

    constexpr retro_core_option_v2_definition CursorTimeout {
        config::screen::CURSOR_TIMEOUT,
        "Cursor Timeout",
        nullptr,
        "If Cursor Mode is set to \"Until Timeout\", "
        "then the cursor will be hidden if it hasn't been moved for a certain time.",
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
    };

    constexpr retro_core_option_v2_definition TouchMode {
        retro_core_option_v2_definition {
            config::screen::TOUCH_MODE,
            "Touch Mode",
            nullptr,
            "Determines how the console's touch screen is emulated.\n"
            "\n"
            "Joystick: Use a joystick to control the cursor. "
            "Recommended if you don't have a mouse or a real touch screen available.\n"
            "Pointer: Use your mouse or touch screen to control the cursor.\n"
            "Auto: Use either Joystick or Pointer, depending on which you last touched.\n"
            "\n"
            "If unsure, set to Auto.",
            nullptr,
            config::screen::CATEGORY,
            {
                {MelonDsDs::config::values::JOYSTICK, "Joystick"},
                {MelonDsDs::config::values::TOUCH, "Pointer"},
                {MelonDsDs::config::values::AUTO, "Auto"},
                {nullptr, nullptr},
            },
            MelonDsDs::config::values::AUTO
        },
    };

    constexpr retro_core_option_v2_definition HybridRatio {
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
    };

    constexpr retro_core_option_v2_definition HybridSmallScreen {
        config::screen::HYBRID_SMALL_SCREEN,
        "Hybrid Small Screen Mode",
        nullptr,
        "Choose which screens will be shown when using a hybrid layout.",
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::ONE, "Show Opposite Screen"},
            {MelonDsDs::config::values::BOTH, "Show Both Screens"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::BOTH
    };

    constexpr retro_core_option_v2_definition ScreenGap {
        config::screen::SCREEN_GAP,
        "Screen Gap",
        nullptr,
        "Choose how large the gap between the screens should be. "
        "Vertical layouts (including rotations) only.",
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
    };

    constexpr retro_core_option_v2_definition NumberOfScreenLayouts {
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
    };

    constexpr retro_core_option_v2_definition ScreenLayout1 {
        config::screen::SCREEN_LAYOUT1,
        "Screen Layout #1",
        "Layout #1",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::TOP_BOTTOM
    };

    constexpr retro_core_option_v2_definition ScreenLayout2 {
        config::screen::SCREEN_LAYOUT2,
        "Screen Layout #2",
        "Layout #2",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::LEFT_RIGHT
    };

    constexpr retro_core_option_v2_definition ScreenLayout3 {
        config::screen::SCREEN_LAYOUT3,
        "Screen Layout #3",
        "Layout #3",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::TOP
    };

    constexpr retro_core_option_v2_definition ScreenLayout4 {
        config::screen::SCREEN_LAYOUT4,
        "Screen Layout #4",
        "Layout #4",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::BOTTOM
    };

    constexpr retro_core_option_v2_definition ScreenLayout5 {
        config::screen::SCREEN_LAYOUT5,
        "Screen Layout #5",
        "Layout #5",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::HYBRID_TOP
    };

    constexpr retro_core_option_v2_definition ScreenLayout6 {
        config::screen::SCREEN_LAYOUT6,
        "Screen Layout #6",
        "Layout #6",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::HYBRID_BOTTOM
    };

    constexpr retro_core_option_v2_definition ScreenLayout7 {
        config::screen::SCREEN_LAYOUT7,
        "Screen Layout #7",
        "Layout #7",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::BOTTOM_TOP
    };

    constexpr retro_core_option_v2_definition ScreenLayout8 {
        config::screen::SCREEN_LAYOUT8,
        "Screen Layout #8",
        "Layout #8",
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {MelonDsDs::config::values::TOP_BOTTOM, "Top/Bottom"},
            {MelonDsDs::config::values::BOTTOM_TOP, "Bottom/Top"},
            {MelonDsDs::config::values::LEFT_RIGHT, "Left/Right"},
            {MelonDsDs::config::values::RIGHT_LEFT, "Right/Left"},
            {MelonDsDs::config::values::TOP, "Top Only"},
            {MelonDsDs::config::values::BOTTOM, "Bottom Only"},
            {MelonDsDs::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {MelonDsDs::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {MelonDsDs::config::values::ROTATE_LEFT, "Rotated Left"},
            {MelonDsDs::config::values::ROTATE_RIGHT, "Rotated Right"},
            {MelonDsDs::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::RIGHT_LEFT
    };

    constexpr std::initializer_list<retro_core_option_v2_definition> ScreenOptionDefinitions {
        ShowCursor,
        CursorTimeout,
        TouchMode,
        NumberOfScreenLayouts,
        ScreenLayout1,
        ScreenLayout2,
        ScreenLayout3,
        ScreenLayout4,
        ScreenLayout5,
        ScreenLayout6,
        ScreenLayout7,
        ScreenLayout8,
        HybridRatio,
        HybridSmallScreen,
        ScreenGap,
    };
}

#endif //MELONDS_DS_SCREEN_HPP
