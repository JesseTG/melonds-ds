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
#include "environment.hpp"

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

void CursorState::Update(const CoreConfig& config, const ScreenLayoutData& layout, const PointerState& pointer, const JoypadState& joypad) noexcept {
    if (_cursorSettingsDirty) {
        ResetCursorTimeout();
    }

    _joypadCursorTouching = joypad.IsTouching();
    _joypadCursorLastUpdate = joypad.LastPointerUpdate();
    _pointerCursorLastUpdate = pointer.LastPointerUpdate();
    _isTouchReleased = pointer.CursorReleased() || joypad.TouchReleased();

    if (_touchMode == TouchMode::Pointer || _touchMode == TouchMode::Auto) {
        _pointerRawPosition = pointer.RawPosition();
        _pointerCursorTouching = pointer.IsTouching();
        _pointerCursorPosition = layout.TransformPointerInput(_pointerRawPosition);
        _hybridTouchPosition = layout.TransformPointerInputToHybridScreen(_pointerRawPosition);

        if (pointer.CursorActive()) {
            // If the player moved, pressed, or released the pointer within the past frame...
            ResetCursorTimeout();
        }
    }

    if (_touchMode == TouchMode::Joystick || _touchMode == TouchMode::Auto) {
        retro::ScreenOrientation orientation = layout.EffectiveOrientation();
        _joypadCursorTouching = joypad.IsTouching();
        _joystickRawDirection = joypad.RawCursorDirection();
        if (_joystickRawDirection != i16vec2(0)) {
            // If the player moved the joypad's cursor this frame...
            if (_pointerCursorLastUpdate > _joypadCursorLastUpdate) {
                // If the pointer was used more recently than the joypad cursor...
                // Then continue using the cursor from where the pointer last left it
                _joystickCursorPosition = _pointerCursorPosition;
            }
            // Rotate the joypad cursor to match the screen layout (if necessary),
            // then clamp it to the touch screen's coordinates.

            int maxSpeed = config.JoystickCursorMaxSpeed();
            float realSpeed;
            switch (maxSpeed) {
                case 1:
                    realSpeed = 0.4f;
                    break;            
                case 2:
                    realSpeed = 0.6f;
                    break;
                case 3:
                    realSpeed = 0.8f;
                    break;
                case 4:
                    realSpeed = 1.0f;
                    break;
                case 5:
                    realSpeed = 1.2f;
                    break;
                case 6:
                    realSpeed = 1.4f;
                    break;
                case 7:
                    realSpeed = 1.6f;
                    break;
                case 8:
                    realSpeed = 1.8f;
                    break;
                case 9:
                    realSpeed = 2.0f;
                    break;
                default:
                    realSpeed = 1.0f;
            }            
            //float widthSpeed = (NDS_SCREEN_WIDTH / 20.0) * realSpeed; //Currently unused
            float heightSpeed = (NDS_SCREEN_HEIGHT / 20.0) * realSpeed;
            float joystickNormX = _joystickRawDirection.x/32767.0;
            float joystickNormY = _joystickRawDirection.y/32767.0;
            float deadzone = config.JoystickCursorDeadzone() / 100.0f;
            bool speedup_enabled = config.JoystickSpeedupEnabled();
            float responsecurve = config.JoystickCursorResponse() / 100.0f;
            float speedupratio = config.JoystickCursorSpeedup() / 100.0f;
            float radialLength = std::sqrt((joystickNormX * joystickNormX) + (joystickNormY * joystickNormY));
            float joystickScaledX = 0.0f;
            float joystickScaledY = 0.0f;
            if (radialLength > deadzone) {
                // Get X and Y as a relation to the radial length
                float dirX = joystickNormX / radialLength;
                float dirY = joystickNormY / radialLength;
                // Apply deadzone and response curve
                float scaledLength = (radialLength - deadzone) / (1.0f - deadzone);
                float curvedLength = std::pow(std::min<float>(1.0f, scaledLength), responsecurve);
                // Final output
                float finalLength = speedup_enabled ? curvedLength * speedupratio : curvedLength;
                joystickScaledX = dirX * finalLength;
                joystickScaledY = dirY * finalLength;    
            } else {
                joystickScaledX = 0.0f;
                joystickScaledY = 0.0f;
            }
            _joystickCursorPosition +=  vec2(joystickScaledX * heightSpeed, joystickScaledY * heightSpeed);
            _joystickCursorPosition = clamp(_joystickCursorPosition, vec2(1.0), NDS_SCREEN_SIZE<float> - 1.0f); 
            
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

bool CursorState::CursorVisible() const noexcept {
    bool modeAllowsCursor = false;
    switch (_cursorMode) {
        case CursorMode::Always:
            modeAllowsCursor = true;
            break;
        case CursorMode::Never:
            modeAllowsCursor = false;
            break;
        case CursorMode::Touching:
            modeAllowsCursor = IsTouching();
            break;
        case CursorMode::Timeout:
            modeAllowsCursor = _cursorTimeout > 0;
            break;
    }

    return modeAllowsCursor && IsCursorInputInBounds();
}

bool CursorState::IsCursorInputInBounds() const noexcept {
    switch (_touchMode) {
        case TouchMode::Pointer:
            // Finger is touching the screen or the mouse cursor is atop the window
            return _pointerRawPosition != i16vec2(0);
        case TouchMode::Joystick:
            // Joystick cursor is constrained to always be on the touch screen
            return true;
        case TouchMode::Auto:
            // If the joystick cursor was last used, it's an automatic true.
            // If the pointer was last used, see if the value is zero
            return (_joypadCursorLastUpdate > _pointerCursorLastUpdate) || (_pointerRawPosition != i16vec2(0));
        default:
            return false;
    }

    // Why are we comparing _pointerRawPosition against (0, 0)?
    // libretro's pointer API returns (0, 0) if the pointer is not over the play area, even if it's still over the window.
    // Theoretically means that the cursor will be hidden if the player moves the pointer to the dead center of the screen,
    // but the screen's resolution probably isn't big enough for that to happen in practice.
}
