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

#include "config/types.hpp"
#include <libretro.h>
#include <glm/vec2.hpp>

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    class CoreConfig;
    class ScreenLayoutData;
    class MicrophoneState;
    struct InputPollResult;

    class PointerState {
    public:
        void SetConfig(const CoreConfig& config) noexcept;
        void Update(const InputPollResult& poll) noexcept;

        [[nodiscard]] retro_perf_tick_t LastPointerUpdate() const noexcept { return _lastUpdated; }
        [[nodiscard]] glm::i16vec2 RawPosition() const noexcept { return _rawPosition; }
        [[nodiscard]] bool IsTouching() const noexcept { return _touching; }
        [[nodiscard]] bool CursorReleased() const noexcept { return _previousTouching && !_touching; }
        [[nodiscard]] bool CursorMoved() const noexcept { return _rawPosition != _previousRawPosition; }
        [[nodiscard]] bool CursorActive() const noexcept {
            return CursorMoved() || (_touching != _previousTouching);
        }
    private:
        bool _touching;
        bool _previousTouching;
        glm::i16vec2 _rawPosition;
        glm::i16vec2 _previousRawPosition;
        retro_perf_tick_t _lastUpdated;
        enum CursorMode _cursorMode;
        enum TouchMode _touchMode;
    };
}