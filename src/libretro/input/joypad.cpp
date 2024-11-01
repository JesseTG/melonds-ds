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


#include "joypad.hpp"

#include <NDS.h>
#include <features/features_cpu.h>
#include <glm/gtx/rotate_vector.hpp>

#include "environment.hpp"
#include "format.hpp"
#include "microphone.hpp"
#include "screenlayout.hpp"
#include "tracy/client.hpp"

using glm::vec2;
using MelonDsDs::JoypadState;

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

void JoypadState::SetConfig(const CoreConfig& config) noexcept {
    _touchMode = config.TouchMode();
}

#define ADD_KEY_TO_MASK(key, i, bits) \
    do { \
        if (bits & (1 << (key))) {               \
            ndsInputBits &= ~(1 << (i));           \
        }                                      \
        else {                                 \
            ndsInputBits |= (1 << (i));            \
        }                                      \
    } while (false)

void JoypadState::Poll(const InputPollResult& poll) noexcept {
    ZoneScopedN(TracyFunction);
    uint32_t ndsInputBits = 0xFFF; // Input bits passed to the emulated DS

    // Delicate bit manipulation; do not touch!
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_A, 0, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_B, 1, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_SELECT, 2, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_START, 3, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_RIGHT, 4, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_LEFT, 5, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_UP, 6, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_DOWN, 7, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_R, 8, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_L, 9, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_X, 10, poll.JoypadButtons);
    ADD_KEY_TO_MASK(RETRO_DEVICE_ID_JOYPAD_Y, 11, poll.JoypadButtons);

    _consoleButtons = ndsInputBits;

    _previousToggleLidButton = _toggleLidButton;
    _toggleLidButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_L3);

    _previousMicButton = _micButton;
    _micButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_L2);

    _previousCycleLayoutButton = _cycleLayoutButton;
    _cycleLayoutButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_R2);

    _previousJoystickTouchButton = _joystickTouchButton;


    ScreenLayout layout = screen_layout_data.Layout();
//    if (layout == ScreenLayout::TopOnly) {
//        isPointerTouching = false;
//        _joystickTouchButton = false;
//    } else {
//    if (_touchMode == TouchMode::Pointer || _touchMode == TouchMode::Auto) {
//        isPointerTouching = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
//        pointerRawPosition.x = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
//        pointerRawPosition.y = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
//        pointerTouchPosition = screen_layout_data.TransformPointerInput(pointerRawPosition);
//        hybridTouchPosition = screen_layout_data.TransformPointerInputToHybridScreen(pointerRawPosition);
//
//        if (isPointerTouching != previousIsPointerTouching || pointerTouchPosition != previousPointerTouchPosition) {
//            // If the player moved, pressed, or released the pointer within the past frame...
//            cursorTimeout = maxCursorTimeout * 60;
//            pointerUpdateTimestamp = now;
//        }
//    }

    // TODO: Pass the screen layout data into Poll(), or move its use outside
    if (_touchMode == TouchMode::Joystick || _touchMode == TouchMode::Auto) {
        retro::ScreenOrientation orientation = screen_layout_data.EffectiveOrientation();
        _joystickTouchButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_R3);
        _joystickRawDirection = retro::analog_state(0, RETRO_DEVICE_INDEX_ANALOG_RIGHT);
        _joystickRawDirection = glm::rotate(vec2(_joystickRawDirection), GetOrientationAngle(orientation));

        if (_joystickRawDirection != i16vec2(0)) {
            // If the player moved the joystick this frame...
//            if (pointerUpdateTimestamp > _lastPointerUpdate) {
//                _joystickCursorPosition = pointerTouchPosition;
//            }
            _joystickCursorPosition += _joystickRawDirection / i16vec2(2048);
            _joystickCursorPosition = clamp(_joystickCursorPosition, ivec2(0), NDS_SCREEN_SIZE<int> - 1);
        }

        if (_joystickTouchButton != _previousJoystickTouchButton || _joystickCursorPosition != _previousJoystickCursorPosition) {
            // If the player moved, pressed, or released the joystick cursor within the past frame...
            // cursorTimeout = maxCursorTimeout * 60;
            // TODO: reset cursorTimeout in InputState
            _lastPointerUpdate = poll.Timestamp;
        }
    }
    //}
}


void JoypadState::Apply(melonDS::NDS& nds) const noexcept {
    nds.SetKeyMask(_consoleButtons);

    if (_toggleLidButton && !_previousToggleLidButton) {
        // If the "toggle lid" button was just pressed (and is not being held)...
        nds.SetLidClosed(!nds.IsLidClosed());
        retro::debug("{} the lid", nds.IsLidClosed() ? "Closed" : "Opened");
    }
}


void JoypadState::Apply(ScreenLayoutData& layout) const noexcept {
    if (_cycleLayoutButton && !_previousCycleLayoutButton) {
        // If the "cycle screen layout" button was just pressed (and is not being held)...

        layout.NextLayout(); // ...update the screen layout to the next in the sequence.
        retro::debug("Switched to screen layout {} of {} ({})", layout.LayoutIndex() + 1, layout.NumberOfLayouts(), layout.Layout());
        // Add 1 to the index because we present the layout index as 1-based to the user.
    }
}

void JoypadState::Apply(MicrophoneState& mic) const noexcept {
    mic.SetMicButtonState(_micButton);
}

void JoypadState::SetControllerPortDevice(unsigned int port, unsigned int device) noexcept {
    _device = device;
}