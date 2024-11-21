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
#include <GBACart.h>
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
#include "rumble.hpp"
#include "screenlayout.hpp"
#include "solar.hpp"
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
using std::get_if;

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
        default:
            return "<unknown>";
    }
}

void MelonDsDs::InputState::SetControllerPortDevice(unsigned int port, unsigned int device) noexcept {
    retro::debug("MelonDsDs::InputState::SetControllerPortDevice({}, {})", port, device_name(device));

    _inputDeviceType = device;
    _joypad.SetControllerPortDevice(port, device);
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
    if (auto* solar = get_if<SolarSensorState>(&_slot2)) {
        solar->Update(_joypad);
    }
    _pointer.Update(pollResult);

    _cursor.Update(layout, _pointer, _joypad);
}

void InputState::Apply(melonDS::NDS& nds, ScreenLayoutData& layout, MicrophoneState& mic) const noexcept {
    ZoneScopedN(TracyFunction);

    // Adjust the screen layout based on the frontend's input
    _joypad.Apply(layout);

    // Forward the frontend's button input to the emulated DS
    _joypad.Apply(nds);

    // Update the microphone's state
    _joypad.Apply(mic);
    if (const auto* solar = get_if<SolarSensorState>(&_slot2)) {
        solar->Apply(nds);
    }

    _cursor.Apply(nds);
}

void InputState::SetConfig(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    _joypad.SetConfig(config);
    _cursor.SetConfig(config);
    _pointer.SetConfig(config);
    if (auto* solar = std::get_if<SolarSensorState>(&_slot2)) {
        solar->SetConfig(config);
    }

    // RumbleState doesn't have any internal config right now
    // (but if it does, do the same as with SolarSensorState)
}

void InputState::SetSlot2Input(const melonDS::GBACart::CartCommon& gbacart) noexcept {
    switch (gbacart.Type()) {
        case melonDS::GBACart::CartType::GameSolarSensor:
            _slot2 = SolarSensorState(0);
            retro::debug("Enabled SolarSensorState");
            break;
        case melonDS::GBACart::CartType::RumblePak:
            _slot2 = RumbleState();
            retro::debug("Enabled RumbleState");
            break;
        default:
            // No GBA cart, or it's a plain game, or it's a peripheral unrelated to input
            _slot2 = std::monostate(); // "no relevant device"
            break;
    }
}

void InputState::RumbleStart(std::chrono::milliseconds len) noexcept {
    if (auto* rumble = get_if<RumbleState>(&_slot2)) {
        rumble->RumbleStart(len);
    }
}

void InputState::RumbleStop() noexcept {
    if (auto* rumble = get_if<RumbleState>(&_slot2)) {
        rumble->RumbleStop();
    }
}

void melonDS::Platform::Addon_RumbleStart(melonDS::u32 len, void* userdata)
{
    ZoneScopedN(TracyFunction);
    MelonDsDs::CoreState& core = *reinterpret_cast<MelonDsDs::CoreState*>(userdata);
    core.GetInputState().RumbleStart(std::chrono::milliseconds(len));
}

void melonDS::Platform::Addon_RumbleStop(void* userdata)
{
    ZoneScopedN(TracyFunction);
    MelonDsDs::CoreState& core = *reinterpret_cast<MelonDsDs::CoreState*>(userdata);
    core.GetInputState().RumbleStop();
}
