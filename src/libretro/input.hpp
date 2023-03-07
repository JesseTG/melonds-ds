//
// Created by Jesse on 3/7/2023.
//

#ifndef MELONDS_DS_INPUT_HPP
#define MELONDS_DS_INPUT_HPP

#include <libretro.h>

namespace melonds {

    extern const struct retro_input_descriptor input_descriptors[];

    enum class TouchMode {
        Disabled,
        Mouse,
        Touch,
        Joystick,
    };

    struct InputState
    {
        bool touching;
        int touch_x, touch_y;
        TouchMode current_touch_mode;

        bool previous_holding_noise_btn = false;
        bool holding_noise_btn = false;
        bool swap_screens_btn = false;
        bool lid_closed = false;
    };

    extern InputState input_state;
}
#endif //MELONDS_DS_INPUT_HPP
