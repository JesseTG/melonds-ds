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

#pragma once

#include <features/features_cpu.h>
#include <libretro.h>
#include <glm/vec2.hpp>

#include "config/types.hpp"
#include "cursor.hpp"
#include "joypad.hpp"
#include "pointer.hpp"
#include "retro/task_queue.hpp"
#include "rumble.hpp"
#include "solar.hpp"
#include "std/chrono.hpp"

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    constexpr const char* const RUMBLE_TASK = "RumbleTask";
    class CoreConfig;
    class ScreenLayoutData;
    class MicrophoneState;
    extern const struct retro_input_descriptor input_descriptors[];

    struct InputPollResult {
        uint32_t JoypadButtons;
        i16vec2 AnalogCursorDirection;
        i16vec2 PointerPosition;
        bool PointerPressed;
        retro_perf_tick_t Timestamp;
    };

    class InputState
    {
    public:
        void SetConfig(const CoreConfig& config) noexcept;
        void Update(const ScreenLayoutData& layout) noexcept;
        void Apply(melonDS::NDS& nds, ScreenLayoutData& layout, MicrophoneState& mic) const noexcept;
        [[nodiscard]] bool CursorVisible() const noexcept { return _cursor.CursorVisible(); }
        [[nodiscard]] bool IsTouching() const noexcept { return _cursor.IsTouching(); }
        [[nodiscard]] bool TouchReleased() const noexcept {
            return _pointer.CursorReleased() || _joypad.TouchReleased();
        }
        [[nodiscard]] ivec2 TouchPosition() const noexcept { return _cursor.TouchPosition(); };
        [[nodiscard]] ivec2 PointerTouchPosition() const noexcept { return _cursor.PointerTouchPosition(); }
        [[nodiscard]] ivec2 JoystickTouchPosition() const noexcept { return _cursor.JoypadTouchPosition(); }
        [[nodiscard]] i16vec2 PointerRawPosition() const noexcept { return _pointer.RawPosition(); }

        void SetControllerPortDevice(unsigned port, unsigned device) noexcept;

        void RumbleStart(std::chrono::milliseconds len) noexcept;
        void RumbleStop() noexcept;
        [[nodiscard]] retro::task::TaskSpec RumbleTask() noexcept {
            return _rumble ? _rumble->RumbleTask() : retro::task::TaskSpec();
        }
    private:
        JoypadState _joypad;
        PointerState _pointer;
        CursorState _cursor;

        unsigned _inputDeviceType;
        enum TouchMode _touchMode;

        // TODO: Consolidate into a std::variant
        std::optional<SolarSensorState> _solarSensor = std::nullopt;
        std::optional<RumbleState> _rumble = std::nullopt;
    };
}
