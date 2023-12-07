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

#include "power.hpp"

#include "PlatformOGLPrivate.h"

#include <NDS.h>
#include <DSi.h>
#include <DSi_I2C.h>
#include <SPI.h>
#include <retro_assert.h>

#include "config.hpp"
#include "core.hpp"
#include "environment.hpp"
#include "tracy.hpp"

using namespace melonDS;
using std::optional;

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

retro::task::TaskSpec MelonDsDs::power::PowerStatusUpdateTask() noexcept
{
    retro::task::TaskSpec updatePowerStatus(
        [timeToPowerStatusUpdate=0](retro::task::TaskHandle& task) mutable noexcept {
            ZoneScopedN("MelonDsDs::power::PowerStatusUpdateTask");
            if (!retro::supports_power_status()) {
                // If this frontend or device doesn't support querying the power status...
                task.Finish();
                return;
            }

            if (timeToPowerStatusUpdate > 0) {
                // If we'll be checking the power status soon...
                timeToPowerStatusUpdate--;
            }

            if (timeToPowerStatusUpdate == 0) {
                // If it's time to check the power status...
                retro_assert(MelonDsDs::Core.Console != nullptr);
                NDS& nds = *MelonDsDs::Core.Console;
                if (optional<retro_device_power> devicePower = retro::get_device_power()) {
                    // ...and the check succeeded...
                    bool charging = devicePower->state == RETRO_POWERSTATE_CHARGING || devicePower->state == RETRO_POWERSTATE_PLUGGED_IN;
                    switch (static_cast<ConsoleType>(nds.ConsoleType)) {
                        case ConsoleType::DS: {
                            // If the threshold is 0, the battery level is always okay
                            // If the threshold is 100, the battery level is never okay
                            bool ok = charging || static_cast<unsigned>(devicePower->percent) > config::system::DsPowerOkayThreshold();
                            retro_assert(nds.SPI.GetPowerMan() != nullptr);
                            nds.SPI.GetPowerMan()->SetBatteryLevelOkay(ok);
                            break;
                        }
                        case ConsoleType::DSi: {
                            DSi& dsi = static_cast<DSi&>(nds);
                            u8 batteryLevel = GetDsiBatteryLevel(devicePower->percent == RETRO_POWERSTATE_NO_ESTIMATE ? 100 : devicePower->percent);
                            retro_assert(dsi.I2C.GetBPTWL() != nullptr);
                            dsi.I2C.GetBPTWL()->SetBatteryCharging(charging);
                            dsi.I2C.GetBPTWL()->SetBatteryLevel(batteryLevel);
                            break;
                        }
                    }
                } else {
                    retro::warn("Failed to get device power status\n");
                }

                timeToPowerStatusUpdate = config::system::PowerUpdateInterval() * 60; // Reset the timer
            }
        },
        nullptr,
        nullptr,
        retro::task::ASAP,
        "PowerStatusUpdateTask"
    );

    return updatePowerStatus;
}