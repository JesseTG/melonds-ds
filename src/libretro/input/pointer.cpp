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
#include "tracy/client.hpp"

using MelonDsDs::PointerState;
using glm::vec2;


void PointerState::SetConfig(const CoreConfig& config) noexcept {
    _cursorMode = config.CursorMode();
    _touchMode = config.TouchMode();
}

void PointerState::Poll(const InputPollResult& poll) noexcept {
    ZoneScopedN(TracyFunction);

    _previousPointerTouchPosition = _pointerTouchPosition;
    _previousIsPointerTouching = _isPointerTouching;

    ScreenLayout layout = screen_layout_data.Layout();

    // TODO: Move matrix transformations out of PointerState and into InputState
    if (_touchMode == TouchMode::Pointer || _touchMode == TouchMode::Auto) {
        _isPointerTouching = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
        _pointerRawPosition.x = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
        _pointerRawPosition.y = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

        // TODO: Can I decouple the screen layout from here?
        _pointerTouchPosition = screen_layout_data.TransformPointerInput(_pointerRawPosition);
        _hybridTouchPosition = screen_layout_data.TransformPointerInputToHybridScreen(_pointerRawPosition);

        if (_isPointerTouching != _previousIsPointerTouching || _pointerTouchPosition != _previousPointerTouchPosition) {
            // If the player moved, pressed, or released the pointer within the past frame...
            _pointerUpdateTimestamp = poll.Timestamp;
        }
    }
}