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

#include <NDS.h>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/common.hpp>

#include "config/config.hpp"
#include "environment.hpp"
#include "screenlayout.hpp"

using MelonDsDs::CursorState;
using glm::ivec2;
using glm::uvec2;
using glm::vec2;

constexpr float GetOrientationAngle(retro::ScreenOrientation orientation) noexcept {
    switch (orientation) {
        case retro::ScreenOrientation::Normal:
            return 0;
        case retro::ScreenOrientation::RotatedLeft:
            return glm::radians(90.f);
        case retro::ScreenOrientation::UpsideDown:
            return glm::radians(180.f);
        case retro::ScreenOrientation::RotatedRight:
            return glm::radians(270.f);
        default:
            return 0;
    }
}

void CursorState::SetConfig(const CoreConfig& config) noexcept {

    if (config.CursorMode() != _cursorMode) _cursorSettingsDirty = true;
    if (config.CursorTimeout() != _maxCursorTimeout) _cursorSettingsDirty = true;
    if (config.TouchMode() != _touchMode) _cursorSettingsDirty = true;

    _cursorMode = config.CursorMode();
    _maxCursorTimeout = config.CursorTimeout();
    _touchMode = config.TouchMode();
}

void CursorState::Update(const ScreenLayoutData& layout, const PointerState& pointer, const JoypadState& joypad) noexcept {
    if (_cursorSettingsDirty) {
        ResetCursorTimeout();
    }

    _joypadCursorTouching = joypad.IsTouching();
    _joypadCursorLastUpdate = joypad.LastPointerUpdate();
    _pointerCursorLastUpdate = pointer.LastPointerUpdate();

    if (_touchMode == TouchMode::Pointer || _touchMode == TouchMode::Auto) {
        i16vec2 pointerCoordsRaw = pointer.RawPosition();
        _pointerCursorTouching = pointer.IsTouching();
        _pointerCursorPosition = layout.TransformPointerInput(pointerCoordsRaw);
        _hybridTouchPosition = layout.TransformPointerInputToHybridScreen(pointerCoordsRaw);

        if (pointer.CursorActive()) {
            // If the player moved, pressed, or released the pointer within the past frame...
            ResetCursorTimeout();
        }
    }

    if (_touchMode == TouchMode::Joystick || _touchMode == TouchMode::Auto) {
        retro::ScreenOrientation orientation = layout.EffectiveOrientation();
        _joypadCursorTouching = joypad.IsTouching();
        _joystickRawDirection = joypad.RawCursorDirection();
        _joystickRawDirection = (i16vec2)glm::rotate(vec2(_joystickRawDirection), GetOrientationAngle(orientation));

        if (_joystickRawDirection != i16vec2(0)) {
            // If the player moved the joypad's cursor this frame...

            if (_pointerCursorLastUpdate > _joypadCursorLastUpdate) {
                // If the pointer was used more recently than the joypad cursor...
                // Then continue using the cursor from where the pointer last left it
                _joystickCursorPosition = _pointerCursorPosition;
            }
            // Rotate the joypad cursor to match the screen layout (if necessary),
            // then clamp it to the touch screen's coordinates
            // TODO: Allow speed to be customized
            _joystickCursorPosition += _joystickRawDirection / i16vec2(2048);
            _joystickCursorPosition = clamp(_joystickCursorPosition, ivec2(0), NDS_SCREEN_SIZE<int> - 1);
        }

        if (joypad.CursorActive()) {
            // If the player moved, pressed, or released the joystick within the past frame...
            ResetCursorTimeout();
        }
    }

    _consoleTouchPosition = ConsoleTouchPosition(layout);

    if (_cursorMode == CursorMode::Timeout && _cursorTimeout > 0) {
        _cursorTimeout--;
    }



    _cursorSettingsDirty = false;
}

void CursorState::Apply(melonDS::NDS& nds) const noexcept {
    if (IsTouching()) {
        nds.TouchScreen(_consoleTouchPosition.x, _consoleTouchPosition.y);
    } else if (TouchReleased()) {
        nds.ReleaseScreen();
    }
}


void CursorState::ResetCursorTimeout() noexcept {
    _cursorTimeout = _maxCursorTimeout * 60; // TODO: Make this use real time instead of frames (see retro::last_frame_time)
}

uvec2 CursorState::ConsoleTouchPosition(const ScreenLayoutData& layout) const noexcept {
    uvec2 clampedTouch;

    switch (layout.Layout()) {
        case ScreenLayout::HybridBottom:
        case ScreenLayout::FlippedHybridBottom:
            if (layout.HybridSmallScreenLayout() == HybridSideScreenDisplay::One) {
                // If the touch screen is only shown in the hybrid-screen position...
                clampedTouch = clamp(_hybridTouchPosition, ivec2(0), NDS_SCREEN_SIZE<int> - 1);
                // ...then that's the only transformation we'll use for input.
                break;
            } else if (!all(glm::openBounded(TouchPosition(), ivec2(0), NDS_SCREEN_SIZE<int>))) {
                // The touch screen is shown in both the hybrid and secondary positions.
                // If the touch input is not within the secondary position's bounds...
                clampedTouch = clamp(_hybridTouchPosition, ivec2(0), NDS_SCREEN_SIZE<int> - 1);
                break;
            }
            [[fallthrough]];
        default:
            clampedTouch = clamp(TouchPosition(), ivec2(0), NDS_SCREEN_SIZE<int> - 1);
            break;

    }

    return clampedTouch;
}

ivec2 CursorState::TouchPosition() const noexcept {
    if (_touchMode == TouchMode::Joystick) {
        // If we're using joystick mode, ignore the pointer entirely
        return _joystickCursorPosition;
    }

    if (_touchMode == TouchMode::Pointer) {
        // If we're using pointer mode, ignore the joystick entirely
        return _pointerCursorPosition;
    }

    // Otherwise, prioritize whichever is currently being held down
    if (_pointerCursorTouching) {
        return _pointerCursorPosition;
    }

    if (_joypadCursorTouching) {
        return _joystickCursorPosition;
    }

    // If neither the joypad nor pointer are being held down, use whichever one was most recently updated
    if (_pointerCursorLastUpdate > _joypadCursorLastUpdate) {
        return _pointerCursorPosition;
    }

    // Joystick's cursor was updated most recently, use it
    return _joystickCursorPosition;
}

bool CursorState::IsTouching() const noexcept {
    switch (_touchMode) {
        case TouchMode::Joystick:
            return _joypadCursorTouching;
        case TouchMode::Pointer:
            return _pointerCursorTouching;
        case TouchMode::Auto:
            return _pointerCursorTouching || _joypadCursorTouching;
        default:
            return false;
    }
}