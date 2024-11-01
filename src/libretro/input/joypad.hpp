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

#include "config/types.hpp"

#include <cstdint>

#include <glm/vec2.hpp>
#include <libretro.h>

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    using glm::ivec2;
    using glm::i16vec2;

    class ScreenLayoutData;
    class MicrophoneState;
    class CoreConfig;
    struct InputPollResult;

    class JoypadState {
    public:
        void SetConfig(const CoreConfig& config) noexcept;
        void Poll(const InputPollResult& poll) noexcept;
        void Apply(melonDS::NDS& nds) const noexcept;
        void Apply(ScreenLayoutData& layout) const noexcept;
        void Apply(MicrophoneState& mic) const noexcept;
        void SetControllerPortDevice(unsigned port, unsigned device) noexcept;

        [[nodiscard]] retro_perf_tick_t LastPointerUpdate() const noexcept { return _lastPointerUpdate; }
        [[nodiscard]] bool CycleLayoutPressed() const noexcept { return _cycleLayoutButton && !_previousCycleLayoutButton; }
        [[nodiscard]] bool MicButtonDown() const noexcept { return _micButton; }
        [[nodiscard]] bool MicButtonPressed() const noexcept { return _micButton && !_previousMicButton; }
        [[nodiscard]] bool MicButtonReleased() const noexcept { return !_micButton && _previousMicButton; }

        [[nodiscard]] bool IsTouching() const noexcept { return _joystickTouchButton; }
        [[nodiscard]] bool TouchReleased() const noexcept {
            return !_joystickTouchButton && _previousJoystickTouchButton;
        }
        [[nodiscard]] ivec2 TouchPosition() const noexcept { return _joystickCursorPosition; }
        [[nodiscard]] bool TouchMoved() const noexcept {
            return _joystickCursorPosition != _previousJoystickCursorPosition;
        }

    private:
        bool _toggleLidButton;
        bool _previousToggleLidButton;
        bool _micButton;
        bool _previousMicButton;
        bool _cycleLayoutButton;
        bool _previousCycleLayoutButton;
        bool _joystickTouchButton;
        bool _previousJoystickTouchButton;
        uint32_t _consoleButtons;
        unsigned _device;
        TouchMode _touchMode;
        retro_perf_tick_t _lastPointerUpdate;

        ivec2 _joystickCursorPosition;
        ivec2 _previousJoystickCursorPosition;
        i16vec2 _joystickRawDirection;
    };

}

