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

#include "cursor.hpp"

#include "config/config.hpp"
#include "environment.hpp"

using MelonDsDs::CursorState;


void CursorState::SetConfig(const CoreConfig& config) noexcept {

    if (config.CursorMode() != _cursorMode) _cursorSettingsDirty = true;
    if (config.CursorTimeout() != _maxCursorTimeout) _cursorSettingsDirty = true;
    if (config.TouchMode() != _touchMode) _cursorSettingsDirty = true;

    _cursorMode = config.CursorMode();
    _maxCursorTimeout = config.CursorTimeout();
    _touchMode = config.TouchMode();
}

void CursorState::Poll() noexcept {
    if (_cursorSettingsDirty) {
        _cursorTimeout = _maxCursorTimeout * 60;
    }

    ScreenLayout layout = screen_layout_data.Layout();
//    if (layout == ScreenLayout::TopOnly) {
//        isPointerTouching = false;
//        joystickTouchButton = false;
//    } else {
    if (_touchMode == TouchMode::Pointer || _touchMode == TouchMode::Auto) {
//

        if (isPointerTouching != previousIsPointerTouching || pointerTouchPosition != previousPointerTouchPosition) {
            // If the player moved, pressed, or released the pointer within the past frame...
            _cursorTimeout = _maxCursorTimeout * 60;
            pointerUpdateTimestamp = now;
        }
    }

    if (touchMode == TouchMode::Joystick || touchMode == TouchMode::Auto) {
        retro::ScreenOrientation orientation = screen_layout_data.EffectiveOrientation();
        joystickTouchButton = retroInputBits & (1 << RETRO_DEVICE_ID_JOYPAD_R3);
        joystickRawDirection.x = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
        joystickRawDirection.y = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
        joystickRawDirection = (i16vec2)glm::rotate(vec2(joystickRawDirection), GetOrientationAngle(orientation));

        if (joystickRawDirection != i16vec2(0)) {
            if (pointerUpdateTimestamp > joystickTimestamp) {
                joystickCursorPosition = pointerTouchPosition;
            }
            joystickCursorPosition += joystickRawDirection / i16vec2(2048);
            joystickCursorPosition = clamp(joystickCursorPosition, ivec2(0), NDS_SCREEN_SIZE<int> - 1);
        }

        if (joystickTouchButton != previousJoystickTouchButton || joystickCursorPosition != previousJoystickCursorPosition) {
            // If the player moved, pressed, or released the joystick within the past frame...
            _cursorTimeout = _maxCursorTimeout * 60;
            joystickTimestamp = now;
        }
    }
//    }

    if (_cursorMode == CursorMode::Timeout && _cursorTimeout > 0) {
        _cursorTimeout--;
    }

    _cursorSettingsDirty = false;
}