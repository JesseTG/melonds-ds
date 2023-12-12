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

#include <optional>

#include <libretro.h>

#include "config/config.hpp"
#include "environment.hpp"
#include "input.hpp"
#include "tracy.hpp"

using std::optional;
using std::nullopt;

MelonDsDs::MicrophoneState::MicrophoneState() noexcept :
    _micInterface(retro::get_microphone_interface()) {
    if (_micInterface) {
        if (_micInterface->interface_version == RETRO_MICROPHONE_INTERFACE_VERSION) {
            retro::debug("Microphone support available (version {})\n", _micInterface->interface_version);
        }
        else {
            retro::warn("Expected mic interface version {}, got {}.\n", RETRO_MICROPHONE_INTERFACE_VERSION, _micInterface->interface_version);
        }
    }
    else {
        retro::warn("Microphone interface not available; substituting silence instead.\n");
    }
}

void MelonDsDs::MicrophoneState::Apply(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);

    SetMicInputMode(config.MicInputMode());
    SetMicButtonMode(config.MicButtonMode());
}

void MelonDsDs::MicrophoneState::SetMicInputMode(MicInputMode mode) noexcept {
    if (_micInputMode == mode)
        // If the microphone input mode is already set to the desired mode...
        return; // Do nothing

    _micInputMode = mode;

    if (_microphone && _micInputMode != MicInputMode::HostMic) {
        // If we have a host microphone open and we don't want it anymore...
        _microphone = nullopt;
    }

    if (_micInterface && _micInputMode == MicInputMode::HostMic) {
        // If we can access the host microphone and we want to use it...
        _microphone = retro::Microphone::Open(*_micInterface, { 44100 });
    }
}


void MelonDsDs::MicrophoneState::SetMicButtonMode(MicButtonMode mode) noexcept {
    _micButtonMode = mode;
    _shouldCaptureAudio = false;
    _prevShouldCaptureAudio = false;
    _prevMicButtonDown = false;
    _micButtonDown = false;
}

void MelonDsDs::MicrophoneState::SetMicButtonState(bool down) noexcept {
    ZoneScopedN(TracyFunction);

    _prevMicButtonDown = _micButtonDown;
    _micButtonDown = down;
    _prevShouldCaptureAudio = _shouldCaptureAudio;

    switch (_micButtonMode) {
        case MicButtonMode::Hold: {
            _shouldCaptureAudio = _micButtonDown;
            break;
        }
        case MicButtonMode::Toggle: {
            if (_micButtonDown && !_prevMicButtonDown) {
                // If the player just pressed the mic button (but isn't holding it)...
                _shouldCaptureAudio = !_shouldCaptureAudio;
            }
            break;
        }
        case MicButtonMode::Always: {
            _shouldCaptureAudio = true;
            break;
        }
    }

    if (_shouldCaptureAudio != _prevShouldCaptureAudio) {
        // If we should either start or stop the audio feed...
        if (_microphone) {
            _microphone->SetActive(_shouldCaptureAudio);
        }
    }
}


void MelonDsDs::MicrophoneState::Read(std::span<int16_t> buffer) noexcept {
    ZoneScopedN(TracyFunction);

    if (!_shouldCaptureAudio) {
        memset(buffer.data(), 0, buffer.size_bytes());
        return;
    }

    switch (_micInputMode) {
        case MicInputMode::WhiteNoise: {
            for (short& i : buffer)
                i = rand() & 0xFFFF;

            break;
        }
        case MicInputMode::HostMic: {
            if (_microphone && _microphone->IsActive() && _microphone->Read(buffer)) {
                // If the microphone is open and turned on, and we read from it successfully...
                break;
            }
            // If the mic isn't available, feed silence instead
            [[fallthrough]];
        }
        default:
            memset(buffer.data(), 0, buffer.size_bytes());
            break;
    }
}
