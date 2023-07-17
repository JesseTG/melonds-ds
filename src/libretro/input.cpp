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
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/ext/vector_common.hpp>
#include <glm/gtx/common.hpp>

#include "config.hpp"
#include "environment.hpp"
#include "libretro.hpp"
#include "math.hpp"
#include "screenlayout.hpp"
#include "utils.hpp"

using glm::ivec2;
using glm::ivec3;
using glm::mat3;
using glm::vec2;
using glm::uvec2;

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
    // TODO: Get touch input from the joystick regardless of the screen layout
    // TODO: If touching the screen, prioritize the pointer
    if (screen_layout_data.Layout() != ScreenLayout::TopOnly) {
        touching = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
        int16_t pointer_x = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
        int16_t pointer_y = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

        int16_t joystick_x = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
        int16_t joystick_y = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
        // TODO: Provide an option to allow the joystick input to stay relative to the screen

        mat3 joystickMatrix = melonds::math::ts(NDS_SCREEN_SIZE<float> / 2.0f, NDS_SCREEN_SIZE<float> / 65535.0f);
        ivec2 transformed_pointer = screen_layout_data.TransformPointerInput(pointer_x, pointer_y);
        ivec2 transformedHybridPointer = screen_layout_data.TransformPointerInputToHybridScreen(pointer_x, pointer_y);
        ivec2 transformed_joystick = joystickMatrix * ivec3(joystick_x, joystick_y, 1);

        char text[1024];
        sprintf(text, "Pointer: (%d, %d) -> (%d, %d)", pointer_x, pointer_y, transformed_pointer.x, transformed_pointer.y);
        retro_message_ext message {
            .msg = text,
            .duration = 60,
            .priority = 0,
            .level = RETRO_LOG_DEBUG,
            .target = RETRO_MESSAGE_TARGET_OSD,
            .type = RETRO_MESSAGE_TYPE_STATUS,
            .progress = -1
        };
        retro::set_message(&message);

        touch = transformed_pointer;
        hybridTouch = transformedHybridPointer;
    } else {
        touching = false;
    }

    // TODO: Should the input object state modify global state?
    if (IsTouchingScreen()) {
        uvec2 clampedTouch;
        if (screen_layout_data.Layout() == ScreenLayout::HybridBottom && !glm::all(glm::openBounded(uvec2(touch), uvec2(0), NDS_SCREEN_SIZE<unsigned>))) {
            clampedTouch = glm::clamp(uvec2(hybridTouch), uvec2(0), NDS_SCREEN_SIZE<unsigned> - 1u);
        } else {
            clampedTouch = glm::clamp(uvec2(touch), uvec2(0), NDS_SCREEN_SIZE<unsigned> - 1u);
        }

        NDS::TouchScreen(clampedTouch.x, clampedTouch.y);
    } else if (ScreenReleased()) {
        NDS::ReleaseScreen();
    }

    if (ToggleLidPressed()) {
        NDS::SetLidClosed(!NDS::IsLidClosed());
        retro::log(RETRO_LOG_DEBUG, "%s the lid", NDS::IsLidClosed() ? "Closed" : "Opened");
    }
}

bool melonds::InputState::CursorEnabled() const noexcept {
    return true;
}