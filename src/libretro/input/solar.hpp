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
        static std::optional<SolarSensorState> New(unsigned port) noexcept;
        ~SolarSensorState() noexcept;
        SolarSensorState(const SolarSensorState&) = delete;
        SolarSensorState& operator=(const SolarSensorState&) = delete;
        SolarSensorState(SolarSensorState&&) = delete;
        SolarSensorState& operator=(SolarSensorState&&) = delete;

        void Update(const JoypadState& joypad) noexcept;
        void SetConfig(const CoreConfig& config) noexcept;
        void Apply(melonDS::NDS& nds) noexcept;
    private:
        unsigned int _port;
        SolarSensorInputType _type;
        bool _valid = true;
    };
}