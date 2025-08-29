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

#include "config.hpp"
// NOT UNUSED; GPU.h doesn't #include OpenGL, so I do it here.
// This must come before <GPU.h>!
#include "PlatformOGLPrivate.h"

#include <algorithm>
#include <charconv>
#include <codecvt>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <span>
#include <random>
#include <string_view>
#include <utility>
#include <vector>
#include <sys/stat.h>

#include <file/file_path.h>
#include <streams/file_stream.h>
#include <libretro.h>
#include <retro_assert.h>
#include <string/stdstring.h>
#undef isnan
#include <fmt/ranges.h>

#include <Args.h>
#include <DSi.h>
#include <DSi_NAND.h>
#include <NDS.h>
#include <GPU.h>
#include <SPU.h>
#include <SPI.h>
#include <SPI_Firmware.h>
#include <FreeBIOS.h>
#include <file/config_file.h>
#include <Net_PCap.h>

#include "config/constants.hpp"
#include "config/definitions.hpp"
#include "config/definitions/categories.hpp"
#include "../core/core.hpp"
#include "embedded/melondsds_default_wfc_config.h"
#include "environment.hpp"
#include "exceptions.hpp"
#include "format.hpp"
#include "input/input.hpp"
#include "libretro.hpp"
#include "microphone.hpp"
#include "net/pcap.hpp"
#include "retro/dirent.hpp"
#include "screenlayout.hpp"
#include "std/span.hpp"
#include "tracy.hpp"

#ifdef interface
#undef interface
#endif

using namespace melonDS;
using std::array;
using std::find_if;
using std::from_chars;
using std::from_chars_result;
using std::initializer_list;
using std::make_optional;
using std::make_unique;
using std::nullopt;
using std::optional;
using std::pair;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;

constexpr unsigned AUTO_SDCARD_SIZE = 0;
constexpr uint64_t DEFAULT_SDCARD_SIZE = 4096ull * 1024ull * 1024ull; // 4GB
const char* const DEFAULT_HOMEBREW_SDCARD_IMAGE_NAME = "dldi_sd_card.bin";
const char* const DEFAULT_HOMEBREW_SDCARD_DIR_NAME = "dldi_sd_card";
const char* const DEFAULT_DSI_SDCARD_IMAGE_NAME = "dsi_sd_card.bin";
const char* const DEFAULT_DSI_SDCARD_DIR_NAME = "dsi_sd_card";

const initializer_list<unsigned> CURSOR_TIMEOUTS = {1, 2, 3, 5, 10, 15, 20, 30, 60};
const initializer_list<int> JOYSTICK_CURSOR_DEADZONES = {0, 5, 10, 15, 20, 25, 30, 35};
const initializer_list<int> JOYSTICK_CURSOR_MAXSPEEDS = {1, 2, 3, 4, 5, 6, 7, 8, 9};
const initializer_list<int> JOYSTICK_CURSOR_RESPONSES = {100, 200};
const initializer_list<int> JOYSTICK_CURSOR_SPEEDUPS = {33, 50, 66, 150, 200, 250, 300};
const initializer_list<unsigned> DS_POWER_OK_THRESHOLDS = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
const initializer_list<unsigned> POWER_UPDATE_INTERVALS = {1, 2, 3, 5, 10, 15, 20, 30, 60};
const initializer_list<uint16_t> RUMBLE_INTENSITY_VALUES = {0, 6554, 13107, 19661, 26214, 32768, 39321, 45875, 52428, 58982, 65535};
const initializer_list<int> RELATIVE_DAY_OFFSETS = {
    -364, -180, -150, -120, -90, -60, -30, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 30, 60, 90, 120, 150, 180, 364
};

namespace MelonDsDs::config {
    static void ParseSystemOptions(CoreConfig& config) noexcept;
    static void ParseTimeOptions(CoreConfig& config) noexcept;
    static void ParseOsdOptions(CoreConfig& config) noexcept;
    static void ParseJitOptions(CoreConfig& config) noexcept;
    static void ParseHomebrewSaveOptions(CoreConfig& config) noexcept;
    static void ParseDsiStorageOptions(CoreConfig& config) noexcept;
    static void ParseFirmwareOptions(CoreConfig& config) noexcept;
    static void ParseAudioOptions(CoreConfig& config) noexcept;
    static void ParseNetworkOptions(CoreConfig& config) noexcept;
    static void ParseScreenOptions(CoreConfig& config) noexcept;
    static void ParseVideoOptions(CoreConfig& config) noexcept;

}

namespace MelonDsDs::config::definitions {
    // If I mess up an option definition,
    // the following static_asserts should catch it.
    constexpr static bool AreOptionKeysUnique() {
        for (size_t i = 0; i < CoreOptionDefinitions.size(); ++i) {
            for (size_t j = i + 1; j < CoreOptionDefinitions.size(); ++j) {
                if (CoreOptionDefinitions[i].key == CoreOptionDefinitions[j].key) {
                    return false;
                }
            }
        }
        return true;
    }

    static_assert(
        CoreOptionDefinitions[CoreOptionDefinitions.size() - 1].key == nullptr,
        "CoreOptionDefinitions must end with a null key"
    );

#ifndef __clang__
    // Work around a clang bug (can't compare pointers for some reason)
    static_assert(AreOptionKeysUnique());
#endif
}

void MelonDsDs::ParseConfig(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    config::ParseSystemOptions(config);
    config::ParseTimeOptions(config);
    config::ParseOsdOptions(config);
    config::ParseJitOptions(config);
    config::ParseHomebrewSaveOptions(config);
    config::ParseDsiStorageOptions(config);
    config::ParseFirmwareOptions(config);
    config::ParseAudioOptions(config);
    config::ParseNetworkOptions(config);
    config::ParseScreenOptions(config);
    config::ParseVideoOptions(config);
}

static void MelonDsDs::config::ParseSystemOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::system;
    using retro::get_variable;

    // All of these options take effect when a game starts, so there's no need to update them mid-game

    if (optional<MelonDsDs::ConsoleType> type = ParseConsoleType(get_variable(CONSOLE_MODE))) {
        config.SetConsoleType(*type);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CONSOLE_MODE, values::DS);
        config.SetConsoleType(ConsoleType::DS);
    }

    if (optional<MelonDsDs::Slot2Device> type = ParseSlot2Device(get_variable(SLOT2_DEVICE))) {
        config.SetSlot2Device(*type);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SLOT2_DEVICE, values::AUTO);
        config.SetSlot2Device(Slot2Device::Auto);
    }

    if (optional<BootMode> value = ParseBootMode(get_variable(BOOT_MODE))) {
        config.SetBootMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", BOOT_MODE, values::NATIVE);
        config.SetBootMode(BootMode::Direct);
    }

    if (optional<SysfileMode> value = ParseSysfileMode(get_variable(SYSFILE_MODE))) {
        config.SetSysfileMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SYSFILE_MODE, values::BUILT_IN);
        config.SetSysfileMode(SysfileMode::BuiltIn);
    }

    if (optional<bool> value = ParseBoolean(get_variable(SOLAR_SENSOR_HOST_SENSOR))) {
        config.SetUseRealLightSensor(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SOLAR_SENSOR_HOST_SENSOR, values::SENSOR);
        config.SetUseRealLightSensor(true);
    }

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(DS_POWER_OK), DS_POWER_OK_THRESHOLDS)) {
        config.SetDsPowerOkayThreshold(*value);
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to 20%", DS_POWER_OK);
        config.SetDsPowerOkayThreshold(20);
    }

    if (optional<unsigned> value = ParseIntegerInList(get_variable(BATTERY_UPDATE_INTERVAL), POWER_UPDATE_INTERVALS)) {
        config.SetPowerUpdateInterval(*value);
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to 15 seconds", BATTERY_UPDATE_INTERVAL);
        config.SetPowerUpdateInterval(15);
    }
}

void MelonDsDs::config::ParseTimeOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::time;
    using retro::get_variable;

    if (optional<StartTimeMode> value = ParseStartTimeMode(get_variable(definitions::StartTimeMode.key))) {
        config.SetStartTimeMode(*value);
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::StartTimeMode.key, definitions::StartTimeMode.default_value);
        config.SetStartTimeMode(*ParseStartTimeMode(definitions::StartTimeMode.default_value));
    }

    if (optional<int> value = ParseIntegerInRange(get_variable(definitions::RelativeYearOffset.key), -20, 20)) {
        config.SetRelativeYearOffset(years(*value));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::RelativeYearOffset.key, 0);
        config.SetRelativeYearOffset({});
    }

    if (optional<int> value = ParseIntegerInList(get_variable(definitions::RelativeDayOffset.key), RELATIVE_DAY_OFFSETS)) {
        config.SetRelativeDayOffset(days(*value));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::RelativeDayOffset.key, 0);
        config.SetRelativeDayOffset({});
    }

    if (optional<int> value = ParseIntegerInRange(get_variable(definitions::RelativeHourOffset.key), -23, 23)) {
        config.SetRelativeHourOffset(hours(*value));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::RelativeHourOffset.key, 0);
        config.SetRelativeHourOffset({});
    }

    if (optional<int> value = ParseIntegerInRange(get_variable(definitions::RelativeMinuteOffset.key), -59, 59)) {
        config.SetRelativeMinuteOffset(minutes(*value));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::RelativeMinuteOffset.key, 0);
        config.SetRelativeMinuteOffset({});
    }

    // TODO: Get the current time and use its components as the default if getting the variables fails
    if (optional<unsigned> value = ParseIntegerInRange(get_variable(definitions::AbsoluteYear.key), 2000, 2100)) {
        config.SetAbsoluteStartYear(year(*value));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::AbsoluteYear.key, 0);
        config.SetRelativeMinuteOffset({});
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(definitions::AbsoluteMonth.key), 1, 12)) {
        config.SetAbsoluteStartMonth(month(*value));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::AbsoluteMonth.key, 0);
        config.SetAbsoluteStartMonth(month(0));
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(definitions::AbsoluteDay.key), 1, 31)) {
        config.SetAbsoluteStartDay(day(*value));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::AbsoluteDay.key, 0);
        config.SetAbsoluteStartDay(day(0));
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(definitions::AbsoluteHour.key), 0, 23)) {
        config.SetAbsoluteStartHour(hh_mm_ss(hours(*value)));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::AbsoluteHour.key, 0);
        config.SetAbsoluteStartHour(hh_mm_ss(hours(0)));
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(definitions::AbsoluteMinute.key), 0, 59)) {
        config.SetAbsoluteStartMinute(hh_mm_ss(minutes(*value)));
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to {}", definitions::AbsoluteMinute.key, 0);
        config.SetAbsoluteStartMinute(hh_mm_ss(minutes(0)));
    }
}


void MelonDsDs::config::ParseOsdOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::osd;
    using retro::get_variable;

#ifndef NDEBUG
    if (optional<bool> value = ParseBoolean(get_variable(osd::POINTER_COORDINATES))) {
        config.SetShowPointerCoordinates(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", POINTER_COORDINATES, values::DISABLED);
        config.SetShowPointerCoordinates(false);
    }
#endif

    if (optional<bool> value = ParseBoolean(get_variable(osd::UNSUPPORTED_FEATURES))) {
        config.SetShowUnsupportedFeatureWarnings(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", UNSUPPORTED_FEATURES, values::ENABLED);
        config.SetShowUnsupportedFeatureWarnings(true);
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::MIC_STATE))) {
        config.SetShowMicState(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", MIC_STATE, values::ENABLED);
        config.SetShowMicState(true);
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::CAMERA_STATE))) {
        config.SetShowCameraState(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CAMERA_STATE, values::ENABLED);
        config.SetShowCameraState(true);
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::BIOS_WARNINGS))) {
        config.SetShowBiosWarnings(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", BIOS_WARNINGS, values::ENABLED);
        config.SetShowBiosWarnings(true);
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::CURRENT_LAYOUT))) {
        config.SetShowCurrentLayout(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CURRENT_LAYOUT, values::ENABLED);
        config.SetShowCurrentLayout(true);
    }

    if (optional<bool> value = MelonDsDs::ParseBoolean(get_variable(osd::LID_STATE))) {
        config.SetShowLidState(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", LID_STATE, values::DISABLED);
        config.SetShowLidState(false);
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::SENSOR_READING))) {
        config.SetShowSensorReading(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SENSOR_READING, definitions::ShowSensorReading.default_value);
        config.SetShowSensorReading(true);
    }
}

static void MelonDsDs::config::ParseJitOptions(CoreConfig& config) noexcept {
#ifdef HAVE_JIT
    ZoneScopedN(TracyFunction);
    using retro::get_variable;

    if (optional<bool> value = ParseBoolean(get_variable(cpu::JIT_ENABLE))) {
        config.SetJitEnable(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_ENABLE, values::ENABLED);
        config.SetJitEnable(true);
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(cpu::JIT_BLOCK_SIZE), 1, 32)) {
        config.SetMaxBlockSize(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to 32", cpu::JIT_BLOCK_SIZE);
        config.SetMaxBlockSize(32);
    }

    if (optional<bool> value = ParseBoolean(get_variable(cpu::JIT_BRANCH_OPTIMISATIONS))) {
        config.SetBranchOptimizations(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_BRANCH_OPTIMISATIONS, values::ENABLED);
        config.SetBranchOptimizations(true);
    }

    if (optional<bool> value = ParseBoolean(get_variable(cpu::JIT_LITERAL_OPTIMISATIONS))) {
        config.SetLiteralOptimizations(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_LITERAL_OPTIMISATIONS, values::ENABLED);
        config.SetLiteralOptimizations(true);
    }

#ifdef HAVE_JIT_FASTMEM
    if (optional<bool> value = ParseBoolean(get_variable(cpu::JIT_FAST_MEMORY))) {
        config.SetFastMemory(*value);
    } else {
#ifdef NDEBUG
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_FAST_MEMORY, values::ENABLED);
        config.SetFastMemory(true);
#else
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_FAST_MEMORY, values::DISABLED);
        config.SetFastMemory(false);
#endif
    }
#endif
#endif
}

static void MelonDsDs::config::ParseHomebrewSaveOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using retro::get_variable;

    optional<string_view> save_directory = retro::get_save_subdirectory();
    if (!save_directory) {
        config.SetDldiEnable(false);
        retro::error("Failed to get save directory; disabling homebrew SD card");
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::HOMEBREW_READ_ONLY))) {
        config.SetDldiReadOnly(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::HOMEBREW_READ_ONLY, values::DISABLED);
        config.SetDldiReadOnly(false);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::HOMEBREW_SYNC_TO_HOST))) {
        config.SetDldiFolderSync(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::HOMEBREW_SYNC_TO_HOST, values::ENABLED);
        config.SetDldiFolderSync(true);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::HOMEBREW_SAVE_MODE))) {
        config.SetDldiEnable(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::HOMEBREW_SAVE_MODE, values::DISABLED);
        config.SetDldiEnable(false);
    }

    {
        optional<string> imagePath = retro::get_save_subdir_path(DEFAULT_HOMEBREW_SDCARD_IMAGE_NAME);
        config.SetDldiImagePath(std::move(*imagePath));

        optional<string> syncDir = retro::get_save_subdir_path(DEFAULT_HOMEBREW_SDCARD_DIR_NAME);
        config.SetDldiFolderPath(std::move(*syncDir));

        if (path_is_valid(config.DldiImagePath().data())) {
            // If the SD card image exists...
            retro::info("Using existing homebrew SD card image \"{}\"", config.DldiImagePath());
            config.SetDldiImageSize(AUTO_SDCARD_SIZE);
        } else {
            retro::info("No homebrew SD card image found at \"{}\"; will create an image.", config.DldiImagePath());
            config.SetDldiImageSize(DEFAULT_SDCARD_SIZE);
        }
    }
}

static void MelonDsDs::config::ParseDsiStorageOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using retro::get_variable;

    if (optional<bool> value = ParseBoolean(get_variable(storage::DSI_SD_READ_ONLY))) {
        config.SetDsiSdReadOnly(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::DSI_SD_READ_ONLY, values::DISABLED);
        config.SetDsiSdReadOnly(false);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::DSI_SD_SYNC_TO_HOST))) {
        config.SetDsiSdFolderSync(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::DSI_SD_SYNC_TO_HOST, values::ENABLED);
        config.SetDsiSdFolderSync(true);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::DSI_SD_SAVE_MODE))) {
        config.SetDsiSdEnable(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::DSI_SD_SAVE_MODE, values::ENABLED);
        config.SetDsiSdEnable(true);
    }

    {
        optional<string> imagePath = retro::get_save_subdir_path(DEFAULT_DSI_SDCARD_IMAGE_NAME);
        config.SetDsiSdImagePath(std::move(*imagePath));

        optional<string> syncDir = retro::get_save_subdir_path(DEFAULT_DSI_SDCARD_DIR_NAME);
        config.SetDsiSdFolderPath(std::move(*syncDir));


        if (path_is_valid(config.DsiSdImagePath().data())) {
            // If the SD card image exists...
            retro::info("Using existing DSi SD card image \"{}\"", config.DsiSdImagePath());
            config.SetDsiSdImageSize(AUTO_SDCARD_SIZE);
        } else {
            retro::info("No DSi SD card image found at \"{}\"; will create an image.", config.DsiSdImagePath());
            config.SetDsiSdImageSize(DEFAULT_SDCARD_SIZE);
        }
    }

    // If these firmware/BIOS files don't exist, an exception will be thrown later
    if (string_view value = get_variable(storage::DSI_NAND_PATH); !value.empty()) {
        config.SetDsiNandPath(value);
    } else {
        retro::warn("Failed to get value for {}", storage::DSI_NAND_PATH);
        config.SetDsiNandPath(string_view(values::NOT_FOUND));
    }

    if (string_view value = get_variable(system::FIRMWARE_PATH); !value.empty()) {
        config.SetFirmwarePath(value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to built-in firmware", system::FIRMWARE_PATH);
        config.SetFirmwarePath(string_view(values::NOT_FOUND));
    }

    if (string_view value = get_variable(system::FIRMWARE_DSI_PATH); !value.empty()) {
        config.SetDsiFirmwarePath(value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to built-in firmware", system::FIRMWARE_DSI_PATH);
        config.SetDsiFirmwarePath(string_view(values::NOT_FOUND));
    }
}

static void MelonDsDs::config::ParseFirmwareOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using retro::get_variable;

    if (optional<MelonDsDs::FirmwareLanguage> value = ParseLanguage(get_variable(firmware::LANGUAGE))) {
        config.SetLanguage(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::LANGUAGE);
        config.SetLanguage(FirmwareLanguage::Default);
    }

    if (string_view value = get_variable(firmware::FAVORITE_COLOR); value == values::DEFAULT) {
        config.SetFavoriteColor(Color::Default);
    } else if (!value.empty()) {
        config.SetFavoriteColor(static_cast<Color>(std::clamp(std::stoi(value.data()), 0, 15)));
        // TODO: Warn if invalid
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::FAVORITE_COLOR);
        config.SetFavoriteColor(Color::Default);
    }

    if (optional<UsernameMode> username = ParseUsernameMode(get_variable(firmware::USERNAME))) {
        config.SetUsernameMode(*username);
    } else {
        retro::warn("Failed to get value for {}; defaulting to \"melonDS DS\"", firmware::USERNAME);
        config.SetUsernameMode(UsernameMode::MelonDSDS);
    }

    if (optional<AlarmMode> alarmMode = ParseAlarmMode(get_variable(firmware::ENABLE_ALARM))) {
         config.SetAlarmMode(*alarmMode);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ENABLE_ALARM);
        config.SetAlarmMode(AlarmMode::Default);
    }

    if (string_view alarmHourText = get_variable(firmware::ALARM_HOUR); alarmHourText == values::DEFAULT) {
        config.SetAlarmHour(nullopt);
    } else if (optional<unsigned> alarmHour = ParseIntegerInRange(alarmHourText, 0u, 23u)) {
        config.SetAlarmHour(alarmHour);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ALARM_HOUR);
        config.SetAlarmHour(nullopt);
    }

    if (string_view alarmMinuteText = get_variable(firmware::ALARM_MINUTE); alarmMinuteText == values::DEFAULT) {
        config.SetAlarmMinute(nullopt);
    } else if (optional<unsigned> alarmMinute = ParseIntegerInRange(alarmMinuteText, 0u, 59u)) {
        config.SetAlarmMinute(alarmMinute);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ALARM_MINUTE);
        config.SetAlarmMinute(nullopt);
    }

    if (string_view birthMonthText = get_variable(firmware::BIRTH_MONTH); birthMonthText == values::DEFAULT) {
        config.SetBirthdayMonth(0);
    } else if (optional<unsigned> birthMonth = ParseIntegerInRange(birthMonthText, 1u, 12u)) {
        config.SetBirthdayMonth(*birthMonth);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::BIRTH_MONTH);
        config.SetBirthdayMonth(0);
    }

    if (string_view birthDayText = get_variable(firmware::BIRTH_DAY); birthDayText == values::DEFAULT) {
        config.SetBirthdayDay(0);
    } else if (optional<unsigned> birthDay = ParseIntegerInRange(birthDayText, 1u, 31u)) {
        config.SetBirthdayDay(*birthDay);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::BIRTH_DAY);
        config.SetBirthdayDay(0);
    }

    if (string_view wfcDnsText = get_variable(firmware::WFC_DNS); wfcDnsText == values::DEFAULT) {
        config.SetDnsServer(nullopt);
    } else if (optional<IpAddress> wfcDns = ParseIpAddress(wfcDnsText)) {
        config.SetDnsServer(*wfcDns);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::WFC_DNS);
        config.SetDnsServer(nullopt);
    }
}

static void MelonDsDs::config::ParseAudioOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::audio;
    using retro::get_variable;

    if (optional<MicButtonMode> value = ParseMicButtonMode(get_variable(MIC_INPUT_BUTTON))) {
        config.SetMicButtonMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", MIC_INPUT_BUTTON, values::HOLD);
        config.SetMicButtonMode(MicButtonMode::Hold);
    }

    if (optional<MelonDsDs::MicInputMode> value = MelonDsDs::ParseMicInputMode(get_variable(MIC_INPUT))) {
        config.SetMicInputMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", MIC_INPUT, values::SILENCE);
        config.SetMicInputMode(MicInputMode::None);
    }

    if (optional<AudioBitDepth> value = ParseBitDepth(get_variable(AUDIO_BITDEPTH))) {
        config.SetBitDepth(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", AUDIO_BITDEPTH, values::AUTO);
        config.SetBitDepth(AudioBitDepth::Auto);
    }

    if (optional<AudioInterpolation> value = ParseInterpolation(get_variable(AUDIO_INTERPOLATION))) {
        config.SetInterpolation(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", AUDIO_INTERPOLATION, values::DISABLED);
        config.SetInterpolation(AudioInterpolation::None);
    }
}

static void MelonDsDs::config::ParseNetworkOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using retro::get_variable;

#ifdef HAVE_NETWORKING
    if (optional<NetworkMode> value = ParseNetworkMode(get_variable(network::NETWORK_MODE))) {
        config.SetNetworkMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", network::NETWORK_MODE, values::INDIRECT);
        config.SetNetworkMode(NetworkMode::Indirect);
    }

#ifdef HAVE_NETWORKING_DIRECT_MODE
    if (string_view value = get_variable(network::DIRECT_NETWORK_INTERFACE); !value.empty()) {
        config.SetNetworkInterface(value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", network::DIRECT_NETWORK_INTERFACE, values::AUTO);
        config.SetNetworkInterface(string_view(values::AUTO));
    }
#endif
#endif

    if (string_view macAddrModeText = get_variable(network::MAC_ADDRESS_MODE); macAddrModeText == values::FROM_USERNAME) {
        optional<string_view> retro_username = retro::username();
        if (retro_username) {
            // The first 3 bytes of a MAC address are special:
            // they identify the manufacturer
            // 00:09:BF is the manufacturer of the default firmware.
            // We use 00:08:BF to differentiate and because
            // it is not assigned to anyone else.
            // (Not that this MAC address will be used outside the emulated network)
            melonDS::MacAddress addr{0x00, 0x08, 0xBF, 0, 0, 0};
            std::uint32_t seed = 0;
            // We use our own hashing algorithm to guarantee same output with different compilers
            // Our hash does not need to be very good because we already have an rng engine.
            for (char c : *retro_username) {
                // Protect against signed char/unsigned char differences
                if (c < 0) {
                    c *= -1;
                }
                std::uint32_t thisCharSeed = c;
                // 1419857 = 17^5
                // Shift left 4 bits (multiply by 16), then add the previous value 5 times,
                // but done in a quick multiplications instead of a loop.
                // (a * 17) = (a * 16) + a = (a << 4) + a
                // Why 5 times? Because UINT32_MAX/(17^5) > CHAR_MAX, assuming char is <= 8 bits,
                // which means there is no risk of overflow, which is implementation-defined.
                thisCharSeed *= 1419857;
                seed ^= thisCharSeed;
            }
            // We use a specific engine here to guarantee same output with different devices and compilers
            // This is the same as std::mt19937 but with the fixed uint32_t
            // instead of the implementation-defined uint_fast32_t
            std::mersenne_twister_engine<std::uint32_t,
                             32, 624, 397, 31,
                             0x9908b0df, 11,
                             0xffffffff, 7,
                             0x9d2c5680, 15,
                             0xefc60000, 18, 1812433253> rng_engine{seed};
            for (int i = 3; i <= 5; i++) {
                // Usually using modulo is a bad way to narrow a random distribution
                // However because UINT32_MAX + 1 % 256 = 0, it is actually ok.
                // Another way to think is that we are taking the eight lowest bits
                addr[i] = rng_engine() % 256;
            }
            config.SetMacAddress(addr);
        } else {
            config.SetMacAddress(nullopt);
        }
    } else if (macAddrModeText == values::FIRMWARE) {
        config.SetMacAddress(nullopt);
    } else if (optional<melonDS::MacAddress> fileAddress = ParseMacAddress(macAddrModeText)) {
        config.SetMacAddress(fileAddress);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", network::MAC_ADDRESS_MODE);
        config.SetMacAddress(nullopt);
    }
}

static void MelonDsDs::config::ParseScreenOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::screen;
    using retro::get_variable;

    if (optional<unsigned> value = ParseIntegerInRange<unsigned>(get_variable(SCREEN_GAP),0, 126)) {
        config.SetScreenGap(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SCREEN_GAP, 0);
        config.SetScreenGap(0);
    }

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(CURSOR_TIMEOUT), CURSOR_TIMEOUTS)) {
        config.SetCursorTimeout(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CURSOR_TIMEOUT, 3);
        config.SetCursorTimeout(3);
    }

    if (optional<MelonDsDs::TouchMode> value = ParseTouchMode(get_variable(TOUCH_MODE))) {
        config.SetTouchMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", TOUCH_MODE, values::AUTO);
        config.SetTouchMode(TouchMode::Auto);
    }

    if (optional<int> value = ParseIntegerInList<int>(get_variable(JOYSTICK_CURSOR_DEADZONE), JOYSTICK_CURSOR_DEADZONES)) {
        config.SetJoystickCursorDeadzone(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", JOYSTICK_CURSOR_DEADZONE, 0.05);
        config.SetJoystickCursorDeadzone(5);
    }

    if (optional<int> value = ParseIntegerInList<int>(get_variable(JOYSTICK_CURSOR_MAXSPEED), JOYSTICK_CURSOR_MAXSPEEDS)) {
        config.SetJoystickCursorMaxSpeed(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", JOYSTICK_CURSOR_MAXSPEED, 3);
        config.SetJoystickCursorMaxSpeed(3);
    }

    if (optional<int> value = ParseIntegerInList<int>(get_variable(JOYSTICK_CURSOR_RESPONSE), JOYSTICK_CURSOR_RESPONSES)) {
        config.SetJoystickCursorResponse(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", JOYSTICK_CURSOR_RESPONSE, 2);
        config.SetJoystickCursorResponse(200);
    }

    if (optional<int> value = ParseIntegerInList<int>(get_variable(JOYSTICK_CURSOR_SPEEDUP), JOYSTICK_CURSOR_SPEEDUPS)) {
        config.SetJoystickCursorSpeedup(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", JOYSTICK_CURSOR_SPEEDUP, 200);
        config.SetJoystickCursorSpeedup(200);
    }

    if (optional<MelonDsDs::CursorMode> value = ParseCursorMode(get_variable(SHOW_CURSOR))) {
        config.SetCursorMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SHOW_CURSOR, values::ALWAYS);
        config.SetCursorMode(CursorMode::Always);
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(HYBRID_RATIO), 2u, 3u)) {
        config.SetHybridRatio(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", HYBRID_RATIO, 2);
        config.SetHybridRatio(2);
    }

    if (optional<HybridSideScreenDisplay> value = ParseHybridSideScreenDisplay(get_variable(HYBRID_SMALL_SCREEN))) {
        config.SetSmallScreenLayout(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", HYBRID_SMALL_SCREEN, values::BOTH);
        config.SetSmallScreenLayout(HybridSideScreenDisplay::Both);
    }

    if (optional<unsigned> value = MelonDsDs::ParseIntegerInRange(get_variable(NUMBER_OF_SCREEN_LAYOUTS), 1u, MAX_SCREEN_LAYOUTS)) {
        config.SetNumberOfScreenLayouts(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", NUMBER_OF_SCREEN_LAYOUTS, 2);
        config.SetNumberOfScreenLayouts(2);
    }

    array<ScreenLayout, MAX_SCREEN_LAYOUTS> layouts {};
    for (unsigned i = 0; i < MAX_SCREEN_LAYOUTS; i++) {
        if (optional<MelonDsDs::ScreenLayout> value = MelonDsDs::ParseScreenLayout(get_variable(SCREEN_LAYOUTS[i]))) {
            layouts[i] = *value;
        } else {
            retro::warn("Failed to get value for {}; defaulting to {}", SCREEN_LAYOUTS[i], values::TOP_BOTTOM);
            layouts[i] = ScreenLayout::TopBottom;
        }
    }

    config.SetScreenLayouts(layouts);
}

static void MelonDsDs::config::ParseVideoOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::video;
    using retro::get_variable;

    if (optional<ScreenFilter> value = ParseScreenFilter(get_variable(OPENGL_FILTERING))) {
        config.SetScreenFilter(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", OPENGL_FILTERING, values::NEAREST);
        config.SetScreenFilter(*value);
    }

#if defined(HAVE_THREADS) && defined(HAVE_THREADED_RENDERER)
    if (optional<bool> value = ParseBoolean(get_variable(THREADED_RENDERER))) {
        config.SetThreadedSoftRenderer(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", THREADED_RENDERER, values::ENABLED);
        config.SetThreadedSoftRenderer(true);
    }
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (optional<RenderMode> renderer = ParseRenderMode(get_variable(RENDER_MODE))) {
        config.SetConfiguredRenderer(*renderer);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", RENDER_MODE, values::SOFTWARE);
        config.SetConfiguredRenderer(RenderMode::Software);
    }

    if (optional<unsigned> value = ParseIntegerInRange<unsigned>(get_variable(OPENGL_RESOLUTION), 1, MAX_OPENGL_SCALE)) {
        config.SetScaleFactor(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to 1", OPENGL_RESOLUTION);
        config.SetScaleFactor(1);
    }

    if (optional<bool> value = ParseBoolean(get_variable(OPENGL_BETTER_POLYGONS))) {
        config.SetBetterPolygonSplitting(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", OPENGL_BETTER_POLYGONS, values::DISABLED);
        config.SetBetterPolygonSplitting(false);
    }
#endif
}

struct FirmwareEntry {
    std::string path;
    Firmware::FirmwareHeader header;
    struct stat stat;
};

struct MacAddressEntry {
    std::string description;
    std::string printedAddress;
};

static time_t NewestTimestamp(const struct stat& statbuf) noexcept {
    return std::max({statbuf.st_atime, statbuf.st_mtime, statbuf.st_ctime});
}

static bool ConsoleTypeMatches(const Firmware::FirmwareHeader& header, MelonDsDs::ConsoleType type) noexcept {
    if (type == MelonDsDs::ConsoleType::DS) {
        return header.ConsoleType == Firmware::FirmwareConsoleType::DS || header.ConsoleType == Firmware::FirmwareConsoleType::DSLite;
    }
    else {
        return header.ConsoleType == Firmware::FirmwareConsoleType::DSi;
    }
}

static const char* SelectDefaultFirmware(const vector<FirmwareEntry>& images, MelonDsDs::ConsoleType type) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs;

    optional<string_view> sysdir = retro::get_system_directory();

    const auto& best = std::max_element(images.begin(), images.end(), [type](const FirmwareEntry& a, const FirmwareEntry& b) {
        bool aMatches = ConsoleTypeMatches(a.header, type);
        bool bMatches = ConsoleTypeMatches(b.header, type);

        if (!aMatches && bMatches) {
            // If the second image matches but the first doesn't, the second is automatically better
            return true;
        }
        else if (aMatches && !bMatches) {
            // If the first image matches but the second doesn't, the first is automatically better
            return false;
        }

        // Both (or neither) images match the console type, so pick the one with the newest timestamp
        return NewestTimestamp(a.stat) < NewestTimestamp(b.stat);
    });

    retro_assert(best != images.end());

    string_view name = best->path;
    name.remove_prefix(sysdir->size() + 1);

    return name.data();
}

#ifdef HAVE_NETWORKING_DIRECT_MODE
struct AdapterOption {
    AdapterData adapter;
    string value;
    string label;
};
#endif

// If I make an option depend on the game (e.g. different defaults for different games),
// then I can have set_core_option accept a NDSHeader
bool MelonDsDs::RegisterCoreOptions() noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config;

    array categories = definitions::OptionCategories;
    array definitions = definitions::CoreOptionDefinitions;

    optional<string_view> subdir = retro::get_system_subdirectory();

    vector<string> dsiNandPaths;
    vector<FirmwareEntry> firmware;
    vector<MacAddressEntry> macAddresses;
    optional<string_view> sysdir = retro::get_system_directory();

    if (subdir) {
        ZoneScopedN("MelonDsDs::config::set_core_options::find_system_files");
        retro_assert(sysdir.has_value());
        u8 headerBytes[sizeof(Firmware::FirmwareHeader)];
        Firmware::FirmwareHeader& header = *reinterpret_cast<Firmware::FirmwareHeader*>(headerBytes);
        memset(headerBytes, 0, sizeof(headerBytes));
        array paths = {*sysdir, *subdir};
        for (const string_view& path: paths) {
            ZoneScopedN("MelonDsDs::config::set_core_options::find_system_files::paths");
            for (const retro::dirent& d : retro::readdir(string(path), true)) {
                ZoneScopedN("MelonDsDs::config::set_core_options::find_system_files::paths::dirent");
                // TODO: Pick a particular file name and load MAC addresses from it (one per line)

                if (IsDsiNandImage(d)) {
                    dsiNandPaths.emplace_back(d.path);
                } else if (IsFirmwareImage(d, header)) {
                    struct stat statbuf;
                    stat(d.path, &statbuf);
                    firmware.emplace_back(FirmwareEntry {d.path, header, statbuf});
                }
            }
        }

    } else {
        retro::set_error_message("Failed to get system directory, anything that needs it won't work.");
    }

    if (!dsiNandPaths.empty()) {
        ZoneScopedN("MelonDsDs::config::set_core_options::init_dsi_nand_options");
        // If we found at least one DSi NAND image...
        retro_core_option_v2_definition* dsiNandPathOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, MelonDsDs::config::storage::DSI_NAND_PATH);
        });

        retro_assert(dsiNandPathOption != definitions.end());

        memset(dsiNandPathOption->values, 0, sizeof(dsiNandPathOption->values));
        int length = std::min((int)dsiNandPaths.size(), (int)RETRO_NUM_CORE_OPTION_VALUES_MAX - 1);
        for (int i = 0; i < length; ++i) {
            string_view path = dsiNandPaths[i];
            path.remove_prefix(sysdir->size() + 1);
            retro::debug("Found a DSi NAND image at \"{}\", presenting it in the options as \"{}\"", dsiNandPaths[i], path);
            retro_assert(!path_is_absolute(path.data()));
            dsiNandPathOption->values[i].value = path.data();
            dsiNandPathOption->values[i].label = nullptr;
        }

        dsiNandPathOption->default_value = dsiNandPathOption->values[0].value;
    }

    if (!firmware.empty()) {
        ZoneScopedN("MelonDsDs::config::set_core_options::init_firmware_options");
        // If we found at least one firmware image...
        retro_core_option_v2_definition* firmwarePathOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, MelonDsDs::config::system::FIRMWARE_PATH);
        });
        retro_core_option_v2_definition* firmwarePathDsiOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, MelonDsDs::config::system::FIRMWARE_DSI_PATH);
        });

        retro_assert(firmwarePathOption != definitions.end());
        retro_assert(firmwarePathDsiOption != definitions.end());

        int length = std::min((int)firmware.size(), (int)RETRO_NUM_CORE_OPTION_VALUES_MAX - 1);
        for (int i = 0; i < length; ++i) {
            retro::debug("Found a firmware image at \"{}\"", firmware[i].path);
            string_view path = firmware[i].path;
            path.remove_prefix(sysdir->size() + 1);
            firmwarePathOption->values[i] = { path.data(), nullptr };
            firmwarePathDsiOption->values[i] = { path.data(), nullptr };
        }
        firmwarePathOption->values[length + 1] = { nullptr, nullptr };
        firmwarePathDsiOption->values[length + 1] = { nullptr, nullptr };

        firmwarePathOption->default_value = SelectDefaultFirmware(firmware, ConsoleType::DS);
        firmwarePathDsiOption->default_value = SelectDefaultFirmware(firmware, ConsoleType::DSi);

        retro_assert(firmwarePathOption->default_value != nullptr);
        retro_assert(firmwarePathDsiOption->default_value != nullptr);
    }
    if(!macAddresses.empty()) {
        ZoneScopedN("MelonDsDs::config::set_core_options::init_mac_address_options");
        retro_core_option_v2_definition* macAddressOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, MelonDsDs::config::network::MAC_ADDRESS_MODE);
        });
        retro_assert(macAddressOption != definitions.end());
        int existingOptions = 0;
        while (macAddressOption->values[existingOptions++].value != nullptr && existingOptions != RETRO_NUM_CORE_OPTION_VALUES_MAX) {
        }
        existingOptions--;
        int length = std::min((int)macAddresses.size(), (int)RETRO_NUM_CORE_OPTION_VALUES_MAX - existingOptions);
        for (int i = 0; i < length; ++i) {
            macAddressOption->values[i + existingOptions] = { macAddresses[i].printedAddress.data(), macAddresses[i].description.data() };
        }
        if(length + existingOptions < RETRO_NUM_CORE_OPTION_VALUES_MAX) {
            macAddressOption->values[length + existingOptions] = { nullptr, nullptr };
        }
    }

    // TODO: Pass in the LibPCap instance created by NetState
    // TODO: Create a DynamicOption class, pass in instances of that

#ifdef HAVE_NETWORKING_DIRECT_MODE
    // holds on to strings used in dynamic options until we finish submitting the options to the frontend
    // DO NOT move this into a deeper scope, or else the strings that the options point to will be destroyed
    // ReSharper disable once CppTooWideScope
    vector<AdapterOption> adapters;
    if (std::optional<LibPCap> pcap = LibPCap::New(); pcap) {
        ZoneScopedN("MelonDsDs::config::set_core_options::init_adapter_options");
        // If we successfully initialized PCap and got some adapters...
        vector<AdapterData> availableAdapters = pcap->GetAdapters();
        retro_core_option_v2_definition* wifiAdapterOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, MelonDsDs::config::network::DIRECT_NETWORK_INTERFACE);
        });
        retro_assert(wifiAdapterOption != definitions.end());

        // Zero all option values except for the first (Automatic)
        memset(wifiAdapterOption->values + 1, 0, sizeof(retro_core_option_value) * (RETRO_NUM_CORE_OPTION_VALUES_MAX - 1));
        for (const AdapterData& adapter : availableAdapters) {
            if (IsAdapterAcceptable(adapter) && adapters.size() < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1) {
                // If this interface would potentially work, and we haven't added the max...
                string mac = fmt::format("{:02x}", fmt::join(adapter.MAC, ":"));
                retro::debug(
                    "Found a \"{}\" ({}) interface with ID {} at {} bound to {} ({})",
                    adapter.FriendlyName,
                    adapter.Description,
                    adapter.DeviceName,
                    mac,
                    fmt::join(adapter.IP_v4, "."),
                    static_cast<FormattedPCapFlags>(adapter.Flags)
                );

                string label = fmt::format("{} ({})", string_is_empty(adapter.FriendlyName) ? adapter.DeviceName : adapter.FriendlyName, mac);
                adapters.emplace_back(AdapterOption {
                    .adapter = adapter,
                    .value = std::move(mac),
                    .label = std::move(label),
                });
            }
        }

        int numAdapters = std::min<int>(RETRO_NUM_CORE_OPTION_VALUES_MAX - 2, adapters.size());
        for (int i = 0; i < numAdapters; ++i) {
            const AdapterOption& adapter = adapters[i];
            wifiAdapterOption->values[i + 1] = { adapter.value.c_str(), adapter.label.c_str() };
        }
        wifiAdapterOption->values[numAdapters + 1] = { nullptr, nullptr };
    } else {
        retro::warn("Failed to enumerate Wi-fi adapters");
    }
#endif

    retro_core_options_v2 optionsUs = {
        .categories = categories.data(),
        .definitions = definitions.data(),
    };

#ifndef NDEBUG
    // Ensure for sanity's sake that no option value can be the empty string.
    // (This has bitten me before.)
    for (size_t i = 0; i < definitions.size() - 1; ++i) {
        // For each definition except the null terminator at the end...
        for (size_t v = 0; v < RETRO_NUM_CORE_OPTION_VALUES_MAX && definitions[i].values[v].value != nullptr; ++v) {
            // For each option value except the null terminator...
            retro_assert(!string_is_empty(definitions[i].values[v].value));
        }
    }
#endif

    if (!retro::set_core_options(optionsUs)) {
        retro::set_error_message("Failed to set core option definitions, functionality will be limited.");
        return false;
    }

    return true;
}
