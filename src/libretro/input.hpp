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
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

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

        [[nodiscard]] bool cursor_enabled() const;
    };

    extern InputState input_state;

    void update_input(InputState &state);

}
#endif //MELONDS_DS_INPUT_HPP
