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

using std::nullopt;

retro::Microphone::Microphone(retro_microphone_t* microphone, const retro_microphone_interface& microphone_interface) noexcept
    : _microphoneInterface(microphone_interface), _microphone(microphone)
{
}

retro::Microphone::~Microphone() noexcept {
    if (_microphoneInterface.close_mic && _microphone) {
        _microphoneInterface.close_mic(_microphone);
    }
}

retro::Microphone::Microphone(Microphone&& other) noexcept :
    _microphone(other._microphone),
    _microphoneInterface(other._microphoneInterface) {
    other._microphoneInterface = {};
    other._microphone = {};
}

retro::Microphone::Microphone& retro::Microphone::operator=(retro::Microphone&& other) noexcept {
    if (this != &other) {
        if (_microphoneInterface.close_mic && _microphone) {
            _microphoneInterface.close_mic(_microphone);
        }

        _microphone = other._microphone;
        _microphoneInterface = other._microphoneInterface;

        other._microphoneInterface = {};
        other._microphone = {};
    }

    return *this;
}

std::optional<retro_microphone_params_t> retro::Microphone::GetParams() const noexcept {
    if (!_microphoneInterface.get_params)
        return nullopt;

    if (!_microphone)
        return nullopt;

    retro_microphone_params out {};
    if (!_microphoneInterface.get_params(_microphone, &out))
        return nullopt;

    return out;
}

bool retro::Microphone::SetActive(bool on) noexcept {
    if (!_microphoneInterface.get_params)
        return false;

    if (!_microphone)
        return false;

    return _microphoneInterface.set_mic_state(_microphone, on);
}

bool retro::Microphone::IsActive() const noexcept {
    if (!_microphoneInterface.get_params)
        return false;

    if (!_microphone)
        return false;

    return _microphoneInterface.get_mic_state(_microphone);
}

int retro::Microphone::Read(std::span<int16_t> buffer) noexcept {
    if (!_microphoneInterface.get_params)
        return -1;

    if (!_microphone)
        return -1;

    return _microphoneInterface.read_mic(_microphone, buffer.data(), buffer.size() / 2);
}

std::optional<retro::Microphone> retro::Microphone::Open(
    const retro_microphone_interface& micInterface,
    retro_microphone_params_t params
) noexcept {
    if (!micInterface.open_mic)
        return nullopt;

    auto mic = micInterface.open_mic(&params);
    if (!mic)
        return nullopt;

    return Microphone(mic, micInterface);
}