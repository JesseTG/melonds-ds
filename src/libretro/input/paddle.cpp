/*
    Copyright 2026 Matheus Fraguas

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

#include "paddle.hpp"

#include <NDS.h>
#include <Savestate.h>

#include "config/config.hpp"
#include "environment.hpp"
#include "tracy/client.hpp"

using MelonDsDs::PaddleCart;
using MelonDsDs::PaddleState;

PaddleCart::PaddleCart() noexcept :
    CartCommon(static_cast<melonDS::GBACart::CartType>(PADDLE_CART_TYPE)) {
}

void PaddleCart::DoSavestate(melonDS::Savestate* file) {
    CartCommon::DoSavestate(file);
    file->Var16(&_position);
}

melonDS::u16 PaddleCart::ROMRead(melonDS::u32 addr) const {
    // The paddle's ROM bus only ever carries this value;
    // games look for it to tell the paddle apart from other Slot-2 devices.
    return 0xEFFF;
}

void PaddleCart::ROMWrite(melonDS::u32 addr, melonDS::u16 val) {
    _position = 0;
}

melonDS::u8 PaddleCart::SRAMRead(melonDS::u32 addr) {
    switch (addr & 0x00FFFFFF) {
        case 0:
            return _position & 0xFF;
        case 1:
            return (_position >> 8) & 0x0F;
        default:
            return 0;
    }
}

void PaddleCart::SRAMWrite(melonDS::u32 addr, melonDS::u8 val) {
    _position = 0;
}

void PaddleCart::Turn(int steps) noexcept {
    // The counter is 12 bits wide and wraps around in both directions;
    // converting to u16 first keeps negative steps two's-complement before masking.
    _position = static_cast<melonDS::u16>(_position + steps) & POSITION_MASK;
}

/// The mouse buttons that \c PaddleMouseButton refers to, in the same order.
static constexpr unsigned PADDLE_MOUSE_BUTTON_IDS[MelonDsDs::PaddleMouseButtonCount] = {
    RETRO_DEVICE_ID_MOUSE_LEFT,
    RETRO_DEVICE_ID_MOUSE_RIGHT,
    RETRO_DEVICE_ID_MOUSE_MIDDLE,
    RETRO_DEVICE_ID_MOUSE_BUTTON_4,
    RETRO_DEVICE_ID_MOUSE_BUTTON_5,
};

/// How far the left analog stick turns the dial each frame
/// when it's pushed all the way at 1x speed, in counter units.
static constexpr int STICK_STEPS_PER_FRAME = 32;

void PaddleState::SetConfig(const CoreConfig& config) noexcept {
    _axis = config.PaddleAxis();
    _speed = config.PaddleSpeed();
    _inverted = config.PaddleInverted();
    _mouseButtonMappings = config.PaddleMouseButtons();

    bool hasToggleButton = false;
    for (uint32_t mapping : _mouseButtonMappings) {
        hasToggleButton |= (mapping & PADDLE_TOGGLE_BIT) != 0;
    }

    if (!hasToggleButton) {
        // Without a toggle button, there'd be no way to get the mouse back,
        // so the mouse always turns the paddle
        // (as with a spinner that has no buttons of its own).
        _capturesMouse = true;
    }
}

void PaddleState::Update() noexcept {
    ZoneScopedN(TracyFunction);

    // Each mouse button can press DS buttons or switch the mouse to the touch screen and back;
    // the mappings say which.
    uint32_t mouseButtonActions = 0;
    for (unsigned i = 0; i < PaddleMouseButtonCount; ++i) {
        if (retro::input_state(0, RETRO_DEVICE_MOUSE, 0, PADDLE_MOUSE_BUTTON_IDS[i]) != 0) {
            mouseButtonActions |= _mouseButtonMappings[i];
        }
    }

    // Any toggle button hands the mouse back and forth
    // between the paddle and the touch screen.
    bool previousToggleButton = _toggleButton;
    _toggleButton = (mouseButtonActions & PADDLE_TOGGLE_BIT) != 0;
    if (_toggleButton && !previousToggleButton) {
        _capturesMouse = !_capturesMouse;
        _remainder = 0;
        retro_message_ext message {
            .msg = _capturesMouse ? "Mouse controls the paddle" : "Mouse controls the touch screen",
            .duration = 2000,
            .priority = 1,
            .level = RETRO_LOG_INFO,
            .target = RETRO_MESSAGE_TARGET_OSD,
            .type = RETRO_MESSAGE_TYPE_NOTIFICATION_ALT,
            .progress = -1,
        };
        retro::set_message(message);
    }

    // The left analog stick turns the dial no matter who has the mouse;
    // the further it's pushed, the faster the dial turns.
    // Measured in hundredths of a step at 1x speed, like the mouse movement below.
    int movement = retro::input_state(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X)
        * STICK_STEPS_PER_FRAME * 100 / INT16_MAX;

    _joypadButtons = 0;
    if (_capturesMouse) {
        _joypadButtons = mouseButtonActions & ~PADDLE_TOGGLE_BIT;

        // libretro reports the mouse's X and Y as movement since the last poll.
        switch (_axis) {
            case PaddleAxis::MouseX:
                movement += retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X) * 100;
                break;
            case PaddleAxis::MouseY:
                movement += retro::input_state(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y) * 100;
                break;
        }
    }

    if (_inverted) {
        movement = -movement;
    }

    // Scale the movement by the configured speed (in percent),
    // carrying the fractional part over to the next frame
    // so that slow speeds and slight stick pushes still turn the dial eventually.
    int scaled = movement * static_cast<int>(_speed) / 100 + _remainder;
    _steps = scaled / 100;
    _remainder = scaled % 100;
}

void PaddleState::Apply(melonDS::NDS& nds) const noexcept {
    ZoneScopedN(TracyFunction);

    if (_steps == 0) {
        return;
    }

    melonDS::GBACart::CartCommon* cart = nds.GetGBACart();
    if (cart && cart->Type() == PADDLE_CART_TYPE) {
        static_cast<PaddleCart*>(cart)->Turn(_steps);
    }
}
