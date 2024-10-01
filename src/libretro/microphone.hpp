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

#ifndef MELONDS_DS_MICROPHONE_HPP
#define MELONDS_DS_MICROPHONE_HPP

#include <cstdint>
#include <limits>
#include <optional>
#include <random>

#include "config/types.hpp"
#include "retro/microphone.hpp"

namespace MelonDsDs {
    class InputState;
    class CoreConfig;

    class MicrophoneState {
    public:
        MicrophoneState() noexcept;

        void Apply(const CoreConfig& config) noexcept;
        bool IsMicInterfaceAvailable() const noexcept { return _micInterface.has_value(); }
        bool IsHostMicOpen() const noexcept { return _microphone.has_value(); }
        bool IsHostMicActive() const noexcept { return _microphone && _microphone->IsActive(); }

        void Read(std::span<int16_t> buffer) noexcept;

        MicInputMode GetMicInputMode() const noexcept { return _micInputMode; }
        void SetMicInputMode(MicInputMode mode) noexcept;

        MicButtonMode GetMicButtonMode() const noexcept { return _micButtonMode; }
        void SetMicButtonMode(MicButtonMode mode) noexcept;

        void SetMicButtonState(bool down) noexcept;

    private:
        std::optional<retro_microphone_interface> _micInterface {};
        std::optional<retro::Microphone> _microphone {};
        MicInputMode _micInputMode = MicInputMode::None;
        MicButtonMode _micButtonMode = MicButtonMode::Hold;
        size_t _blowSampleOffset = 0;
        std::default_random_engine _randomEngine;
        std::uniform_int_distribution<int16_t> _random {std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()};
        bool _micButtonDown = false;
        bool _prevMicButtonDown = false;
        bool _shouldCaptureAudio = false;
        bool _prevShouldCaptureAudio = false;
    };
}

#endif //MELONDS_DS_MICROPHONE_HPP
