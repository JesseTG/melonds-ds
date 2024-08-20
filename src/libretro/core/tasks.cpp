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

#include <file/file_path.h>
#include <retro_assert.h>
#include <streams/file_stream.h>


#include "../config/config.hpp"
#include "core.hpp"
#include "environment.hpp"
#include "microphone.hpp"
#include "retro/task_queue.hpp"
#include "tracy.hpp"

using namespace melonDS;

using std::nullopt;
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


void MelonDsDs::CoreState::FlushFirmware(string_view firmwarePath, string_view wfcSettingsPath) noexcept {
    ZoneScopedN(TracyFunction);

    retro_assert(!firmwarePath.empty());
    retro_assert(path_is_absolute(firmwarePath.data()));
    retro_assert(!wfcSettingsPath.empty());
    retro_assert(path_is_absolute(wfcSettingsPath.data()));
    retro_assert(Console != nullptr);

    const Firmware& firmware = Console->GetFirmware();

    retro_assert(firmware.Buffer() != nullptr);

    if (firmware.GetHeader().Identifier != GENERATED_FIRMWARE_IDENTIFIER) {
        // If this is a native firmware blob...
        int32_t existingFirmwareFileSize = path_get_size(firmwarePath.data());
        if (existingFirmwareFileSize == -1) {
            retro::warn("Expected firmware \"{}\" to exist before updating, but it doesn't", firmwarePath);
        }
        else if (existingFirmwareFileSize != firmware.Length()) {
            retro::warn(
                "In-memory firmware is {} bytes, but destination file \"{}\" has {} bytes",
                firmware.Length(),
                firmwarePath,
                existingFirmwareFileSize
            );
        }

        retro_assert(firmwarePath.rfind("//notfound") == std::string_view::npos);
        Firmware firmwareCopy(firmware);
        // TODO: Apply the original values of the settings that were overridden
        if (filestream_write_file(firmwarePath.data(), firmware.Buffer(), firmware.Length())) {
            // ...then write the whole thing back.
            retro::debug("Flushed {}-byte firmware to \"{}\"", firmware.Length(), firmwarePath);
        }
        else {
            retro::error("Failed to write {}-byte firmware to \"{}\"", firmware.Length(), firmwarePath);
        }
    }
    else {
        constexpr int32_t expectedWfcSettingsSize = sizeof(firmware.GetExtendedAccessPoints()) + sizeof(firmware.GetAccessPoints());
        int32_t existingWfcSettingsSize = path_get_size(wfcSettingsPath.data());
        if (existingWfcSettingsSize == -1) {
            retro::debug("Wi-Fi settings file at \"{}\" doesn't exist, creating it", wfcSettingsPath);
        }
        else if (existingWfcSettingsSize != expectedWfcSettingsSize) {
            retro::warn(
                "In-memory WFC settings is {} bytes, but destination file \"{}\" has {} bytes",
                expectedWfcSettingsSize,
                wfcSettingsPath,
                existingWfcSettingsSize
            );
        }
        retro_assert(wfcSettingsPath.rfind("/wfcsettings.bin") != std::string_view::npos);
        u32 eapstart = firmware.GetExtendedAccessPointOffset();
        u32 eapend = eapstart + sizeof(firmware.GetExtendedAccessPoints());
        u32 apstart = firmware.GetWifiAccessPointOffset();

        // assert that the extended access points come just before the regular ones
        retro_assert(eapend == apstart);

        const u8* buffer = firmware.GetExtendedAccessPointPosition();
        if (filestream_write_file(wfcSettingsPath.data(), buffer, expectedWfcSettingsSize)) {
            retro::debug("Flushed {}-byte WFC settings to \"{}\"", expectedWfcSettingsSize, wfcSettingsPath);
        }
        else {
            retro::error("Failed to write {}-byte WFC settings to \"{}\"", expectedWfcSettingsSize, wfcSettingsPath);
        }
    }
}

// This task keeps running for the lifetime of the task queue.
retro::task::TaskSpec MelonDsDs::CoreState::FlushGbaSramTask() noexcept {
    ZoneScopedN(TracyFunction);
    return {
        [this](retro::task::TaskHandle &task) noexcept {
            if (!_gbaSaveInfo) {
                task.Finish();
                return;
            }

            if (_timeToGbaFlush != nullopt && (*_timeToGbaFlush)-- <= 0) {
                // If it's time to flush the GBA's SRAM...
                retro::debug("GBA SRAM flush timer expired, flushing save data now");
                FlushGbaSram(*_gbaSaveInfo);
                _timeToGbaFlush = nullopt; // Reset the timer
            }
        },
        nullptr,
        [this](retro::task::TaskHandle& task) noexcept {
            if (_gbaSaveInfo) {
                FlushGbaSram(*_gbaSaveInfo);
                _timeToGbaFlush = nullopt;
            }
        },
        retro::task::ASAP,
        "GBA SRAM Flush"
    };
}

void MelonDsDs::CoreState::FlushGbaSram(const retro::GameInfo& gbaSaveInfo) noexcept {
    ZoneScopedN(TracyFunction);

    auto save_data_path = gbaSaveInfo.GetPath();
    if (save_data_path.empty() || !_gbaSaveManager) {
        // No save data path was provided, or the GBA save manager isn't initialized
        return; // TODO: Report this error
    }
    const u8* gba_sram = _gbaSaveManager->Sram();
    u32 gba_sram_length = _gbaSaveManager->SramLength();

    if (gba_sram == nullptr || gba_sram_length == 0) {
        return; // TODO: Report this error
    }

    if (!filestream_write_file(save_data_path.data(), gba_sram, gba_sram_length)) {
        retro::error("Failed to write {}-byte GBA SRAM to \"{}\"", gba_sram_length, save_data_path);
        // TODO: Report this to the user
    } else {
        retro::debug("Flushed {}-byte GBA SRAM to \"{}\"", gba_sram_length, save_data_path);
    }
}


retro::task::TaskSpec MelonDsDs::CoreState::FlushFirmwareTask(string_view firmwareName) noexcept {
    ZoneScopedN(TracyFunction);
    optional<string> firmwarePath = retro::get_system_path(firmwareName);
    if (!firmwarePath) {
        retro::error("Failed to get system path for firmware named \"{}\", firmware changes won't be saved.",
                     firmwareName);
        return {};
    }

    string_view wfcSettingsName = Config.GeneratedFirmwareSettingsPath();
    optional<string> wfcSettingsPath = retro::get_system_subdir_path(wfcSettingsName);
    if (!wfcSettingsPath) {
        retro::error("Failed to get system path for WFC settings at \"{}\", firmware changes won't be saved.",
                     wfcSettingsName);
        return {};
    }

    return retro::task::TaskSpec(
        [this, firmwarePath=*firmwarePath, wfcSettingsPath=*wfcSettingsPath](retro::task::TaskHandle&) noexcept {
            if (_timeToFirmwareFlush != nullopt && (*_timeToFirmwareFlush)-- <= 0) {
                // If it's time to flush the firmware...
                retro::debug("Firmware flush timer expired, flushing data now");
                FlushFirmware(firmwarePath, wfcSettingsPath);
                _timeToFirmwareFlush = nullopt; // Reset the timer
            }
        },
        nullptr,
        [this, path=*firmwarePath, wfcSettingsPath=*wfcSettingsPath](retro::task::TaskHandle&) noexcept {
            FlushFirmware(path, wfcSettingsPath);
            _timeToFirmwareFlush = nullopt;
        },
        retro::task::ASAP,
        "Firmware Flush"
    );
}

retro::task::TaskSpec MelonDsDs::CoreState::OnScreenDisplayTask() noexcept {
    ZoneScopedN(TracyFunction);
    return retro::task::TaskSpec(
        [this](retro::task::TaskHandle&) noexcept {
            using std::to_string;
            ZoneScopedN(TracyFunction);

            retro_assert(Console != nullptr);
            NDS& nds = *Console;

            // TODO: If an on-screen display isn't supported, finish the task
            fmt::memory_buffer buf;
            auto inserter = std::back_inserter(buf);

            if (Config.ShowPointerCoordinates()) {
                i16vec2 pointerInput = _inputState.PointerInput();
                ivec2 joystick = _inputState.JoystickTouchPosition();
                ivec2 touch = _inputState.PointerTouchPosition();
                fmt::format_to(
                    inserter,
                    "Pointer: ({}, {}) → ({}, {}) || Joystick: ({}, {})",
                    pointerInput.x, pointerInput.y,
                    touch.x, touch.y,
                    joystick.x, joystick.y
                );
            }

            if (Config.ShowMicState() && _micState.IsHostMicActive()) {
                // If the microphone is open and turned on...
                fmt::format_to(
                    inserter,
                    "{}{}",
                    buf.size() == 0 ? "" : OSD_DELIMITER,
                    (nds.NumFrames % 120 > 60) ? "●" : "○"
                );
                // Toggle between a filled circle and an empty one every second
                // (kind of like a blinking "recording" light)
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
