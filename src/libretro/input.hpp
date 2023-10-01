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

#include "config.hpp"

namespace melonds {
    class ScreenLayoutData;
    extern const struct retro_input_descriptor input_descriptors[];

    class InputState
    {
    public:
        [[nodiscard]] bool CursorVisible() const noexcept;
        [[nodiscard]] bool IsTouchingScreen() const noexcept { return isPointerTouching; }
        [[nodiscard]] bool ScreenTouched() const noexcept { return isPointerTouching && !previousIsPointerTouching; }
        [[nodiscard]] bool ScreenReleased() const noexcept { return !isPointerTouching && previousIsPointerTouching; }
        [[nodiscard]] int TouchX() const noexcept { return pointerTouchPosition.x; }
        [[nodiscard]] int TouchY() const noexcept { return pointerTouchPosition.y; }
        [[nodiscard]] glm::ivec2 PointerTouchPosition() const noexcept { return pointerTouchPosition; }
        [[nodiscard]] glm::i16vec2 PointerInput() const noexcept { return pointerRawPosition; }
        [[nodiscard]] glm::ivec2 HybridTouchPosition() const noexcept { return hybridTouchPosition; }
        [[nodiscard]] bool CycleLayoutDown() const noexcept { return cycleLayoutButton; }
        [[nodiscard]] bool CycleLayoutPressed() const noexcept { return cycleLayoutButton && !previousCycleLayoutButton; }
        [[nodiscard]] bool CycleLayoutReleased() const noexcept { return !cycleLayoutButton && previousCycleLayoutButton; }
        [[nodiscard]] bool MicButtonDown() const noexcept { return micButton; }
        [[nodiscard]] bool MicButtonPressed() const noexcept { return micButton && !previousMicButton; }
        [[nodiscard]] bool MicButtonReleased() const noexcept { return !micButton && previousMicButton; }
        [[nodiscard]] bool ToggleLidDown() const noexcept { return toggleLidButton; }
        [[nodiscard]] bool ToggleLidPressed() const noexcept { return toggleLidButton && !previousToggleLidButton; }
        [[nodiscard]] bool ToggleLidReleased() const noexcept { return !toggleLidButton && previousToggleLidButton; }
        [[nodiscard]] unsigned MaxCursorTimeout() const noexcept { return maxCursorTimeout;}
        [[nodiscard]] uint32_t ConsoleButtons() const noexcept { return consoleButtons; }
        void SetMaxCursorTimeout(unsigned timeout) noexcept {
            if (timeout != maxCursorTimeout) cursorSettingsDirty = true;
            maxCursorTimeout = timeout;
        }
        [[nodiscard]] enum CursorMode CursorMode() const noexcept { return cursorMode; }
        void SetCursorMode(melonds::CursorMode mode) noexcept {
            if (mode != cursorMode) cursorSettingsDirty = true;
            cursorMode = mode;
        }
        void Update(const melonds::ScreenLayoutData& screen_layout_data) noexcept;

    private:
        bool cursorSettingsDirty = true;
        bool isPointerTouching;
        bool previousIsPointerTouching;
        glm::ivec2 previousPointerTouchPosition;
        glm::ivec2 pointerTouchPosition;
        glm::i16vec2 pointerRawPosition;

        /// Touch coordinates of the pointer on the hybrid screen,
        /// in NDS pixel coordinates.
        /// Only relevant if a hybrid layout is active
        glm::ivec2 hybridTouchPosition;
        glm::ivec2 joystickCursorPosition;
        enum CursorMode cursorMode;

        unsigned cursorTimeout = 0;
        unsigned maxCursorTimeout;
        bool toggleLidButton;
        bool previousToggleLidButton;
        bool micButton;
        bool previousMicButton;
        bool cycleLayoutButton;
        bool previousCycleLayoutButton;
        bool joystickTouchButton;
        uint32_t consoleButtons;

    };

    void HandleInput(InputState& inputState, ScreenLayoutData& screenLayout) noexcept;
}
#endif //MELONDS_DS_INPUT_HPP
