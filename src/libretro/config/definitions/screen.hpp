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
        "Vertical layouts (including rotations) only. "
        "Dimensions exclude the effects of the frontend's scaling.",
        nullptr,
        config::screen::CATEGORY,
        {
            {"0", "None"},
            {"1", "1px"},
            {"2", "2px"},
            {"3", "3px"},
            {"4", "4px"},
            {"5", "5px"},
            {"6", "6px"},
            {"7", "7px"},
            {"8", "8px"},
            {"9", "9px"},
            {"10", "10px"},
            {"11", "11px"},
            {"12", "12px"},
            {"13", "13px"},
            {"14", "14px"},
            {"15", "15px"},
            {"16", "16px"},
            {"17", "17px"},
            {"18", "18px"},
            {"19", "19px"},
            {"20", "20px"},
            {"21", "21px"},
            {"22", "22px"},
            {"23", "23px"},
            {"24", "24px"},
            {"25", "25px"},
            {"26", "26px"},
            {"27", "27px"},
            {"28", "28px"},
            {"29", "29px"},
            {"30", "30px"},
            {"31", "31px"},
            {"32", "32px"},
            {"33", "33px"},
            {"34", "34px"},
            {"35", "35px"},
            {"36", "36px"},
            {"37", "37px"},
            {"38", "38px"},
            {"39", "39px"},
            {"40", "40px"},
            {"41", "41px"},
            {"42", "42px"},
            {"43", "43px"},
            {"44", "44px"},
            {"45", "45px"},
            {"46", "46px"},
            {"47", "47px"},
            {"48", "48px"},
            {"49", "49px"},
            {"50", "50px"},
            {"51", "51px"},
            {"52", "52px"},
            {"53", "53px"},
            {"54", "54px"},
            {"55", "55px"},
            {"56", "56px"},
            {"57", "57px"},
            {"58", "58px"},
            {"59", "59px"},
            {"60", "60px"},
            {"61", "61px"},
            {"62", "62px"},
            {"63", "63px"},
            {"64", "64px"},
            {"65", "65px"},
            {"66", "66px"},
            {"67", "67px"},
            {"68", "68px"},
            {"69", "69px"},
            {"70", "70px"},
            {"71", "71px"},
            {"72", "72px"},
            {"73", "73px"},
            {"74", "74px"},
            {"75", "75px"},
            {"76", "76px"},
            {"77", "77px"},
            {"78", "78px"},
            {"79", "79px"},
            {"80", "80px"},
            {"81", "81px"},
            {"82", "82px"},
            {"83", "83px"},
            {"84", "84px"},
            {"85", "85px"},
            {"86", "86px"},
            {"87", "87px"},
            {"88", "88px"},
            {"89", "89px"},
            {"90", "90px"},
            {"91", "91px"},
            {"92", "92px"},
            {"93", "93px"},
            {"94", "94px"},
            {"95", "95px"},
            {"96", "96px"},
            {"97", "97px"},
            {"98", "98px"},
            {"99", "99px"},
            {"100", "100px"},
            {"101", "101px"},
            {"102", "102px"},
            {"103", "103px"},
            {"104", "104px"},
            {"105", "105px"},
            {"106", "106px"},
            {"107", "107px"},
            {"108", "108px"},
            {"109", "109px"},
            {"110", "110px"},
            {"111", "111px"},
            {"112", "112px"},
            {"113", "113px"},
            {"114", "114px"},
            {"115", "115px"},
            {"116", "116px"},
            {"117", "117px"},
            {"118", "118px"},
            {"119", "119px"},
            {"120", "120px"},
            {"121", "121px"},
            {"122", "122px"},
            {"123", "123px"},
            {"124", "124px"},
            {"125", "125px"},
            {"126", "126px"},
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
