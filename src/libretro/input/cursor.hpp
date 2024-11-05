/*
    Copyright 2024 Jesse Talavera

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

#pragma once

#include <glm/vec2.hpp>
#include <libretro.h>

#include "config/types.hpp"

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    class CoreConfig;
    class PointerState;
    class JoypadState;
    class ScreenLayoutData;

    class CursorState {
    public:
        void SetConfig(const CoreConfig& config) noexcept;
        void Update(const ScreenLayoutData& layout, const PointerState& pointer, const JoypadState& joypad) noexcept;

        // Gathers the input by the pointer and joystick, and forwards one of them to the NDS
        void Apply(melonDS::NDS& nds) const noexcept;

        [[nodiscard]] unsigned CursorTimeout() const noexcept { return _cursorTimeout; }
        void ResetCursorTimeout() noexcept;

        [[nodiscard]] glm::ivec2 TouchPosition() const noexcept;
        [[nodiscard]] bool IsTouching() const noexcept;
        [[nodiscard]] bool TouchReleased() const noexcept;
    private:
        [[nodiscard]] glm::uvec2 ConsoleTouchPosition(const ScreenLayoutData& layout) const noexcept;

        bool _cursorSettingsDirty = true;
        CursorMode _cursorMode;
        TouchMode _touchMode;
        unsigned _cursorTimeout = 0;
        unsigned _maxCursorTimeout;
        glm::ivec2 _joystickCursorPosition;
        glm::ivec2 _pointerCursorPosition;
        glm::i16vec2 _joystickRawDirection;
        glm::uvec2 _consoleTouchPosition;

        bool _pointerCursorTouching;
        bool _joypadCursorTouching;
        retro_perf_tick_t _joypadCursorLastUpdate;
        retro_perf_tick_t _pointerCursorLastUpdate;

        // ivec2 _previousPointerTouchPosition;
        // ivec2 _pointerTouchPosition;
        /// Touch coordinates of the pointer on the hybrid screen,
        /// in NDS pixel coordinates.
        /// Only relevant if a hybrid layout is active
        glm::ivec2 _hybridTouchPosition;
    };
}