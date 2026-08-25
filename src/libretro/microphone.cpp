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

#include <algorithm>
#include <cmath>
#include <optional>

#include <libretro.h>
#include <frontend/mic_blow.h>

#include "config/config.hpp"
#include "constants.hpp"
#include "environment.hpp"
#include "input/input.hpp"
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

void MelonDsDs::MicrophoneState::SetConfig(const CoreConfig& config) noexcept {
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
        OpenMicrophone();
    }
}

void MelonDsDs::MicrophoneState::OpenMicrophone() noexcept {
    ZoneScopedN(TracyFunction);

    constexpr auto WANTED_RATE = static_cast<unsigned>(MIC_SAMPLE_RATE);

    _resampler = nullopt;
    _microphone = retro::Microphone::Open(*_micInterface, { WANTED_RATE });
    if (!_microphone)
        return;

    // The frontend is free to ignore the sample rate we asked for,
    // in which case we have to convert its samples ourselves.
    optional<retro_microphone_params_t> params = _microphone->GetParams();
    unsigned actualRate = params ? params->rate : WANTED_RATE;

    if (actualRate == 0 || actualRate == WANTED_RATE) {
        retro::debug("Opened the host microphone at {} Hz", actualRate ? actualRate : WANTED_RATE);
        return;
    }

    _resampler = retro::Resampler::Create(actualRate, MIC_SAMPLE_RATE);
    if (_resampler) {
        retro::debug("Opened the host microphone at {} Hz; resampling to {} Hz", actualRate, WANTED_RATE);
    }
    else {
        retro::warn(
            "Opened the host microphone at {} Hz, but couldn't create a resampler for {} Hz; "
            "microphone input will be off-pitch",
            actualRate,
            WANTED_RATE
        );
    }
}

void MelonDsDs::MicrophoneState::Start() noexcept {
    ZoneScopedN(TracyFunction);
}

void MelonDsDs::MicrophoneState::Stop() noexcept {
    ZoneScopedN(TracyFunction);
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


int MelonDsDs::MicrophoneState::Read(std::span<int16_t> buffer) noexcept {
    ZoneScopedN(TracyFunction);

    if (!_shouldCaptureAudio) {
        memset(buffer.data(), 0, buffer.size_bytes());
        return buffer.size();
    }

    switch (_micInputMode) {
        case MicInputMode::WhiteNoise: {
            for (short& i : buffer)
                i = _random(_randomEngine);

            return buffer.size();
        }
        case MicInputMode::Blow: {
            constexpr size_t MIC_BLOW_LENGTH = sizeof(mic_blow) / sizeof(mic_blow[0]);

            // The built-in sample is 16-bit signed PCM
            // at the same rate that melonDS reads microphone input,
            // so we can feed it in as-is.
            for (int i = 0; i < buffer.size(); ++i) {
                buffer[i] = mic_blow[_blowSampleOffset];
                _blowSampleOffset = (_blowSampleOffset + 1) % MIC_BLOW_LENGTH;
            }

            return buffer.size();
        }
        case MicInputMode::HostMic: {
            if (_microphone && _microphone->IsActive()) {
                // If the microphone is open and turned on...
                if (!_resampler) {
                    // ...and the frontend gives us samples at the rate we want...
                    if (optional<unsigned> read = _microphone->Read(buffer); read && *read <= buffer.size()) {
                        return *read;
                    }
                }
                else {
                    // ...or if we have to convert them first...
                    auto wanted = static_cast<size_t>(std::ceil(buffer.size() / _resampler->Ratio()));
                    _hostBuffer.resize(wanted);

                    optional<unsigned> read = _microphone->Read(_hostBuffer);
                    if (read && *read <= _hostBuffer.size()) {
                        return _resampler->ProcessMono(std::span(_hostBuffer).first(*read), buffer);
                    }
                }
            }
            // If the mic isn't available, feed silence instead
            [[fallthrough]];
        }
        default:
            memset(buffer.data(), 0, buffer.size_bytes());
            return buffer.size();
    }
}
