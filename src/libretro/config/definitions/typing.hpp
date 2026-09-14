/*
    Copyright 2026 Davey Hughes

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

#ifndef MELONDS_DS_TYPING_HPP
#define MELONDS_DS_TYPING_HPP

#include <libretro.h>

#include "../constants.hpp"

namespace MelonDsDs::config::definitions {
    constexpr retro_core_option_v2_definition TypingKeyboard {
        config::typing::KEYBOARD,
        "Type With Your Keyboard",
        nullptr,
        "Type into Learn with Pokémon: Typing Adventure with your own keyboard, "
        "in place of the wireless keyboard that came with the game. "
        "Unless Game Focus is on, RetroArch keeps keys bound to buttons or hotkeys for itself, "
        "so turn it on to type freely: "
        "press the Game Focus hotkey (Scroll Lock by default), "
        "or set \"Auto Enable 'Game Focus' Mode\" to Detect to have it turn on whenever this game loads. "
        "Has no effect on other games.",
        nullptr,
        config::typing::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition TypingAutoPair {
        config::typing::AUTO_PAIR,
        "Automatically Send Fn on Start",
        nullptr,
        "Have the keyboard answer the game's search for it as soon as the console starts, "
        "as if it had been switched on with Fn held. "
        "If disabled, press your Fn key when the game asks you to.",
        nullptr,
        config::typing::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition TypingFnKey {
        config::typing::FN_KEY,
        "Fn Key",
        nullptr,
        "The key on your keyboard that stands in for the wireless keyboard's Fn key.",
        nullptr,
        config::typing::CATEGORY,
        {
            {MelonDsDs::config::values::typing::KEY_INSERT, "Insert"},
            {MelonDsDs::config::values::typing::KEY_DELETE, "Delete"},
            {MelonDsDs::config::values::typing::KEY_END, "End"},
            {MelonDsDs::config::values::typing::KEY_PAGE_UP, "Page Up"},
            {MelonDsDs::config::values::typing::KEY_PAGE_DOWN, "Page Down"},
            {MelonDsDs::config::values::typing::KEY_PAUSE, "Pause"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::typing::KEY_INSERT
    };

    constexpr retro_core_option_v2_definition TypingArrowsPressDpad {
        config::typing::ARROWS_PRESS_DPAD,
        "Arrow Keys Press the D-Pad",
        nullptr,
        "Have the arrow keys press the D-pad as well. "
        "The game's menus only respond to the D-pad, "
        "even where they ask for the arrow keys.",
        nullptr,
        config::typing::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };
}

#endif //MELONDS_DS_TYPING_HPP
