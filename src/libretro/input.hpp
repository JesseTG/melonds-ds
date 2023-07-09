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

#ifndef MELONDS_DS_INPUT_HPP
#define MELONDS_DS_INPUT_HPP

#include <libretro.h>

namespace melonds {
    class ScreenLayoutData;
    extern const struct retro_input_descriptor input_descriptors[];

    class InputState
    {
    public:
        [[nodiscard]] bool CursorEnabled() const noexcept;
        [[nodiscard]] bool IsTouchingScreen() const noexcept { return touching; }
        [[nodiscard]] int TouchX() const noexcept { return touch_x; }
        [[nodiscard]] int TouchY() const noexcept { return touch_y; }
        [[nodiscard]] bool SwapScreenPressed() const noexcept { return swap_screens_btn; }
        [[nodiscard]] bool MicButtonPressed() const noexcept { return holding_noise_btn; }
        [[nodiscard]] bool MicButtonJustPressed() const noexcept { return holding_noise_btn && !previous_holding_noise_btn; }
        [[nodiscard]] bool MicButtonJustReleased() const noexcept { return !holding_noise_btn && previous_holding_noise_btn; }
        [[nodiscard]] bool LidClosed() const noexcept { return lid_closed; }
        void Update(const melonds::ScreenLayoutData& screen_layout_data) noexcept;

    private:
        bool touching;
        int touch_x, touch_y;

        bool previous_holding_noise_btn = false;
        bool holding_noise_btn = false;
        bool swap_screens_btn = false;
        bool lid_closed = false;
        bool _has_touched = false;

    };
}
#endif //MELONDS_DS_INPUT_HPP
