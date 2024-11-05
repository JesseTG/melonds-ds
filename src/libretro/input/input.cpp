/*
    Copyright 2023 Jesse Talavera-Greenberg

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

#include "input.hpp"

#include <Platform.h>
#include "PlatformOGLPrivate.h"
#include <NDS.h>
#include <glm/gtx/common.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <features/features_cpu.h>

#include "config/config.hpp"
#include "core/core.hpp"
#include "environment.hpp"
#include "format.hpp"
#include "info.hpp"
#include "libretro.hpp"
#include "math.hpp"
#include "screenlayout.hpp"
#include "tracy.hpp"
#include "utils.hpp"

using MelonDsDs::InputState;
using glm::ivec2;
using glm::ivec3;
using glm::i16vec2;
using glm::mat3;
using glm::vec2;
using glm::vec3;
using glm::uvec2;
using MelonDsDs::NDS_SCREEN_SIZE;
using MelonDsDs::NDS_SCREEN_HEIGHT;

const struct retro_input_descriptor MelonDsDs::input_descriptors[] = {
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_UP,     "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_A,      "A"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_B,      "B"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_START,  "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R,      "R"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L,      "L"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_X,      "X"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_Y,      "Y"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L2,     "Microphone"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R2,     "Next Screen Layout"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_L3,     "Close Lid"},
        {0, RETRO_DEVICE_JOYPAD, 0,                               RETRO_DEVICE_ID_JOYPAD_R3,     "Touch Joystick"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X,      "Touch Joystick Horizontal"},
        {0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,      "Touch Joystick Vertical"},
        {},
};

static bool IsInNdsScreenBounds(ivec2 position) noexcept {
    return glm::all(glm::openBounded(position, ivec2(0), NDS_SCREEN_SIZE<int>));
}



static const char *device_name(unsigned device) {
    switch (device) {
        case RETRO_DEVICE_NONE:
            return "RETRO_DEVICE_NONE";
        case RETRO_DEVICE_JOYPAD:
            return "RETRO_DEVICE_JOYPAD";
        case RETRO_DEVICE_MOUSE:
            return "RETRO_DEVICE_MOUSE";
        case RETRO_DEVICE_KEYBOARD:
            return "RETRO_DEVICE_KEYBOARD";
        case RETRO_DEVICE_LIGHTGUN:
            return "RETRO_DEVICE_LIGHTGUN";
        case RETRO_DEVICE_ANALOG:
            return "RETRO_DEVICE_ANALOG";
        case RETRO_DEVICE_POINTER:
            return "RETRO_DEVICE_POINTER";
        case MelonDsDs::MELONDSDS_DEVICE_JOYPAD_WITH_PHOTOSENSOR:
            return "MELONDSDS_DEVICE_JOYPAD_WITH_PHOTOSENSOR";
        default:
            return "<unknown>";
    }
}

void MelonDsDs::InputState::SetControllerPortDevice(unsigned int port, unsigned int device) noexcept {
    retro::debug("MelonDsDs::InputState::SetControllerPortDevice({}, {})", port, device_name(device));

    _inputDeviceType = device;
    _joypad.SetControllerPortDevice(port, device);
}

bool InputState::IsCursorInputInBounds() const noexcept {
    switch (_touchMode) {
        case TouchMode::Pointer:
            // Finger is touching the screen or the mouse cursor is atop the window
            return _pointer.RawPosition() != i16vec2(0);
        case TouchMode::Joystick:
            // Joystick cursor is constrained to always be on the touch screen
            return true;
        case TouchMode::Auto:
            // If the joystick cursor was last used, it's an automatic true.
            // If the pointer was last used, see if the value is zero
            return (_joypad.LastPointerUpdate() > _pointer.LastPointerUpdate()) || (_pointer.RawPosition() != i16vec2(0));
        default:
            return false;
    }

    // Why are we comparing _pointer.RawPosition() against (0, 0)?
    // libretro's pointer API returns (0, 0) if the pointer is not over the play area, even if it's still over the window.
    // Theoretically means that the cursor will be hidden if the player moves the pointer to the dead center of the screen,
    // but the screen's resolution probably isn't big enough for that to happen in practice.
}

void InputState::Update(const ScreenLayoutData& layout) noexcept {
    ZoneScopedN(TracyFunction);

    retro::input_poll();

    // First get the raw input from libretro itself
    InputPollResult pollResult;

    pollResult.JoypadButtons = retro::joypad_state(0);
    if (_touchMode == TouchMode::Joystick || _touchMode == TouchMode::Auto) {
        // We don't yet know if the player is using the touch screen with a joypad or a pointer,
        // so if the touch mode is Auto then we need to check both.

        pollResult.AnalogCursorDirection = retro::analog_state(0, RETRO_DEVICE_INDEX_ANALOG_RIGHT);
    }

    if (_touchMode == TouchMode::Pointer || _touchMode == TouchMode::Auto) {
        pollResult.PointerPressed = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
        pollResult.PointerPosition.x = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
        pollResult.PointerPosition.y = retro::input_state(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    }
    pollResult.Timestamp = cpu_features_get_perf_counter();

    // Update each device's internal state
    _joypad.Update(pollResult);
    if (_solarSensor) {
        _solarSensor->Update(_joypad);
    }
    _pointer.Update(pollResult);

    _cursor.Update(layout, _pointer, _joypad);
    // TODO: Transform the pointer coordinates using the screen layout and save the result
    // TODO: Instantiate a SolarSensorState or RumbleState based on the contents of Slot-2
}

void InputState::Apply(melonDS::NDS& nds, ScreenLayoutData& layout, MicrophoneState& mic) const noexcept {
    ZoneScopedN(TracyFunction);

    // Adjust the screen layout based on the frontend's input
    _joypad.Apply(layout);

    // Forward the frontend's button input to the emulated DS
    _joypad.Apply(nds);

    // Update the microphone's state
    _joypad.Apply(mic);
    if (_solarSensor) {
        // TODO: Apply joypad state to solar sensor
    }

    _cursor.Apply(nds);
}

void InputState::SetConfig(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    _joypad.SetConfig(config);
    _cursor.SetConfig(config);
    if (_solarSensor) {
        _solarSensor->SetConfig(config);
    }
}

bool MelonDsDs::InputState::CursorVisible() const noexcept {
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
            modeAllowsCursor = _cursor.CursorTimeout() > 0;
            break;
    }

    return modeAllowsCursor && IsCursorInputInBounds();
}

void InputState::RumbleStart(std::chrono::milliseconds len) noexcept {
    if (_rumble) {
        _rumble->RumbleStart(len);
    }
}

void InputState::RumbleStop() noexcept {
    if (_rumble) {
        _rumble->RumbleStop();
    }
}

void melonDS::Platform::Addon_RumbleStart(melonDS::u32 len, void* userdata)
{
    MelonDsDs::CoreState& core = *reinterpret_cast<MelonDsDs::CoreState*>(userdata);
    core.GetInputState().RumbleStart(std::chrono::milliseconds(len));
}

void melonDS::Platform::Addon_RumbleStop(void* userdata)
{
    MelonDsDs::CoreState& core = *reinterpret_cast<MelonDsDs::CoreState*>(userdata);
    core.GetInputState().RumbleStop();
}
