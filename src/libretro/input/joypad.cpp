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

void JoypadState::Update(const InputPollResult& poll) noexcept {
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

    // We'll send these bits to the DS in Apply() later
    _consoleButtons = ndsInputBits;

    _previousToggleLidButton = _toggleLidButton;
    _toggleLidButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_L3);

    _previousMicButton = _micButton;
    _micButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_L2);

    _previousCycleLayoutButton = _cycleLayoutButton;
    _cycleLayoutButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_R2);

    _previousJoystickTouchButton = _joystickTouchButton;
    _previousJoystickRawDirection = _joystickRawDirection;

    if (_touchMode == TouchMode::Joystick || _touchMode == TouchMode::Auto) {
        _joystickTouchButton = poll.JoypadButtons & (1 << RETRO_DEVICE_ID_JOYPAD_R3);
        _joystickRawDirection = poll.AnalogCursorDirection;

        if (_joystickTouchButton != _previousJoystickTouchButton || _joystickRawDirection != _previousJoystickRawDirection) {
            // If the player moved, pressed, or released the joystick cursor within the past frame...
            _lastPointerUpdate = poll.Timestamp;
        }
    }
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