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

#include "core.hpp"

#include <NDS.h>
#include <DSi.h>
#include <DSi_I2C.h>
#include <SPI.h>

#include <glm/vec2.hpp>
#include <retro_assert.h>


#include "config.hpp"
#include "core.hpp"
#include "environment.hpp"
#include "microphone.hpp"
#include "retro/task_queue.hpp"
#include "tracy.hpp"

using namespace melonDS;

using std::optional;
using glm::ivec2;
using glm::i16vec2;

constexpr const char* const OSD_DELIMITER = " || ";
constexpr const char* const OSD_YES = "✔";
constexpr const char* const OSD_NO = "✘";

static u8 GetDsiBatteryLevel(u8 percent) noexcept {
    u8 level = std::round(percent / 25.0f); // Round the percent from 0 to 4
    switch (level) {
        case 0:
            // The DSi sends a shutdown signal when the battery runs out;
            // that would result in the core suddenly quitting, which we don't want.
            // So the battery level will never actually be reported as empty.
            return DSi_BPTWL::batteryLevel_AlmostEmpty;
        case 1:
            return DSi_BPTWL::batteryLevel_Low;
        case 2:
            return DSi_BPTWL::batteryLevel_Half;
        case 3:
            return DSi_BPTWL::batteryLevel_ThreeQuarters;
        default:
            return DSi_BPTWL::batteryLevel_Full;
    }
}

retro::task::TaskSpec MelonDsDs::CoreState::PowerStatusUpdateTask() noexcept {
    ZoneScopedN(TracyFunction);
    retro::task::TaskSpec updatePowerStatus(
        [this, timeToPowerStatusUpdate=0](retro::task::TaskHandle& task) mutable noexcept {
            ZoneScopedN(TracyFunction);
            if (!retro::supports_power_status()) {
                // If this frontend or device doesn't support querying the power status...
                task.Finish();
                return;
            }

            if (timeToPowerStatusUpdate > 0) {
                // If we'll be checking the power status soon...
                timeToPowerStatusUpdate--;
            }

            if (timeToPowerStatusUpdate != 0) {
                // If it's not yet time to check the power status...
                return;
            }

            if (Console == nullptr)
                return;

            retro_assert(Console != nullptr);
            if (optional<retro_device_power> devicePower = retro::get_device_power()) {
                // ...and the check succeeded...
                bool charging =
                    devicePower->state == RETRO_POWERSTATE_CHARGING ||
                    devicePower->state == RETRO_POWERSTATE_PLUGGED_IN;

                switch (static_cast<ConsoleType>(Console->ConsoleType)) {
                    case ConsoleType::DS: {
                        // If the threshold is 0, the battery level is always okay
                        // If the threshold is 100, the battery level is never okay
                        bool ok =
                            charging ||
                            static_cast<unsigned>(devicePower->percent) > Config.DsPowerOkayThreshold();

                        retro_assert(Console->SPI.GetPowerMan() != nullptr);
                        Console->SPI.GetPowerMan()->SetBatteryLevelOkay(ok);
                        break;
                    }
                    case ConsoleType::DSi: {
                        DSi& dsi = *static_cast<DSi*>(Console.get());
                        u8 percent = devicePower->percent == RETRO_POWERSTATE_NO_ESTIMATE ? 100 : devicePower->percent;
                        u8 batteryLevel = GetDsiBatteryLevel(percent);
                        retro_assert(dsi.I2C.GetBPTWL() != nullptr);
                        dsi.I2C.GetBPTWL()->SetBatteryCharging(charging);
                        dsi.I2C.GetBPTWL()->SetBatteryLevel(batteryLevel);
                        break;
                    }
                }
            }
            else {
                retro::warn("Failed to get device power status\n");
            }

            timeToPowerStatusUpdate = Config.PowerUpdateInterval() * 60; // Reset the timer
        },
        nullptr,
        nullptr,
        retro::task::ASAP,
        "PowerStatusUpdateTask"
    );

    return updatePowerStatus;
}

retro::task::TaskSpec MelonDsDs::CoreState::OnScreenDisplayTask() noexcept {
    ZoneScopedN(TracyFunction);
    return retro::task::TaskSpec(
        [this](retro::task::TaskHandle&) noexcept {
            using std::to_string;
            ZoneScopedN(TracyFunction);

            retro_assert(MelonDsDs::Core.Console != nullptr);
            NDS& nds = *MelonDsDs::Core.Console;

            // TODO: If an on-screen display isn't supported, finish the task
            fmt::memory_buffer buf;
            auto inserter = std::back_inserter(buf);

            if (Config.ShowPointerCoordinates()) {
                i16vec2 pointerInput = _inputState.PointerInput();
                ivec2 joystick = _inputState.JoystickTouchPosition();
                ivec2 touch = _inputState.PointerTouchPosition();
                format_to(
                    inserter,
                    "Pointer: ({}, {}) → ({}, {}) || Joystick: ({}, {})",
                    pointerInput.x, pointerInput.y,
                    touch.x, touch.y,
                    joystick.x, joystick.y
                );
            }

            if (Config.ShowMicState()) {
                optional<bool> mic_state = retro::microphone::get_state();

                if (mic_state && *mic_state) {
                    // If the microphone is open and turned on...
                    fmt::format_to(
                        inserter,
                        "{}{}",
                        buf.size() == 0 ? "" : OSD_DELIMITER,
                        (nds.NumFrames % 120 > 60) ? "●" : "○"
                    );
                    // Toggle between a filled circle and an empty one every 1.5 seconds
                    // (kind of like a blinking "recording" light)
                }
            }

            if (Config.ShowCurrentLayout()) {
                fmt::format_to(
                    inserter,
                    "{}Layout {}/{}",
                    buf.size() == 0 ? "" : OSD_DELIMITER,
                    _screenLayout.LayoutIndex() + 1,
                    _screenLayout.NumberOfLayouts()
                );
            }

            if (Config.ShowLidState() && nds.IsLidClosed()) {
                fmt::format_to(
                    inserter,
                    "{}Closed",
                    buf.size() == 0 ? "" : OSD_DELIMITER
                );
            }

            // fmt::format_to does not append a null terminator
            buf.push_back('\0');


            if (buf.size() > 0) {
                retro_message_ext message {
                    .msg = buf.data(),
                    .duration = 60,
                    .priority = 0,
                    .level = RETRO_LOG_DEBUG,
                    .target = RETRO_MESSAGE_TARGET_OSD,
                    .type = RETRO_MESSAGE_TYPE_STATUS,
                    .progress = -1
                };
                retro::set_message(message);
            }
        },
        nullptr,
        nullptr,
        retro::task::ASAP,
        "OnScreenDisplayTask"
    );
}
