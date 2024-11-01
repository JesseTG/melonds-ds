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
    using glm::ivec2;
    using glm::i16vec2;
    using glm::uvec2;

    class CoreConfig;
    class ScreenLayoutData;
    class MicrophoneState;
    struct InputPollResult;

    class PointerState {
    public:
        void SetConfig(const CoreConfig& config) noexcept;
        void Poll(const InputPollResult& poll) noexcept;
        void Apply(melonDS::NDS& nds) const noexcept;
        void Apply(ScreenLayoutData& layout) const noexcept;

        [[nodiscard]] retro_perf_tick_t LastPointerUpdate() const noexcept { return _pointerUpdateTimestamp; }
        [[nodiscard]] uvec2 ConsoleTouchCoordinates(const ScreenLayoutData& layout) const noexcept;
        [[nodiscard]] ivec2 TouchPosition() const noexcept { return _pointerTouchPosition; }
        [[nodiscard]] i16vec2 RawPosition() const noexcept { return _pointerRawPosition; }
        [[nodiscard]] ivec2 HybridTouchPosition() const noexcept { return _hybridTouchPosition; }
        [[nodiscard]] bool IsTouching() const noexcept { return _isPointerTouching; }
        [[nodiscard]] bool TouchReleased() const noexcept { return _previousIsPointerTouching && !_isPointerTouching; }

    private:
        bool _isPointerTouching;
        bool _previousIsPointerTouching;
        ivec2 _previousPointerTouchPosition;
        ivec2 _pointerTouchPosition;
        i16vec2 _pointerRawPosition;
        retro_perf_tick_t _pointerUpdateTimestamp;

        /// Touch coordinates of the pointer on the hybrid screen,
        /// in NDS pixel coordinates.
        /// Only relevant if a hybrid layout is active
        ivec2 _hybridTouchPosition;
        retro_perf_tick_t _joystickTimestamp;
        enum CursorMode _cursorMode;
        enum TouchMode _touchMode;
    };
}