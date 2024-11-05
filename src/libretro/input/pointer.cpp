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

#include "pointer.hpp"

#include <glm/gtx/common.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <features/features_cpu.h>

#include "config/config.hpp"
#include "environment.hpp"
#include "input.hpp"
#include "screenlayout.hpp"
#include "tracy/client.hpp"

using MelonDsDs::PointerState;
using glm::vec2;


void PointerState::SetConfig(const CoreConfig& config) noexcept {
    _cursorMode = config.CursorMode();
    _touchMode = config.TouchMode();
}

void PointerState::Update(const InputPollResult& poll) noexcept {
    ZoneScopedN(TracyFunction);

    _previousTouching = _touching;
    _previousRawPosition = _rawPosition;

    if (_touchMode == TouchMode::Pointer || _touchMode == TouchMode::Auto) {
        _touching = poll.PointerPressed;
        _rawPosition = poll.PointerPosition;

        if ((_touching != _previousTouching) || CursorMoved()) {
            // If the player moved, pressed, or released the pointer within the past frame...
            _lastUpdated = poll.Timestamp;
        }
    }
}
