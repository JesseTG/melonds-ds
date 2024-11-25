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

#include <cstdint>
#include <optional>

#include "config/types.hpp"

namespace melonDS {
    class NDS;
};

namespace MelonDsDs {
    class CoreConfig;
    class JoypadState;

    class SolarSensorState {
    public:
        explicit SolarSensorState(unsigned port) noexcept;
        ~SolarSensorState() noexcept;
        SolarSensorState(const SolarSensorState&) = delete;
        SolarSensorState& operator=(const SolarSensorState&) = delete;
        SolarSensorState(SolarSensorState&&) noexcept;
        SolarSensorState& operator=(SolarSensorState&&) noexcept;

        void Update(const JoypadState& joypad) noexcept;
        void SetConfig(const CoreConfig& config) noexcept;
        void Apply(melonDS::NDS& nds) const noexcept;

        [[nodiscard]] std::optional<float> LuxReading() const noexcept { return _lux; }
    private:
        enum class InterfaceState : uint8_t {
            Off,
            Unavailable,
            Deferred,
            On,
        };
        unsigned int _port;
        std::optional<float> _lux;
        bool _buttonUp = false;
        bool _buttonDown = false;
        InterfaceState _state = InterfaceState::Off;
    };
}