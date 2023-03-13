//
// Created by Jesse on 3/7/2023.
//

#include "input.hpp"

#include <algorithm>
#include <NDS.h>
#include "environment.hpp"
#include "utils.hpp"

namespace melonds {

    struct InputState input_state;
    static bool _has_touched = false;
}

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
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L2,     "Make microphone noise"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R2,     "Swap screens"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L3,     "Close lid"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R3,     "Touch joystick"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X,      "Touch joystick X"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,      "Touch joystick Y"},
        {0},
};

#define ADD_KEY_TO_MASK(key, i, bits) \
    do { \
        if (bits & (1 << (key))) {               \
            input_mask &= ~(1 << (i));           \
        }                                      \
        else {                                 \
            input_mask |= (1 << (i));            \
        }                                      \
    } while (false)


void melonds::update_input(InputState &state) {
    uint32_t joypad_bits;
    int i;
    uint32_t input_mask = 0xFFF;

    retro::input_poll();

    if (retro::supports_bitmasks()) {
        joypad_bits = retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
    } else {
        joypad_bits = 0;
        for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3 + 1); i++)
            joypad_bits |= retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
    }

    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_A, 0, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_B, 1, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_SELECT, 2, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_START, 3, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_RIGHT, 4, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_LEFT, 5, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_UP, 6, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_DOWN, 7, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_R, 8, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_L, 9, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_X, 10, joypad_bits);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_Y, 11, joypad_bits);

    NDS::SetKeyMask(input_mask);

    bool lid_closed_btn = retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
    if (lid_closed_btn != state.lid_closed) {
        NDS::SetLidClosed(lid_closed_btn);
        state.lid_closed = lid_closed_btn;
    }

    state.previous_holding_noise_btn = state.holding_noise_btn;
    state.holding_noise_btn = retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
    state.swap_screens_btn = retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

//    if (micNoiseType == MicInput && // If the player wants to use their real mic...
//        noise_button_required && // ...and they don't always want it hot...
//        (state.holding_noise_btn != state.previous_holding_noise_btn) &&
//        // ...and they just pressed or released the button...
//        micHandle != NULL && micInterface.interface_version > 0 // ...and the mic is valid...
//            ) {
//        bool stateSet = micInterface.set_mic_state(micHandle, state.holding_noise_btn);
//        // ...then set the state of the mic to the active staste of the noise button
//
//        if (!stateSet)
//            retro::log(RETRO_LOG_ERROR, "[melonDS] Error setting state of microphone to %s\n",
//                   state.holding_noise_btn ? "enabled" : "disabled");
//    }

    if (current_screen_layout() != ScreenLayout::TopOnly) {
        switch (state.current_touch_mode) {
            case TouchMode::Disabled:
                state.touching = false;
                break;
            case TouchMode::Mouse: {
                int16_t mouse_x = retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
                int16_t mouse_y = retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

                state.touching = retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);

                state.touch_x = std::clamp(state.touch_x + mouse_x, 0, VIDEO_WIDTH - 1);
                state.touch_y = std::clamp(state.touch_y + mouse_y, 0, VIDEO_HEIGHT - 1);
            }

                break;
            case TouchMode::Touch:
                if (retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)) {
                    int16_t pointer_x = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
                    int16_t pointer_y = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

                    unsigned int touch_scale = screen_layout_data.displayed_layout == ScreenLayout::HybridBottom
                                               ? screen_layout_data.hybrid_ratio : 1;

                    unsigned int x =
                            ((int) pointer_x + 0x8000) * screen_layout_data.buffer_width / 0x10000 / touch_scale;
                    unsigned int y =
                            ((int) pointer_y + 0x8000) * screen_layout_data.buffer_height / 0x10000 / touch_scale;

                    if ((x >= screen_layout_data.touch_offset_x) &&
                        (x < screen_layout_data.touch_offset_x + screen_layout_data.screen_width) &&
                        (y >= screen_layout_data.touch_offset_y) &&
                        (y < screen_layout_data.touch_offset_y + screen_layout_data.screen_height)) {
                        state.touching = true;

                        state.touch_x = std::clamp(
                                static_cast<int>((x - screen_layout_data.touch_offset_x) * VIDEO_WIDTH /
                                                 screen_layout_data.screen_width),
                                0, VIDEO_WIDTH - 1);
                        state.touch_y = std::clamp(
                                static_cast<int>((y - screen_layout_data.touch_offset_y) * VIDEO_HEIGHT /
                                                 screen_layout_data.screen_height),
                                0,
                                VIDEO_HEIGHT - 1);
                    }
                } else if (state.touching) {
                    state.touching = false;
                }

                break;
            case TouchMode::Joystick:
                int16_t joystick_x = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                                        RETRO_DEVICE_ID_ANALOG_X) / 2048;
                int16_t joystick_y = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                                        RETRO_DEVICE_ID_ANALOG_Y) / 2048;

                state.touch_x = std::clamp(state.touch_x + joystick_x, 0, melonds::VIDEO_WIDTH - 1);
                state.touch_y = std::clamp(state.touch_y + joystick_y, 0, melonds::VIDEO_HEIGHT - 1);

                state.touching = retro::input_state(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

                break;
        }
    } else {
        state.touching = false;
    }

    if (state.touching) {
        NDS::TouchScreen(state.touch_x, state.touch_y);
        _has_touched = true;
    } else if (_has_touched) {
        NDS::ReleaseScreen();
        _has_touched = false;
    }
}

bool melonds::InputState::cursor_enabled() const {

    return current_touch_mode == TouchMode::Mouse || current_touch_mode == TouchMode::Joystick;
}