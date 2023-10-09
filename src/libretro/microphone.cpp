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

#include "microphone.hpp"

#include "environment.hpp"
#include "tracy.hpp"

#include <optional>

#include <libretro.h>

using std::optional;
using std::nullopt;

namespace retro::microphone {
    static optional<struct retro_microphone_interface> _microphone_interface;
    static retro_microphone_t* _microphone_handle;
}

void retro::microphone::init_interface() noexcept {
    ZoneScopedN("retro::microphone::init_interface");
    if (!_microphone_interface) {
        // If we haven't yet initialized a microphone interface...
        // (retro_environment can be called multiple times)
        struct retro_microphone_interface microphoneInterface;
        microphoneInterface.interface_version = RETRO_MICROPHONE_INTERFACE_VERSION;
        if (environment(RETRO_ENVIRONMENT_GET_MICROPHONE_INTERFACE, &microphoneInterface)) {
            _microphone_interface = microphoneInterface;

            if (microphoneInterface.interface_version == RETRO_MICROPHONE_INTERFACE_VERSION) {
                retro::debug("Microphone support available (version {})\n", microphoneInterface.interface_version);
            } else {
                retro::warn("Expected mic interface version {}, got {}.\n",
                            RETRO_MICROPHONE_INTERFACE_VERSION, microphoneInterface.interface_version);
            }
        } else {
            retro::warn("Microphone interface not available; substituting silence instead.\n");
        }
    }
}

bool retro::microphone::is_interface_available() noexcept
{
    return _microphone_interface.has_value();
}

void retro::microphone::clear_interface() noexcept {
    ZoneScopedN("retro::microphone::clear_interface");
    if (_microphone_interface && _microphone_handle) {
        _microphone_interface->close_mic(_microphone_handle);
    }
    _microphone_interface = nullopt;
    _microphone_handle = nullptr;
}

bool retro::microphone::set_open(bool open) noexcept {
    ZoneScopedN("retro::microphone::set_open");
    if (!_microphone_interface) {
        // If we don't have microphone support available...
        return false;
    }

    if (open) {
        // If we want a microphone...
        if (_microphone_handle) {
            // If we already have an open microphone...
            return true;
        }

        retro_microphone_params_t params = {
            .rate = 44100, // melonDS expects this rate
        };
        _microphone_handle = _microphone_interface->open_mic(&params);
        return _microphone_handle != nullptr;
    }
    else {
        // If we want to close the microphone...
        if (!_microphone_handle) {
            // If we don't have an open microphone...
            return true; // Good; we want it closed anyway
        }

        _microphone_interface->close_mic(_microphone_handle);
        _microphone_handle = nullptr;

        return true;
    }
}

bool retro::microphone::is_open() noexcept {
    return _microphone_interface && _microphone_handle;
}

bool retro::microphone::set_state(bool on) noexcept {
    if (!_microphone_interface || !_microphone_handle) {
        // If we don't have microphone support available...
        return false; // Can't set the state
    }

    if (_microphone_interface->get_mic_state(_microphone_handle) == on) {
        // If the microphone is already in the desired state...
        return true; // Good; we want it in that state anyway
    }

    return _microphone_interface->set_mic_state(_microphone_handle, on);
}
optional<bool> retro::microphone::get_state() noexcept
{
    if (!_microphone_interface || !_microphone_handle) {
        // If we don't have microphone support available...
        return nullopt; // Can't get the state
    }

    return _microphone_interface->get_mic_state(_microphone_handle);
}

optional<int> retro::microphone::read(int16_t* samples, size_t num_samples) noexcept
{
    ZoneScopedN("retro::microphone::read");
    if (!_microphone_interface || !_microphone_handle) {
        // If we don't have microphone support available...
        return nullopt; // Can't read
    }

    return _microphone_interface->read_mic(_microphone_handle, samples, num_samples);
}
