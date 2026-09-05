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

#include <variant>

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
#include "utils.hpp"

namespace melonDS {
    class NDS;
    namespace GBACart {
        class CartCommon;
    }
}

namespace MelonDsDs {
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

    using Slot2State = std::variant<std::monostate, SolarSensorState, RumbleState>;

    class InputState
    {
    public:
        void SetConfig(const CoreConfig& config) noexcept;
        void Update(const CoreConfig& config, const ScreenLayoutData& layout) noexcept;
        /// Chooses the Slot-2 input device to emulate;
        /// pass \c nullptr when Slot-2 is empty.
        void SetSlot2Input(const melonDS::GBACart::CartCommon* gbacart) noexcept;
        void Apply(melonDS::NDS& nds, ScreenLayoutData& layout, MicrophoneState& mic, CoreConfig& config) const noexcept;
        /// Applies only the screen layout hotkey, for when there's no console to drive
        /// (such as on the error screen).
        void Apply(ScreenLayoutData& layout) const noexcept;
        [[nodiscard]] bool CursorVisible() const noexcept { return _cursor.CursorVisible(); }
        [[nodiscard]] bool IsTouching() const noexcept { return _cursor.IsTouching(); }
        [[nodiscard]] bool TouchReleased() const noexcept {
            return _pointer.CursorReleased() || _joypad.TouchReleased();
        }
        [[nodiscard]] ivec2 TouchPosition() const noexcept { return _cursor.TouchPosition(); };
        [[nodiscard]] ivec2 ConsoleTouchPosition() const noexcept { return _cursor.ConsoleTouchPosition(); }
        [[nodiscard]] ivec2 PointerTouchPosition() const noexcept { return _cursor.PointerTouchPosition(); }
        [[nodiscard]] ivec2 JoystickTouchPosition() const noexcept { return _cursor.JoypadTouchPosition(); }
        [[nodiscard]] i16vec2 PointerRawPosition() const noexcept { return _pointer.RawPosition(); }

        void SetControllerPortDevice(unsigned port, unsigned device) noexcept;
        [[nodiscard]] unsigned GetControllerPortDevice(unsigned port) const noexcept {
            // We may use port at some point, but not now
            return _inputDeviceType;
        }

        [[nodiscard]] std::optional<float> LuxReading() const noexcept {
            if (const auto* solar = std::get_if<SolarSensorState>(&_slot2)) {
                return solar->LuxReading();
            }

            return std::nullopt;
        }

        void RumbleStart() noexcept;
        void RumbleStop() noexcept;

        /// Folds this frame's Rumble Pak activity into the frontend's motors.
        void UpdateRumble() noexcept;

        /// Switches the frontend's motors off, if a Rumble Pak is driving them.
        void StopRumble() noexcept;

        /// The strength most recently computed for the motors,
        /// or -1 if there's no Rumble Pak in Slot-2.
        [[nodiscard]] int32_t RumbleLevel() const noexcept;

        /// The number of Rumble Pak register toggles counted in the most recent frame,
        /// or -1 if there's no Rumble Pak in Slot-2.
        [[nodiscard]] int32_t RumbleEdges() const noexcept;
    private:
        JoypadState _joypad;
        PointerState _pointer;
        CursorState _cursor;

        unsigned _inputDeviceType;
        enum TouchMode _touchMode;

        Slot2State _slot2;
    };
}
