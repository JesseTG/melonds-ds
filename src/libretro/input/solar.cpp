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
#include <retro_assert.h>

#include "config/config.hpp"
#include "environment.hpp"
#include "joypad.hpp"
#include "tracy/client.hpp"

using MelonDsDs::SolarSensorState;

SolarSensorState::SolarSensorState(unsigned port) noexcept : _port(port) {
}

SolarSensorState::~SolarSensorState() noexcept {
    if (_state == InterfaceState::On) {
        retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_DISABLE, 0);
        retro::debug("Disabled host illuminance sensor at port {}", _port);
    }
}

SolarSensorState& SolarSensorState::operator=(SolarSensorState&& other) noexcept {
    if (this != &other) {
        if (_port != other._port) {
            // If we're assigning a new port to this state...
            retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_DISABLE, 0);
            retro::debug("Disabled host illuminance sensor at port {}", _port);
        }
        _port = other._port;
        _lux = other._lux;
        _buttonUp = other._buttonUp;
        _buttonDown = other._buttonDown;
        _state = other._state;
        other._state = InterfaceState::Off;
        other._lux = std::nullopt;
    }

    return *this;
}

SolarSensorState::SolarSensorState(SolarSensorState&& other) noexcept :
    _port(other._port),
    _lux(other._lux),
    _buttonUp(other._buttonUp),
    _buttonDown(other._buttonDown),
    _state(other._state) {
    // Solar sensor activation state is managed RAII-like
    // (but there's no actual resource here)
    other._state = InterfaceState::Off;
    other._lux = std::nullopt;
}

void SolarSensorState::Update(const JoypadState& joypad) noexcept {
    ZoneScopedN(TracyFunction);
    if (_state != InterfaceState::On) {
        // If we're not using the real light sensor...
        _buttonUp = joypad.LightLevelUpPressed() || (retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP) != 0);
        _buttonDown = joypad.LightLevelDownPressed() || (retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN) != 0);
    }
    else {
        _buttonUp = false;
        _buttonDown = false;
    }

    if (_state == InterfaceState::Deferred) [[unlikely]] {
        // If the sensor interface wasn't ready in retro_load_game...
      if (retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_ENABLE, 0)) {
          retro::debug("Initialized sensor interface in port {} after deferral", _port);
          _state = InterfaceState::On;
      }
      else {
          retro::warn("Failed to enable host illuminance sensor at port {}", _port);
          retro::set_warn_message("Can't find this device's luminance sensor. See the core options for more info.");
          _state = InterfaceState::Unavailable;
      }
    }

    if (_state == InterfaceState::On) {
        // If we're using the illuminance sensor...
        _lux = retro::sensor_get_input(0, RETRO_SENSOR_ILLUMINANCE);
#ifdef HAVE_TRACY
        if (_lux) {
            TracyPlot("Illuminance Reading", *_lux);
        }
#endif
    }
    else {
        _lux = std::nullopt;
    }
}


void SolarSensorState::SetConfig(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    bool useRealSensor = config.UseRealLightSensor();
    if (useRealSensor) {
        // If we're using the host's luminance sensor...
        if (retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_ENABLE, 0)) {
            retro::debug("Enabled host illuminance sensor at port {}", _port);
            _state = InterfaceState::On;
        }
        else {
            retro::warn("Failed to enable host illuminance sensor at port {}; deferring once", _port);
            _state = InterfaceState::Deferred;
        }
    }
    else {
        if (retro::set_sensor_state(_port, RETRO_SENSOR_ILLUMINANCE_DISABLE, 0)) {
            // If we disabled the illuminance sensor...
            retro::debug("Disabled host illuminance sensor at port {}", _port);
        }
        else {
            retro::warn("Failed to disable host illuminance sensor at port {}", _port);
        }
        _state = InterfaceState::Off;
    }
}

void SolarSensorState::Apply(melonDS::NDS& nds) const noexcept {
    auto* gbacart = nds.GetGBACart();
    if (!gbacart || gbacart->Type() != melonDS::GBACart::CartType::GameSolarSensor)
        // If a photosensor-enabled GBA game isn't inserted...
        return;

    auto* solarcart = static_cast<melonDS::GBACart::CartGameSolarSensor*>(gbacart);

    if (_lux) {
        // If we could read the illuminance sensor...
        // Taken from the mgba core's use of the light sensor
        // (I don't actually know how this math works)
        uint8_t lightLevel = static_cast<uint8_t>(cbrtf(*_lux) * 8);
        TracyPlot("Solar Sensor Light Level", static_cast<int64_t>(lightLevel));
        solarcart->SetLightLevel(lightLevel);
    }
    else {
        if (_buttonUp) {
            solarcart->SetInput(melonDS::GBACart::Input_SolarSensorUp, true);
        }
        if (_buttonDown) {
            solarcart->SetInput(melonDS::GBACart::Input_SolarSensorDown, true);
        }
    }
}