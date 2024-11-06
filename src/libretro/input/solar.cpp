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

#include "solar.hpp"

#include <NDS.h>

#include "config/config.hpp"
#include "environment.hpp"
#include "joypad.hpp"
#include "tracy/client.hpp"

using MelonDsDs::SolarSensorState;

std::optional<SolarSensorState> SolarSensorState::New(unsigned port) noexcept {
    if (retro::set_sensor_state(port, RETRO_SENSOR_ILLUMINANCE_ENABLE, 0.0f)) {
        // Initialize the device's solar sensor. If that happened successfully...
        return std::make_optional<SolarSensorState>(port);
    }

    return std::nullopt;
}

SolarSensorState::SolarSensorState(unsigned port) noexcept : _port(port) {
    retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_ENABLE, 0.0f);
}

SolarSensorState::~SolarSensorState() noexcept {
    if (_valid) {
        retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_DISABLE, 0.0f);
    }
}

SolarSensorState& SolarSensorState::operator=(SolarSensorState&& other) noexcept {
    if (this != &other) {
        if (_port != other._port) {
            // If we're assigning a new port to this state...
            retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_DISABLE, 0.0f);
        }
        _port = other._port;
        _type = other._type;
        _valid = other._valid;
        other._valid = false;
    }

    return *this;
}

SolarSensorState::SolarSensorState(SolarSensorState&& other) noexcept: _port(other._port), _type(other._type), _valid(other._valid) {
    // Solar sensor activation state is managed RAII-like
    // (but there's no actual resource here)
    other._valid = false;
}

void SolarSensorState::Update(const JoypadState& joypad) noexcept {
    // TODO: Fetch the solar sensor reading
    // TODO: Check if the solar sensor buttons are pressed
}


void SolarSensorState::SetConfig(const CoreConfig& config) noexcept {
    // TODO: Configure where the solar sensor input will come from
}

void SolarSensorState::Apply(melonDS::NDS& nds) noexcept {
    auto* gbacart = nds.GetGBACart();
    if (!gbacart || gbacart->Type() != melonDS::GBACart::CartType::GameSolarSensor)
        return;

    // TODO: Am I using the sensor, the buttons, or a fixed value?

    if (std::optional<float> lux = retro::sensor_get_input(0, RETRO_SENSOR_ILLUMINANCE); lux) {
        TracyPlot("Solar Sensor Reading", *lux);
        if (auto* gbacart = nds.GetGBACart(); gbacart && gbacart->Type() == melonDS::GBACart::CartType::GameSolarSensor) {
            // If a photosensor-enabled game is inserted in the GBA slot...
            auto* solarcart = static_cast<melonDS::GBACart::CartGameSolarSensor*>(gbacart);
        }
    }
}