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
#include <LAN_PCap.h>

#include "config/constants.hpp"
#include "config/definitions.hpp"
#include "config/definitions/categories.hpp"
#include "../core/core.hpp"
#include "embedded/melondsds_default_wfc_config.h"
#include "environment.hpp"
#include "exceptions.hpp"
#include "format.hpp"
#include "input.hpp"
#include "libretro.hpp"
#include "microphone.hpp"
#include "retro/dirent.hpp"
#include "screenlayout.hpp"
#include "tracy.hpp"
#include "pcap.hpp"

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
using LAN_PCap::AdapterData;

constexpr unsigned AUTO_SDCARD_SIZE = 0;
constexpr unsigned DEFAULT_SDCARD_SIZE = 4096;
const char* const DEFAULT_HOMEBREW_SDCARD_IMAGE_NAME = "dldi_sd_card.bin";
const char* const DEFAULT_HOMEBREW_SDCARD_DIR_NAME = "dldi_sd_card";
const char* const DEFAULT_DSI_SDCARD_IMAGE_NAME = "dsi_sd_card.bin";
const char* const DEFAULT_DSI_SDCARD_DIR_NAME = "dsi_sd_card";

const initializer_list<unsigned> SCREEN_GAP_LENGTHS = {0, 1, 2, 8, 16, 24, 32, 48, 64, 72, 88, 90, 128};
const initializer_list<unsigned> CURSOR_TIMEOUTS = {1, 2, 3, 5, 10, 15, 20, 30, 60};
const initializer_list<unsigned> DS_POWER_OK_THRESHOLDS = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
const initializer_list<unsigned> POWER_UPDATE_INTERVALS = {1, 2, 3, 5, 10, 15, 20, 30, 60};

namespace MelonDsDs::config {
    static void ParseSystemOptions(CoreConfig& config) noexcept;
    static void ParseOsdOptions(CoreConfig& config) noexcept;
    static void ParseJitOptions(CoreConfig& config) noexcept;
    static void ParseHomebrewSaveOptions(CoreConfig& config) noexcept;
    static void ParseDsiStorageOptions(CoreConfig& config) noexcept;
    static void ParseFirmwareOptions(CoreConfig& config) noexcept;
    static void ParseAudioOptions(CoreConfig& config) noexcept;
    static void ParseNetworkOptions(CoreConfig& config) noexcept;
    static void ParseScreenOptions(CoreConfig& config) noexcept;
    static void ParseVideoOptions(CoreConfig& config) noexcept;

    static void apply_system_options(MelonDsDs::CoreState& core, const NDSHeader* header);

    static void apply_audio_options(NDS& nds) noexcept;
    static void apply_save_options(const NDSHeader* header);
    static void apply_screen_options(ScreenLayoutData& screenLayout, InputState& inputState) noexcept;

}

void MelonDsDs::ParseConfig(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    config::ParseSystemOptions(config);
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

void MelonDsDs::InitConfig(
    MelonDsDs::CoreState& core,
    const melonDS::NDSHeader* header,
    ScreenLayoutData& screenLayout,
    InputState& inputState
) {
    ZoneScopedN("MelonDsDs::InitConfig");
    config::parse_system_options();
    config::parse_osd_options();
    config::parse_jit_options();
    config::parse_homebrew_save_options(header);
    config::parse_dsi_storage_options();
    config::parse_firmware_options();
    config::parse_audio_options();
    config::parse_network_options();
    bool openGlNeedsRefresh = config::parse_video_options(true);
    config::parse_screen_options();

    config::apply_system_options(core, header);
    config::apply_save_options(header);
    config::apply_screen_options(screenLayout, inputState);

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (MelonDsDs::opengl::UsingOpenGl() && (openGlNeedsRefresh || screenLayout.Dirty())) {
        // If we're using OpenGL and the settings changed, or the screen layout changed...
        MelonDsDs::opengl::RequestOpenGlRefresh();
    }
#endif

    if (MelonDsDs::render::CurrentRenderer() == Renderer::None) {
        screenLayout.Update(config::video::ConfiguredRenderer());
    } else {
        screenLayout.Update(MelonDsDs::render::CurrentRenderer());
    }

    update_option_visibility();
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

    if (optional<unsigned> value = ParseIntegerInList(get_variable(DS_POWER_OK), DS_POWER_OK_THRESHOLDS)) {
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
    ZoneScopedN("MelonDsDs::config::parse_homebrew_save_options");
    using retro::get_variable;

    optional<string> save_directory = retro::get_save_directory();
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

    if (optional<bool> value = ParseBoolean(get_variable(storage::HOMEBREW_READ_ONLY))) {
        config.SetDldiEnable(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::HOMEBREW_READ_ONLY, values::DISABLED);
        config.SetDldiEnable(false);
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

    // If these firmware/BIOS files don't exist, an exception will be thrown later
    if (const char* value = get_variable(storage::DSI_NAND_PATH); !string_is_empty(value)) {
        config.SetDsiNandPath(string_view(value));
    } else {
        retro::warn("Failed to get value for {}", storage::DSI_NAND_PATH);
        config.SetDsiNandPath(string_view(values::NOT_FOUND));
    }

    if (const char* value = get_variable(system::FIRMWARE_PATH); !string_is_empty(value)) {
        config.SetFirmwarePath(string_view(value));
    } else {
        retro::warn("Failed to get value for {}; defaulting to built-in firmware", system::FIRMWARE_PATH);
        config.SetFirmwarePath(string_view(values::NOT_FOUND));
    }

    if (const char* value = get_variable(system::FIRMWARE_DSI_PATH); !string_is_empty(value)) {
        config.SetDsiFirmwarePath(string_view(value));
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

    if (const char* value = get_variable(firmware::FAVORITE_COLOR); string_is_equal(value, values::DEFAULT)) {
        config.SetFavoriteColor(Color::Default);
    } else if (!string_is_empty(value)) {
        config.SetFavoriteColor(static_cast<Color>(std::clamp(std::stoi(value), 0, 15)));
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

    if (const char* alarmHourText = get_variable(firmware::ALARM_HOUR); string_is_equal(alarmHourText, values::DEFAULT)) {
        config.SetAlarmHour(nullopt);
    } else if (optional<unsigned> alarmHour = ParseIntegerInRange(alarmHourText, 0u, 23u)) {
        config.SetAlarmHour(alarmHour);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ALARM_HOUR);
        config.SetAlarmHour(nullopt);
    }

    if (const char* alarmMinuteText = get_variable(firmware::ALARM_MINUTE); string_is_equal(alarmMinuteText, values::DEFAULT)) {
        config.SetAlarmMinute(nullopt);
    } else if (optional<unsigned> alarmMinute = ParseIntegerInRange(alarmMinuteText, 0u, 59u)) {
        config.SetAlarmMinute(alarmMinute);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ALARM_MINUTE);
        config.SetAlarmMinute(nullopt);
    }

    if (const char* birthMonthText = get_variable(firmware::BIRTH_MONTH); string_is_equal(birthMonthText, values::DEFAULT)) {
        config.SetBirthdayMonth(0);
    } else if (optional<unsigned> birthMonth = ParseIntegerInRange(birthMonthText, 1u, 12u)) {
        config.SetBirthdayMonth(*birthMonth);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::BIRTH_MONTH);
        config.SetBirthdayMonth(0);
    }

    if (const char* birthDayText = get_variable(firmware::BIRTH_DAY); string_is_equal(birthDayText, values::DEFAULT)) {
        config.SetBirthdayDay(0);
    } else if (optional<unsigned> birthDay = ParseIntegerInRange(birthDayText, 1u, 31u)) {
        config.SetBirthdayDay(*birthDay);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::BIRTH_DAY);
        config.SetBirthdayDay(0);
    }

    if (const char* wfcDnsText = get_variable(firmware::WFC_DNS); string_is_equal(wfcDnsText, values::DEFAULT)) {
        config.SetDnsServer(nullopt);
    } else if (optional<IpAddress> wfcDns = ParseIpAddress(wfcDnsText)) {
        config.SetDnsServer(*wfcDns);
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::WFC_DNS);
        config.SetDnsServer(nullopt);
    }

    // TODO: Make MAC address configurable with a file at runtime
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
#ifdef HAVE_NETWORKING
    ZoneScopedN(TracyFunction);
    using retro::get_variable;

    if (optional<NetworkMode> value = ParseNetworkMode(get_variable(network::NETWORK_MODE))) {
        config.SetNetworkMode(*value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", network::NETWORK_MODE, values::INDIRECT);
        config.SetNetworkMode(NetworkMode::Indirect);
    }

#ifdef HAVE_NETWORKING_DIRECT_MODE
    if (const char* value = get_variable(network::DIRECT_NETWORK_INTERFACE); !string_is_empty(value)) {
        config.SetNetworkInterface(string_view(value));
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", network::DIRECT_NETWORK_INTERFACE, values::AUTO);
        config.SetNetworkInterface(string_view(values::AUTO));
    }
#endif
#endif
}

static void MelonDsDs::config::ParseScreenOptions(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::screen;
    using retro::get_variable;

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(SCREEN_GAP), SCREEN_GAP_LENGTHS)) {
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
    if (optional<Renderer> renderer = ParseRenderer(get_variable(RENDER_MODE))) {
        config.SetConfiguredRenderer(*renderer);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", RENDER_MODE, values::SOFTWARE);
        config.SetConfiguredRenderer(Renderer::Software);
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

void MelonDsDs::UpdateConfig(MelonDsDs::CoreState& core, ScreenLayoutData& screenLayout, InputState& inputState) noexcept {
    ZoneScopedN("MelonDsDs::config::UpdateConfig");
    config::parse_audio_options();
    bool openGlNeedsRefresh = config::parse_video_options(false);
    config::parse_screen_options();
    config::parse_osd_options();

    retro_assert(core.Console != nullptr);

    config::apply_audio_options(*core.Console);
    config::apply_screen_options(screenLayout, inputState);

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (MelonDsDs::opengl::UsingOpenGl() && (openGlNeedsRefresh || screenLayout.Dirty())) {
        // If we're using OpenGL and the settings changed, or the screen layout changed...
        MelonDsDs::opengl::RequestOpenGlRefresh();
    }
#endif

    update_option_visibility();
}

static void MelonDsDs::config::apply_audio_options(NDS& nds) noexcept {
    ZoneScopedN("MelonDsDs::config::apply_audio_options");
    bool is_using_host_mic = audio::MicInputMode() == MicInputMode::HostMic;
    if (retro::microphone::is_interface_available()) {
        // Open the mic if the user wants it (and it isn't already open)
        // Close the mic if the user wants it (and it is open)
        bool ok = retro::microphone::set_open(is_using_host_mic);
        if (!ok) {
            // If we couldn't open or close the microphone...
            retro::warn("Failed to {} microphone", is_using_host_mic ? "open" : "close");
        }
    } else {
        if (is_using_host_mic && osd::ShowUnsupportedFeatureWarnings()) {
            retro::set_warn_message("This frontend doesn't support microphones.");
        }
    }

    nds.SPU.SetInterpolation(config::audio::Interpolation());
}

static void MelonDsDs::config::apply_save_options(const NDSHeader* header) {
    ZoneScopedN("MelonDsDs::config::apply_save_options");
    using namespace config::save;

    const optional<string> save_directory = retro::get_save_directory();
    if (!save_directory && (DldiEnable() || DsiSdEnable())) {
        // If we want to use SD cards, but we can't get the save directory...
        _dsiSdEnable = false;
        _dldiEnable = false;
        retro::set_error_message("Failed to get save directory; SD cards will not be available.");
        return;
    }

    if (header && header->IsHomebrew() && DldiEnable()) {
        // If we're loading a homebrew game with an SD card...
        char path[PATH_MAX];

        fill_pathname_join_special(path, save_directory->c_str(), DEFAULT_HOMEBREW_SDCARD_DIR_NAME, sizeof(path));
        _dldiFolderPath = string(path);

        fill_pathname_join_special(path, save_directory->c_str(), DEFAULT_HOMEBREW_SDCARD_IMAGE_NAME, sizeof(path));
        _dldiImagePath = string(path);

        if (path_is_valid(_dldiImagePath.c_str())) {
            // If the SD card image exists...
            retro::info("Using existing homebrew SD card image \"{}\"", _dldiImagePath);
            _dldiImageSize = AUTO_SDCARD_SIZE;
        } else {
            retro::info("No homebrew SD card image found at \"{}\"; will create an image.", _dldiImagePath);
            _dldiImageSize = DEFAULT_SDCARD_SIZE;
        }

        if (DldiFolderSync()) {
            // If we want to sync the homebrew SD card to the host...
            if (!path_mkdir(_dldiFolderPath.c_str())) {
                // Create the save directory. If that failed...
                throw emulator_exception("Failed to create homebrew save directory at " + _dldiFolderPath);
            }

            retro::info("Created (or using existing) homebrew save directory \"{}\"", _dldiFolderPath);
        }
    } else {
        retro::info("Not using homebrew SD card");
    }

    if (system::ConsoleType() == ConsoleType::DSi && DsiSdEnable()) {
        // If we're running in DSi mode and we want to sync its SD card image to the host...
        char path[PATH_MAX];

        fill_pathname_join_special(path, save_directory->c_str(), DEFAULT_DSI_SDCARD_DIR_NAME, sizeof(path));
        _dsiSdFolderPath = string(path);

        fill_pathname_join_special(path, save_directory->c_str(), DEFAULT_DSI_SDCARD_IMAGE_NAME, sizeof(path));
        _dsiSdImagePath = string(path);

        if (path_is_valid(_dsiSdImagePath.c_str())) {
            // If the SD card image exists...
            retro::info("Using existing DSi SD card image \"{}\"", _dsiSdImagePath);
            _dsiSdImageSize = AUTO_SDCARD_SIZE;
        } else {
            retro::info("No DSi SD card image found at \"{}\"; will create an image.", _dsiSdImagePath);
            _dsiSdImageSize = DEFAULT_SDCARD_SIZE;
        }

        if (DsiSdFolderSync()) {
            // If we want to sync the homebrew SD card to the host...
            if (!path_mkdir(_dsiSdFolderPath.c_str())) {
                // Create the save directory. If that failed...
                throw emulator_exception("Failed to create DSi SD card save directory at " + _dsiSdFolderPath);
            }

            retro::info("Created (or using existing) DSi SD card save directory \"{}\"", _dsiSdFolderPath);
        }
    } else {
        retro::info("Not using DSi SD card");
    }
}

struct FirmwareEntry {
    std::string path;
    Firmware::FirmwareHeader header;
    struct stat stat;
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
    ZoneScopedN("MelonDsDs::config::SelectDefaultFirmware");
    using namespace MelonDsDs;

    const optional<string>& sysdir = retro::get_system_directory();

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


static vector<string_view> fmt_flags(const pcap_if_t& interface) noexcept {
    fmt::memory_buffer buffer;

    vector<string_view> flagNames;
    if (interface.flags & PCAP_IF_LOOPBACK) {
        flagNames.emplace_back("Loopback");
    }

    if (interface.flags & PCAP_IF_UP) {
        flagNames.emplace_back("Up");
    }

    if (interface.flags & PCAP_IF_RUNNING) {
        flagNames.emplace_back("Running");
    }

    if (interface.flags & PCAP_IF_WIRELESS) {
        flagNames.emplace_back("Wireless");
    }

    switch (interface.flags & PCAP_IF_CONNECTION_STATUS) {
        case PCAP_IF_CONNECTION_STATUS_UNKNOWN:
            flagNames.emplace_back("UnknownStatus");
            break;
        case PCAP_IF_CONNECTION_STATUS_CONNECTED:
            flagNames.emplace_back("Connected");
            break;
        case PCAP_IF_CONNECTION_STATUS_DISCONNECTED:
            flagNames.emplace_back("Disconnected");
            break;
        case PCAP_IF_CONNECTION_STATUS_NOT_APPLICABLE:
            flagNames.emplace_back("ConnectionStatusNotApplicable");
            break;
    }

    return flagNames;
}
#endif

// If I make an option depend on the game (e.g. different defaults for different games),
// then I can have set_core_option accept a NDSHeader
bool MelonDsDs::RegisterCoreOptions() noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config;

    array categories = definitions::OptionCategories<RETRO_LANGUAGE_ENGLISH>;
    array definitions = definitions::CoreOptionDefinitions<RETRO_LANGUAGE_ENGLISH>;

    optional<string> subdir = retro::get_system_subdirectory();

    vector<string> dsiNandPaths;
    vector<FirmwareEntry> firmware;
    const optional<string>& sysdir = retro::get_system_directory();

    if (subdir) {
        ZoneScopedN("MelonDsDs::config::set_core_options::find_system_files");
        retro_assert(sysdir.has_value());
        u8 headerBytes[sizeof(Firmware::FirmwareHeader)];
        Firmware::FirmwareHeader& header = *reinterpret_cast<Firmware::FirmwareHeader*>(headerBytes);
        memset(headerBytes, 0, sizeof(headerBytes));
        array paths = {*sysdir, *subdir};
        for (const string& path: paths) {
            ZoneScopedN("MelonDsDs::config::set_core_options::find_system_files::paths");
            for (const retro::dirent& d : retro::readdir(path, true)) {
                ZoneScopedN("MelonDsDs::config::set_core_options::find_system_files::paths::dirent");
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
            retro::debug("Found a DSi NAND image at \"{}\"", dsiNandPaths[i]);
            string_view path = dsiNandPaths[i];
            path.remove_prefix(sysdir->size() + 1);
            dsiNandPathOption->values[i].value = path.data();
            dsiNandPathOption->values[i].label = nullptr;
        }

        dsiNandPathOption->default_value = dsiNandPaths[0].c_str();
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

    bool pcapOk;
    {
        ZoneScopedN("LAN_PCap::Init");
        pcapOk = LAN_PCap::Init(false);
    }

#ifdef HAVE_NETWORKING_DIRECT_MODE
    // holds on to strings used in dynamic options until we finish submitting the options to the frontend
    vector<AdapterOption> adapters;
    if (pcapOk) {
        ZoneScopedN("MelonDsDs::config::set_core_options::init_adapter_options");
        // If we successfully initialized PCap and got some adapters...
        retro_core_option_v2_definition* wifiAdapterOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, MelonDsDs::config::network::DIRECT_NETWORK_INTERFACE);
        });
        retro_assert(wifiAdapterOption != definitions.end());

        // Zero all option values except for the first (Automatic)
        memset(wifiAdapterOption->values + 1, 0, sizeof(retro_core_option_value) * (RETRO_NUM_CORE_OPTION_VALUES_MAX - 1));
        int length = std::min<int>(LAN_PCap::NumAdapters, RETRO_NUM_CORE_OPTION_VALUES_MAX - 1);
        for (int i = 0; i < length; ++i) {
            const LAN_PCap::AdapterData& adapter = LAN_PCap::Adapters[i];
            if (IsAdapterAcceptable(adapter)) {
                // If this interface would potentially work...

                string mac = fmt::format("{:02x}", fmt::join(adapter.MAC, ":"));
                retro::debug(
                    "Found a \"{}\" ({}) interface with ID {} at {} bound to {} ({})",
                    adapter.FriendlyName,
                    adapter.Description,
                    adapter.DeviceName,
                    mac,
                    fmt::join(adapter.IP_v4, "."),
                    fmt::join(fmt_flags(*static_cast<const pcap_if_t*>(adapter.Internal)), "|")
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
