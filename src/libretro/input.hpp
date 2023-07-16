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
#include <glm/vec2.hpp>

namespace melonds {
    class ScreenLayoutData;
    extern const struct retro_input_descriptor input_descriptors[];

    class InputState
    {
    public:
        [[nodiscard]] bool CursorEnabled() const noexcept;
        [[nodiscard]] bool IsTouchingScreen() const noexcept { return touching; }
        [[nodiscard]] bool ScreenTouched() const noexcept { return touching && !previousTouching; }
        [[nodiscard]] bool ScreenReleased() const noexcept { return !touching && previousTouching; }
        [[nodiscard]] int TouchX() const noexcept { return touch.x; }
        [[nodiscard]] int TouchY() const noexcept { return touch.y; }
        [[nodiscard]] glm::ivec2 TouchPosition() const noexcept { return touch; }
        [[nodiscard]] bool CycleLayoutDown() const noexcept { return cycleLayoutButton; }
        [[nodiscard]] bool CycleLayoutPressed() const noexcept { return cycleLayoutButton && !previousCycleLayoutButton; }
        [[nodiscard]] bool CycleLayoutReleased() const noexcept { return !cycleLayoutButton && previousCycleLayoutButton; }
        [[nodiscard]] bool MicButtonDown() const noexcept { return micButton; }
        [[nodiscard]] bool MicButtonPressed() const noexcept { return micButton && !previousMicButton; }
        [[nodiscard]] bool MicButtonReleased() const noexcept { return !micButton && previousMicButton; }
        [[nodiscard]] bool ToggleLidDown() const noexcept { return toggleLidButton; }
        [[nodiscard]] bool ToggleLidPressed() const noexcept { return toggleLidButton && !previousToggleLidButton; }
        [[nodiscard]] bool ToggleLidReleased() const noexcept { return !toggleLidButton && previousToggleLidButton; }
        void Update(const melonds::ScreenLayoutData& screen_layout_data) noexcept;

    private:
        bool touching;
        bool previousTouching;
        glm::ivec2 touch;
        glm::ivec2 joystickTouch;

        bool toggleLidButton;
        bool previousToggleLidButton;
        bool previousMicButton;
        bool micButton;
        bool cycleLayoutButton;
        bool previousCycleLayoutButton;

    };
}
#endif //MELONDS_DS_INPUT_HPP
