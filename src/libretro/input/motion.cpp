/*
    Copyright 2026 Jesse Talavera

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

#include "motion.hpp"

#include "../constants.hpp"
#include "config/config.hpp"
#include "environment.hpp"
#include "tracy/client.hpp"

using MelonDsDs::MotionState;

/// How often the frontend should update its motion sensors, in Hz.
/// There's no point in sampling them faster than the emulated console runs.
static constexpr unsigned SENSOR_RATE = static_cast<unsigned>(MelonDsDs::FPS + 0.5);

MotionState::MotionState(unsigned port, bool hasGyroscope) noexcept : _port(port) {
    // If the frontend isn't ready for sensor requests yet
    // (e.g. because we're still in retro_load_game),
    // Update will try again.
    _accelerometer = Enable(RETRO_SENSOR_ACCELEROMETER_ENABLE, "accelerometer", InterfaceState::Deferred);

    if (hasGyroscope) {
        _gyroscope = Enable(RETRO_SENSOR_GYROSCOPE_ENABLE, "gyroscope", InterfaceState::Deferred);
    }
}

MotionState::~MotionState() noexcept {
    Disable(_accelerometer, RETRO_SENSOR_ACCELEROMETER_DISABLE, "accelerometer");
    Disable(_gyroscope, RETRO_SENSOR_GYROSCOPE_DISABLE, "gyroscope");
}

MotionState::MotionState(MotionState&& other) noexcept :
    _port(other._port),
    _accelerometer(other._accelerometer),
    _gyroscope(other._gyroscope),
    _showWarnings(other._showWarnings),
    _acceleration(other._acceleration),
    _rotation(other._rotation) {
    // Only one Motion Pak can be in Slot-2 at a time,
    // so only one MotionState may own the frontend's sensors.
    other._accelerometer = InterfaceState::Off;
    other._gyroscope = InterfaceState::Off;
}

MotionState& MotionState::operator=(MotionState&& other) noexcept {
    if (this != &other) {
        // Release the sensors that the incoming state won't be using,
        // but not the ones it's already switched on for itself;
        // otherwise, replacing one Motion Pak with another would turn them off for good.
        if (_port != other._port || other._accelerometer != InterfaceState::On) {
            Disable(_accelerometer, RETRO_SENSOR_ACCELEROMETER_DISABLE, "accelerometer");
        }

        if (_port != other._port || other._gyroscope != InterfaceState::On) {
            Disable(_gyroscope, RETRO_SENSOR_GYROSCOPE_DISABLE, "gyroscope");
        }

        _port = other._port;
        _accelerometer = other._accelerometer;
        _gyroscope = other._gyroscope;
        _showWarnings = other._showWarnings;
        _acceleration = other._acceleration;
        _rotation = other._rotation;
        other._accelerometer = InterfaceState::Off;
        other._gyroscope = InterfaceState::Off;
    }

    return *this;
}

void MotionState::SetConfig(const CoreConfig& config) noexcept {
    _showWarnings = config.ShowUnsupportedFeatureWarnings();
}

void MotionState::Update() noexcept {
    ZoneScopedN(TracyFunction);

    if (_accelerometer == InterfaceState::Deferred) [[unlikely]] {
        _accelerometer = Enable(RETRO_SENSOR_ACCELEROMETER_ENABLE, "accelerometer", InterfaceState::Unavailable);

        if (_accelerometer == InterfaceState::Unavailable && _showWarnings) {
            retro::set_warn_message(
                "Can't find this device's motion sensors, "
                "so the Motion Pak will act as if it's lying flat."
            );
        }
    }

    if (_gyroscope == InterfaceState::Deferred) [[unlikely]] {
        // The homebrew Motion Pak is still usable without its gyroscope,
        // so this doesn't merit a warning.
        _gyroscope = Enable(RETRO_SENSOR_GYROSCOPE_ENABLE, "gyroscope", InterfaceState::Unavailable);
    }

    if (_accelerometer == InterfaceState::On) {
        _acceleration = {
            retro::sensor_get_input(_port, RETRO_SENSOR_ACCELEROMETER_X).value_or(0.0f),
            retro::sensor_get_input(_port, RETRO_SENSOR_ACCELEROMETER_Y).value_or(0.0f),
            retro::sensor_get_input(_port, RETRO_SENSOR_ACCELEROMETER_Z).value_or(1.0f),
        };
    }

    if (_gyroscope == InterfaceState::On) {
        _rotation = {
            retro::sensor_get_input(_port, RETRO_SENSOR_GYROSCOPE_X).value_or(0.0f),
            retro::sensor_get_input(_port, RETRO_SENSOR_GYROSCOPE_Y).value_or(0.0f),
            retro::sensor_get_input(_port, RETRO_SENSOR_GYROSCOPE_Z).value_or(0.0f),
        };
    }
}

float MotionState::Query(melonDS::Platform::MotionQueryType type) const noexcept {
    switch (type) {
        case melonDS::Platform::MotionAccelerationX:
            return _acceleration.x * STANDARD_GRAVITY;
        case melonDS::Platform::MotionAccelerationY:
            return _acceleration.y * STANDARD_GRAVITY;
        case melonDS::Platform::MotionAccelerationZ:
            return _acceleration.z * STANDARD_GRAVITY;
        case melonDS::Platform::MotionRotationX:
            return _rotation.x;
        case melonDS::Platform::MotionRotationY:
            return _rotation.y;
        case melonDS::Platform::MotionRotationZ:
            return _rotation.z;
        default:
            return Resting(type);
    }
}

float MotionState::Resting(melonDS::Platform::MotionQueryType type) noexcept {
    return type == melonDS::Platform::MotionAccelerationZ ? STANDARD_GRAVITY : 0.0f;
}

MotionState::InterfaceState MotionState::Enable(retro_sensor_action action, const char* name, InterfaceState failure) const noexcept {
    if (retro::set_sensor_state(_port, action, SENSOR_RATE)) {
        retro::debug("Enabled host {} at port {}", name, _port);
        return InterfaceState::On;
    }

    if (failure == InterfaceState::Deferred) {
        retro::warn("Failed to enable host {} at port {}; deferring once", name, _port);
    }
    else {
        retro::warn("Failed to enable host {} at port {}; it will read as if lying flat and still", name, _port);
    }

    return failure;
}

void MotionState::Disable(InterfaceState& state, retro_sensor_action action, const char* name) const noexcept {
    if (state == InterfaceState::On) {
        if (retro::set_sensor_state(_port, action, 0)) {
            retro::debug("Disabled host {} at port {}", name, _port);
        }
        else {
            retro::warn("Failed to disable host {} at port {}", name, _port);
        }
    }

    state = InterfaceState::Off;
}
