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
        void Update(const InputPollResult& poll) noexcept;
        void Apply(melonDS::NDS& nds) const noexcept;
        void Apply(CoreConfig& config) const noexcept;
        void Apply(ScreenLayoutData& layout) const noexcept;
        void Apply(MicrophoneState& mic) const noexcept;
        void SetControllerPortDevice(unsigned port, unsigned device) noexcept;

        [[nodiscard]] bool LightLevelUpPressed() const noexcept {
            return _lightLevelUpCombo && !_previousLightLevelUpCombo;
        }

        [[nodiscard]] bool LightLevelDownPressed() const noexcept {
            return _lightLevelDownCombo && !_previousLightLevelDownCombo;
        }

        [[nodiscard]] retro_perf_tick_t LastPointerUpdate() const noexcept { return _lastPointerUpdate; }
        [[nodiscard]] bool CycleLayoutPressed() const noexcept { return _cycleLayoutButton && !_previousCycleLayoutButton; }
        [[nodiscard]] bool MicButtonDown() const noexcept { return _micButton; }
        [[nodiscard]] bool MicButtonPressed() const noexcept { return _micButton && !_previousMicButton; }
        [[nodiscard]] bool MicButtonReleased() const noexcept { return !_micButton && _previousMicButton; }

        [[nodiscard]] bool IsTouching() const noexcept { return _joystickTouchButton; }
        [[nodiscard]] bool TouchReleased() const noexcept {
            return !_joystickTouchButton && _previousJoystickTouchButton;
        }
        [[nodiscard]] bool CursorMoved() const noexcept {
            return _joystickRawDirection != _previousJoystickRawDirection;
        }

        /// Return true if the joystick cursor was moved, touched, or released
        [[nodiscard]] bool CursorActive() const noexcept {
            return CursorMoved() || (_joystickTouchButton != _previousJoystickTouchButton);
        }

        [[nodiscard]] glm::i16vec2 RawCursorDirection() const noexcept {
            return _joystickRawDirection;
        }

    private:
        bool _toggleLidButton;
        bool _previousToggleLidButton;
        bool _micButton;
        bool _previousMicButton;
        bool _cycleLayoutButton;
        bool _previousCycleLayoutButton;
        bool _joystickSpeedupCursorButton;
        bool _joystickTouchButton;
        bool _previousJoystickTouchButton;
        bool _lightLevelUpCombo;
        bool _previousLightLevelUpCombo;
        bool _lightLevelDownCombo;
        bool _previousLightLevelDownCombo;
        uint32_t _consoleButtons;
        unsigned _device;
        TouchMode _touchMode;
        retro_perf_tick_t _lastPointerUpdate;
        i16vec2 _joystickRawDirection;
        i16vec2 _previousJoystickRawDirection;
    };

}

