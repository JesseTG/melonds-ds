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

#include "input.hpp"

#include <algorithm>
#include <retro_miscellaneous.h>
#include <NDS.h>
#include <glm/ext/vector_common.hpp>

#include "config.hpp"
#include "environment.hpp"
#include "libretro.hpp"
#include "screenlayout.hpp"
#include "utils.hpp"

using glm::ivec2;

const struct retro_input_descriptor melonds::input_descriptors[] = {
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_UP,     "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_A,      "A"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_B,      "B"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_START,  "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R,      "R"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L,      "L"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_X,      "X"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_Y,      "Y"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L2,     "Microphone"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R2,     "Next Screen Layout"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L3,     "Close lid"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R3,     "Touch joystick"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X,      "Touch joystick X"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,      "Touch joystick Y"},
        {},
};

static const char *device_name(unsigned device) {
    switch (device) {
        case RETRO_DEVICE_NONE:
            return "RETRO_DEVICE_NONE";
        case RETRO_DEVICE_JOYPAD:
            return "RETRO_DEVICE_JOYPAD";
        case RETRO_DEVICE_MOUSE:
            return "RETRO_DEVICE_MOUSE";
        case RETRO_DEVICE_KEYBOARD:
            return "RETRO_DEVICE_KEYBOARD";
        case RETRO_DEVICE_LIGHTGUN:
            return "RETRO_DEVICE_LIGHTGUN";
        case RETRO_DEVICE_ANALOG:
            return "RETRO_DEVICE_ANALOG";
        case RETRO_DEVICE_POINTER:
            return "RETRO_DEVICE_POINTER";
        default:
            return "<unknown>";
    }
}

// Not really needed, but libretro requires all retro_* functions to be defined
PUBLIC_SYMBOL void retro_set_controller_port_device(unsigned port, unsigned device) {
    retro::log(RETRO_LOG_DEBUG, "retro_set_controller_port_device(%d, %s)", port, device_name(device));
}

#define ADD_KEY_TO_MASK(key, i, bits) \
    do { \
        if (bits & (1 << (key))) {               \
            ndsInputBits &= ~(1 << (i));           \
        }                                      \
        else {                                 \
            ndsInputBits |= (1 << (i));            \
        }                                      \
    } while (false)

// TODO: Refactor Update to only read global state;
// apply it in a separate function
void melonds::InputState::Update(const ScreenLayoutData& screen_layout_data) noexcept {
    uint32_t retroInputBits; // Input bits from libretro
    uint32_t ndsInputBits = 0xFFF; // Input bits passed to the emulated DS

    retro::input_poll();

    if (retro::supports_bitmasks()) {
        retroInputBits = retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
    } else {
        retroInputBits = 0;
        for (int i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3 + 1); i++)
            retroInputBits |= retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
    }

    // Delicate bit manipulation; do not touch!
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_A, 0, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_B, 1, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_SELECT, 2, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_START, 3, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_RIGHT, 4, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_LEFT, 5, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_UP, 6, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_DOWN, 7, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_R, 8, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_L, 9, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_X, 10, retroInputBits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_Y, 11, retroInputBits);

    NDS::SetKeyMask(ndsInputBits);

    previousToggleLidButton = toggleLidButton;
    toggleLidButton = retroInputBits & (1 << RETRO_DEVICE_ID_JOYPAD_L3);

    previousMicButton = micButton;
    micButton = retroInputBits & (1 << RETRO_DEVICE_ID_JOYPAD_L2);

    previousCycleLayoutButton = cycleLayoutButton;
    cycleLayoutButton = retroInputBits & (1 << RETRO_DEVICE_ID_JOYPAD_R2);

    previousTouching = touching;
    // TODO: Touching should be disabled when the lid is closed
    if (screen_layout_data.Layout() != ScreenLayout::TopOnly) {
        switch (config::screen::TouchMode()) {
            case TouchMode::Disabled:
                touching = false;
                break;
            case TouchMode::Mouse: {
                int16_t mouse_dx = retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
                int16_t mouse_dy = retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

                touching = retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);

                touch.x = std::clamp(touch.x + mouse_dx, 0, NDS_SCREEN_WIDTH - 1);
                touch.y = std::clamp(touch.y + mouse_dy, 0, NDS_SCREEN_HEIGHT - 1);
                break;
            }
            case TouchMode::Touch: {
                bool pointerPressed = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
                if (pointerPressed) {
                    int16_t pointer_x = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
                    int16_t pointer_y = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

                    unsigned int touch_scale = screen_layout_data.Layout() == ScreenLayout::HybridBottom
                                               ? screen_layout_data.HybridRatio() : 1;

                    unsigned int x =
                            ((int) pointer_x + 0x8000) * screen_layout_data.BufferWidth() / 0x10000 / touch_scale;
                    unsigned int y =
                            ((int) pointer_y + 0x8000) * screen_layout_data.BufferHeight() / 0x10000 / touch_scale;

                    if ((x >= screen_layout_data.TouchOffsetX()) &&
                        (x < screen_layout_data.TouchOffsetX() + screen_layout_data.ScreenWidth()) &&
                        (y >= screen_layout_data.TouchOffsetY()) &&
                        (y < screen_layout_data.TouchOffsetY() + screen_layout_data.ScreenHeight())) {
                        touching = true;

                        touch.x = std::clamp(
                            static_cast<int>((x - screen_layout_data.TouchOffsetX()) * NDS_SCREEN_WIDTH /
                                             screen_layout_data.ScreenWidth()),
                            0, NDS_SCREEN_WIDTH - 1);
                        touch.y = std::clamp(
                            static_cast<int>((y - screen_layout_data.TouchOffsetY()) * NDS_SCREEN_HEIGHT /
                                             screen_layout_data.ScreenHeight()),
                            0,
                            NDS_SCREEN_HEIGHT - 1);
                    }
                } else {
                    touching = false;
                }

                break;
            }
            case TouchMode::Joystick: {
                int16_t joystick_x = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                                        RETRO_DEVICE_ID_ANALOG_X) / 2048;
                int16_t joystick_y = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                                        RETRO_DEVICE_ID_ANALOG_Y) / 2048;

                touch.x = std::clamp(touch.x + joystick_x, 0, melonds::NDS_SCREEN_WIDTH - 1);
                touch.y = std::clamp(touch.y + joystick_y, 0, melonds::NDS_SCREEN_HEIGHT - 1);

                touching = retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

                break;
            }

        }
    } else {
        touching = false;
    }

    // TODO: Should the input object state modify global state?
    if (IsTouchingScreen()) {
        NDS::TouchScreen(touch.x, touch.y);
    } else if (ScreenReleased()) {
        NDS::ReleaseScreen();
    }

    if (ToggleLidPressed()) {
        NDS::SetLidClosed(!NDS::IsLidClosed());
        retro::log(RETRO_LOG_DEBUG, "%s the lid", NDS::IsLidClosed() ? "Closed" : "Opened");
    }
}

bool melonds::InputState::CursorEnabled() const noexcept {
    return config::screen::TouchMode() == TouchMode::Mouse || config::screen::TouchMode() == TouchMode::Joystick;
}