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
#include <pcap/pcap.h>

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
    namespace visibility {
        static bool ShowMicButtonMode = true;
#ifdef HAVE_NETWORKING_DIRECT_MODE
        static bool ShowWifiInterface = true;
#endif
        static bool ShowHomebrewSdOptions = true;
        static bool ShowDsOptions = true;
        static bool ShowDsiOptions = true;
        static bool ShowDsiSdCardOptions = true;
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
        static bool ShowOpenGlOptions = true;
#endif
        static bool ShowSoftwareRenderOptions = true;
        static bool ShowHybridOptions = true;
        static bool ShowVerticalLayoutOptions = true;
        static bool ShowCursorTimeout = true;
        static bool ShowAlarm = true;
        static unsigned NumberOfShownScreenLayouts = screen::MAX_SCREEN_LAYOUTS;
#ifdef JIT_ENABLED
        static bool ShowJitOptions = true;
#endif
    }

    static void LoadSystemOptions(CoreConfig& config) noexcept;
    static void LoadOsdOptions(CoreConfig& config) noexcept;
    static void LoadJitOptions(CoreConfig& config) noexcept;
    static void LoadHomebrewSaveOptions(CoreConfig& config) noexcept;
    static void LoadDsiStorageOptions(CoreConfig& config) noexcept;
    static void LoadFirmwareOptions(CoreConfig& config) noexcept;
    static void LoadAudioOptions(CoreConfig& config) noexcept;
    static void LoadNetworkOptions(CoreConfig& config) noexcept;
    static void LoadScreenOptions(CoreConfig& config) noexcept;
    static void LoadVideoOptions(CoreConfig& config) noexcept;

    static void apply_system_options(MelonDsDs::CoreState& core, const NDSHeader* header);

    static void apply_audio_options(NDS& nds) noexcept;
    static void apply_save_options(const NDSHeader* header);
    static void apply_screen_options(ScreenLayoutData& screenLayout, InputState& inputState) noexcept;

}

void MelonDsDs::LoadConfig(CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    config::LoadSystemOptions(config);
    config::LoadOsdOptions(config);
    config::LoadJitOptions(config);
    config::LoadHomebrewSaveOptions(config);
    config::LoadDsiStorageOptions(config);
    config::LoadFirmwareOptions(config);
    config::LoadAudioOptions(config);
    config::LoadNetworkOptions(config);
    config::LoadScreenOptions(config);
    config::LoadVideoOptions(config);
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

    retro_assert(core.Console == nullptr);
    config::apply_system_options(core, header);
    retro_assert(core.Console != nullptr);
    config::apply_save_options(header);
    config::apply_audio_options(*core.Console);
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

static void MelonDsDs::config::LoadSystemOptions(CoreConfig& config) noexcept {
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

void MelonDsDs::config::LoadOsdOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadJitOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadHomebrewSaveOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadDsiStorageOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadFirmwareOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadAudioOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadNetworkOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadScreenOptions(CoreConfig& config) noexcept {
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

static void MelonDsDs::config::LoadVideoOptions(CoreConfig& config) noexcept {
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

bool MelonDsDs::update_option_visibility() {
    ZoneScopedN("MelonDsDs::update_option_visibility");
    using namespace MelonDsDs::config;
    using namespace MelonDsDs::config::visibility;
    using retro::environment;
    using retro::get_variable;
    using retro::set_option_visible;
    bool updated = false;

    retro::debug("MelonDsDs::update_option_visibility");

    // Convention: if an option is not found, show any dependent options
    bool oldShowMicButtonMode = ShowMicButtonMode;
    optional<MicInputMode> micInputMode = MelonDsDs::ParseMicInputMode(get_variable(audio::MIC_INPUT));
    ShowMicButtonMode = !micInputMode || *micInputMode != MicInputMode::None;
    if (ShowMicButtonMode != oldShowMicButtonMode) {
        set_option_visible(audio::MIC_INPUT_BUTTON, ShowMicButtonMode);
        updated = true;
    }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    // Show/hide OpenGL core options
    bool oldShowOpenGlOptions = ShowOpenGlOptions;
    bool oldShowSoftwareRenderOptions = ShowSoftwareRenderOptions;
    optional<Renderer> renderer = MelonDsDs::ParseRenderer(get_variable(video::RENDER_MODE));
    ShowOpenGlOptions = !renderer || *renderer == Renderer::OpenGl;
    ShowSoftwareRenderOptions = !ShowOpenGlOptions;
    if (ShowOpenGlOptions != oldShowOpenGlOptions) {
        set_option_visible(video::OPENGL_RESOLUTION, ShowOpenGlOptions);
        set_option_visible(video::OPENGL_FILTERING, ShowOpenGlOptions);
        set_option_visible(video::OPENGL_BETTER_POLYGONS, ShowOpenGlOptions);
        updated = true;
    }
#ifdef HAVE_THREADED_RENDERER
    if (ShowSoftwareRenderOptions != oldShowSoftwareRenderOptions) {
        set_option_visible(video::THREADED_RENDERER, ShowSoftwareRenderOptions);
        updated = true;
    }
#endif

#else
    set_option_visible(video::RENDER_MODE, false);
#endif

    bool oldShowDsiOptions = ShowDsiOptions;
    optional<ConsoleType> consoleType = MelonDsDs::ParseConsoleType(get_variable(system::CONSOLE_MODE));
    ShowDsiOptions = !consoleType || *consoleType == ConsoleType::DSi;
    if (ShowDsiOptions != oldShowDsiOptions) {
        set_option_visible(config::system::FIRMWARE_DSI_PATH, ShowDsiOptions);
        set_option_visible(config::storage::DSI_NAND_PATH, ShowDsiOptions);
        set_option_visible(storage::DSI_SD_SAVE_MODE, ShowDsiOptions);
        updated = true;
    }

    bool oldShowDsiSdCardOptions = ShowDsiSdCardOptions && ShowDsiOptions;
    optional<bool> dsiSdEnable = MelonDsDs::ParseBoolean(get_variable(storage::DSI_SD_SAVE_MODE));
    ShowDsiSdCardOptions = !dsiSdEnable || *dsiSdEnable;
    if (ShowDsiSdCardOptions != oldShowDsiSdCardOptions) {
        set_option_visible(storage::DSI_SD_READ_ONLY, ShowDsiSdCardOptions);
        set_option_visible(storage::DSI_SD_SYNC_TO_HOST, ShowDsiSdCardOptions);
        updated = true;
    }

    bool oldShowDsOptions = ShowDsOptions;
    ShowDsOptions = !consoleType || *consoleType == ConsoleType::DS;
    if (ShowDsOptions != oldShowDsOptions) {
        set_option_visible(config::system::SYSFILE_MODE, ShowDsOptions);
        set_option_visible(config::system::FIRMWARE_PATH, ShowDsOptions);
        set_option_visible(config::system::DS_POWER_OK, ShowDsOptions);
        updated = true;
    }

    bool oldShowHomebrewSdOptions = ShowHomebrewSdOptions;
    optional<bool> homebrewSdCardEnabled = MelonDsDs::ParseBoolean(get_variable(storage::HOMEBREW_SAVE_MODE));
    ShowHomebrewSdOptions = !homebrewSdCardEnabled || *homebrewSdCardEnabled;
    if (ShowHomebrewSdOptions != oldShowHomebrewSdOptions) {
        set_option_visible(storage::HOMEBREW_READ_ONLY, ShowHomebrewSdOptions);
        set_option_visible(storage::HOMEBREW_SYNC_TO_HOST, ShowHomebrewSdOptions);
        updated = true;
    }

    bool oldShowCursorTimeout = ShowCursorTimeout;
    optional<MelonDsDs::CursorMode> cursorMode = MelonDsDs::ParseCursorMode(get_variable(screen::SHOW_CURSOR));
    ShowCursorTimeout = !cursorMode || *cursorMode == MelonDsDs::CursorMode::Timeout;
    if (ShowCursorTimeout != oldShowCursorTimeout) {
        set_option_visible(screen::CURSOR_TIMEOUT, ShowCursorTimeout);
        updated = true;
    }

    unsigned oldNumberOfShownScreenLayouts = NumberOfShownScreenLayouts;
    optional<unsigned> numberOfScreenLayouts = MelonDsDs::ParseIntegerInRange(get_variable(screen::NUMBER_OF_SCREEN_LAYOUTS), 1u, screen::MAX_SCREEN_LAYOUTS);
    NumberOfShownScreenLayouts = numberOfScreenLayouts ? *numberOfScreenLayouts : screen::MAX_SCREEN_LAYOUTS;
    if (NumberOfShownScreenLayouts != oldNumberOfShownScreenLayouts) {
        for (unsigned i = 0; i < screen::MAX_SCREEN_LAYOUTS; ++i) {
            set_option_visible(screen::SCREEN_LAYOUTS[i], i < NumberOfShownScreenLayouts);
        }
        updated = true;
    }

    // Show/hide Hybrid screen options
    bool oldShowHybridOptions = ShowHybridOptions;
    bool oldShowVerticalLayoutOptions = ShowVerticalLayoutOptions;
    bool anyHybridLayouts = false;
    bool anyVerticalLayouts = false;
    for (unsigned i = 0; i < NumberOfShownScreenLayouts; i++) {
        optional<MelonDsDs::ScreenLayout> parsedLayout = MelonDsDs::ParseScreenLayout(get_variable(screen::SCREEN_LAYOUTS[i]));
        anyHybridLayouts |= !parsedLayout || IsHybridLayout(*parsedLayout);
        anyVerticalLayouts |= !parsedLayout || LayoutSupportsScreenGap(*parsedLayout);
    }
    ShowHybridOptions = anyHybridLayouts;
    ShowVerticalLayoutOptions = anyVerticalLayouts;

    if (ShowHybridOptions != oldShowHybridOptions) {
        set_option_visible(screen::HYBRID_SMALL_SCREEN, ShowHybridOptions);
        set_option_visible(screen::HYBRID_RATIO, ShowHybridOptions);
        updated = true;
    }

    if (ShowVerticalLayoutOptions != oldShowVerticalLayoutOptions) {
        set_option_visible(screen::SCREEN_GAP, ShowVerticalLayoutOptions);
        updated = true;
    }

    bool oldShowAlarm = ShowAlarm;
    optional<AlarmMode> alarmMode = MelonDsDs::ParseAlarmMode(get_variable(firmware::ENABLE_ALARM));
    ShowAlarm = !alarmMode || *alarmMode == AlarmMode::Enabled;
    if (ShowAlarm != oldShowAlarm) {
        set_option_visible(firmware::ALARM_HOUR, ShowAlarm);
        set_option_visible(firmware::ALARM_MINUTE, ShowAlarm);
        updated = true;
    }

#ifdef JIT_ENABLED
    // Show/hide JIT core options
    bool oldShowJitOptions = ShowJitOptions;
    optional<bool> jitEnabled = MelonDsDs::ParseBoolean(get_variable(cpu::JIT_ENABLE));
    ShowJitOptions = !jitEnabled || *jitEnabled;
    if (ShowJitOptions != oldShowJitOptions) {
        set_option_visible(cpu::JIT_BLOCK_SIZE, ShowJitOptions);
        set_option_visible(cpu::JIT_BRANCH_OPTIMISATIONS, ShowJitOptions);
        set_option_visible(cpu::JIT_LITERAL_OPTIMISATIONS, ShowJitOptions);
#ifdef HAVE_JIT_FASTMEM
        set_option_visible(cpu::JIT_FAST_MEMORY, ShowJitOptions);
#endif
        updated = true;
    }
#endif

#ifdef HAVE_NETWORKING_DIRECT_MODE
    bool oldShowWifiInterface = ShowWifiInterface;
    optional<NetworkMode> networkMode = MelonDsDs::ParseNetworkMode(get_variable(network::NETWORK_MODE));

    ShowWifiInterface = !networkMode || *networkMode == NetworkMode::Direct;
    if (ShowWifiInterface != oldShowWifiInterface) {
        set_option_visible(network::DIRECT_NETWORK_INTERFACE, ShowWifiInterface);
        updated = true;
    }
#endif

    return updated;
}

static Firmware::Language get_firmware_language(retro_language language) noexcept {
    switch (language) {
        case RETRO_LANGUAGE_ENGLISH:
        case RETRO_LANGUAGE_BRITISH_ENGLISH:
            return Firmware::Language::English;
        case RETRO_LANGUAGE_JAPANESE:
            return Firmware::Language::Japanese;
        case RETRO_LANGUAGE_FRENCH:
            return Firmware::Language::French;
        case RETRO_LANGUAGE_GERMAN:
            return Firmware::Language::German;
        case RETRO_LANGUAGE_ITALIAN:
            return Firmware::Language::Italian;
        case RETRO_LANGUAGE_SPANISH:
            return Firmware::Language::Spanish;
        case RETRO_LANGUAGE_CHINESE_SIMPLIFIED:
        case RETRO_LANGUAGE_CHINESE_TRADITIONAL:
            // The DS/DSi itself doesn't seem to distinguish between the two variants;
            // different regions just have one or the other.
            return Firmware::Language::Chinese;
        default:
            return Firmware::Language::English;
    }
}

static bool LoadBios(const string_view& name, MelonDsDs::BiosType type, std::span<u8> buffer) noexcept {
    ZoneScopedN(TracyFunction);

    auto LoadBiosImpl = [&](const string& path) -> bool {
        RFILE* file = filestream_open(path.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

        if (!file) {
            retro::error("Failed to open {} file \"{}\" for reading", type, path);
            return false;
        }

        if (int64_t size = filestream_get_size(file); size != buffer.size()) {
            retro::error("Expected {} file \"{}\" to be exactly {} bytes long, got {} bytes", type, path, buffer.size(), size);
            filestream_close(file);
            return false;
        }

        if (int64_t bytesRead = filestream_read(file, buffer.data(), buffer.size()); bytesRead != buffer.size()) {
            retro::error("Expected to read {} bytes from {} file \"{}\", got {} bytes", buffer.size(), type, path, bytesRead);
            filestream_close(file);
            return false;
        }

        filestream_close(file);
        retro::info("Successfully loaded {}-byte {} file \"{}\"", buffer.size(), type, path);

        return true;
    };

    // Prefer looking in "system/melonDS DS/${name}", but fall back to "system/${name}" if that fails

    if (optional<string> path = retro::get_system_subdir_path(name); path && LoadBiosImpl(*path)) {
        // Get the path where we're expecting a BIOS file. If it's there and we loaded it...
        return true;
    }

    if (optional<string> path = retro::get_system_path(name); path && LoadBiosImpl(*path)) {
        // Get the path where we're expecting a BIOS file. If it's there and we loaded it...
        return true;
    }

    retro::error("Failed to load {} file \"{}\"", type, name);

    return false;
}

/// Loads firmware, does not patch it.
static optional<Firmware> LoadFirmware(const string& firmwarePath) noexcept {
    ZoneScopedN("MelonDsDs::config::LoadFirmware");
    using namespace MelonDsDs;
    using namespace MelonDsDs::config::firmware;
    using namespace Platform;

    // Try to open the configured firmware dump.
    RFILE* file = filestream_open(firmwarePath.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!file) {
        // If that fails...
        retro::error("Failed to open firmware file \"{}\" for reading", firmwarePath);
        return nullopt;
    }

    int64_t fileSize = filestream_get_size(file);
    unique_ptr<u8[]> buffer = make_unique<u8[]>(fileSize);
    int64_t bytesRead = filestream_read(file, buffer.get(), fileSize);
    filestream_close(file);

    if (bytesRead != fileSize) {
        // If we couldn't read the firmware file...
        retro::error("Failed to read firmware file \"{}\"; got {} bytes, expected {} bytes", firmwarePath, bytesRead, fileSize);
        return nullopt;
    }

    // Try to load the firmware dump into the object.
    optional<Firmware> firmware = std::make_optional<Firmware>(buffer.get(), fileSize);

    if (!firmware->Buffer()) {
        // If we failed to load the firmware...
        retro::error("Failed to read opened firmware file \"{}\"", firmwarePath);
        return nullopt;
    }

    FirmwareIdentifier id = firmware->GetHeader().Identifier;
    Firmware::FirmwareConsoleType type = firmware->GetHeader().ConsoleType;
    retro::info(
        "Loaded {} firmware from \"{}\" (Identifier: {})",
        type,
        firmwarePath,
        string_view(reinterpret_cast<const char*>(id.data()), 4)
    );

    return firmware;
}

static optional<FATStorage> LoadDSiSDCardImage() noexcept {
    using namespace MelonDsDs;
    using namespace MelonDsDs::config::save;

    if (!DsiSdEnable()) return nullopt;

    return FATStorage(
        DsiSdImagePath(),
        DsiSdImageSize(),
        DsiSdReadOnly(),
        DsiSdFolderSync() ? make_optional(DsiSdFolderPath()) : nullopt
    );
}

/// Loads the DSi NAND, does not patch it
static DSi_NAND::NANDImage LoadNANDImage(const string& nandPath, const u8* es_keyY) {
    using namespace MelonDsDs;
    Platform::FileHandle* nandFile = Platform::OpenLocalFile(nandPath, Platform::FileMode::ReadWriteExisting);
    if (!nandFile) {
        throw dsi_nand_missing_exception(nandPath);
    }

    DSi_NAND::NANDImage nand(nandFile, es_keyY);
    if (!nand) {
        throw dsi_nand_corrupted_exception(nandPath);
    }
    retro::debug("Opened the DSi NAND image file at {}", nandPath);

    return nand;
}

void MelonDsDs::CoreConfig::CustomizeFirmware(Firmware& firmware) {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::firmware;
    using namespace Platform;

    // We don't need to save the whole firmware, just the part that may actually change.
    optional<string> wfcsettingspath = retro::get_system_subdir_path(GeneratedFirmwareSettingsPath());
    if (!wfcsettingspath) {
        throw environment_exception("No system directory is available");
    }

    const Firmware::FirmwareHeader& header = firmware.GetHeader();

    // If using generated firmware, we keep the wi-fi settings on the host disk separately.
    // Wi-fi access point data includes Nintendo WFC settings,
    // and if we didn't keep them then the player would have to reset them in each session.
    if (RFILE* file = filestream_open(wfcsettingspath->c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE)) {
        // If we have Wi-fi settings to load...
        constexpr unsigned TOTAL_WFC_SETTINGS_SIZE = 3 * (sizeof(Firmware::WifiAccessPoint) + sizeof(Firmware::ExtendedWifiAccessPoint));

        // The access point and extended access point segments might
        // be in different locations depending on the firmware revision,
        // but our generated firmware always keeps them next to each other.
        // (Extended access points first, then regular ones.)
        u8* userdata = firmware.GetExtendedAccessPointPosition();

        if (filestream_read(file, userdata, TOTAL_WFC_SETTINGS_SIZE) != TOTAL_WFC_SETTINGS_SIZE) {
            // If we couldn't read the Wi-fi settings from this file...
            retro::warn("Failed to read Wi-fi settings from \"{}\"; using defaults instead\n", *wfcsettingspath);

            firmware.GetAccessPoints() = {
                Firmware::WifiAccessPoint(header.ConsoleType == Firmware::FirmwareConsoleType::DSi ? 1 : 0),
                Firmware::WifiAccessPoint(),
                Firmware::WifiAccessPoint(),
            };

            firmware.GetExtendedAccessPoints() = {
                Firmware::ExtendedWifiAccessPoint(),
                Firmware::ExtendedWifiAccessPoint(),
                Firmware::ExtendedWifiAccessPoint(),
            };
        }

        filestream_close(file);
    }

    // If we don't have Wi-fi settings to load,
    // then the defaults will have already been populated by the constructor.

    if (header.Identifier != GENERATED_FIRMWARE_IDENTIFIER && header.ConsoleType == Firmware::FirmwareConsoleType::DS) {
        // If we're using externally-loaded DS (not DSi) firmware...

        u8 chk1[0x180], chk2[0x180];

        // I don't really know how this works, it's just adapted from upstream
        memcpy(chk1, firmware.Buffer(), sizeof(chk1));
        memcpy(chk2, firmware.Buffer() + firmware.Length() - 0x380, sizeof(chk2));

        memset(&chk1[0x0C], 0, 8);
        memset(&chk2[0x0C], 0, 8);

        if (!memcmp(chk1, chk2, sizeof(chk1))) {
            constexpr const char* const WARNING_MESSAGE =
                "Corrupted firmware detected!\n"
                "Any game that alters Wi-fi settings will break this firmware, even on real hardware.\n";

            if (showBiosWarnings) {
                retro::set_warn_message(WARNING_MESSAGE);
            } else {
                retro::warn(WARNING_MESSAGE);
            }
        }
    }

    Firmware::UserData& currentData = firmware.GetEffectiveUserData();

    // setting up username
    if (_usernameMode != UsernameMode::Firmware) {
        // If we want to override the existing username...
        string username = MelonDsDs::config::GetUsername(_usernameMode);
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string convertedUsername = converter.from_bytes(username);
        size_t usernameLength = std::min(convertedUsername.length(), (size_t) MelonDsDs::config::DS_NAME_LIMIT);
        currentData.NameLength = usernameLength;

        memcpy(currentData.Nickname, convertedUsername.data(), usernameLength * sizeof(char16_t));
    }

    switch (_language) {
        case FirmwareLanguage::Auto:

            if (optional<retro_language> retroLanguage = retro::get_language()) {
                currentData.Settings &= ~Firmware::Language::Reserved; // clear the existing language bits
                currentData.Settings |= static_cast<Firmware::Language>(get_firmware_language(*retroLanguage));
            } else {
                retro::warn("Failed to get language from frontend; defaulting to existing firmware value");
            }
            break;
        case FirmwareLanguage::Default:
            // do nothing, leave the existing language in place
            break;
        default:
            currentData.Settings &= ~Firmware::Language::Reserved;
            currentData.Settings |= static_cast<Firmware::Language>(_language);
            break;
    }

    if (_favoriteColor != Color::Default) {
        currentData.FavoriteColor = static_cast<u8>(_favoriteColor);
    }

    if (_birthdayMonth != 0) {
        // If the frontend specifies a birth month (rather than using the existing value)...
        currentData.BirthdayMonth = _birthdayMonth;
    }

    if (_birthdayDay != 0) {
        // If the frontend specifies a birthday (rather than using the existing value)...
        currentData.BirthdayDay = _birthdayDay;
    }

    if (_dnsServer) {
        firmware.GetAccessPoints()[0].PrimaryDns = *_dnsServer;
        firmware.GetAccessPoints()[0].SecondaryDns = *_dnsServer;
    }

    if (_macAddress) {
        melonDS::MacAddress mac = *_macAddress;
        mac[0] &= 0xFC; // ensure the MAC isn't a broadcast MAC
        firmware.GetHeader().MacAddr = mac;
    }

    // fix touchscreen coords
    currentData.TouchCalibrationADC1[0] = 0;
    currentData.TouchCalibrationADC1[1] = 0;
    currentData.TouchCalibrationPixel1[0] = 0;
    currentData.TouchCalibrationPixel1[1] = 0;
    currentData.TouchCalibrationADC2[0] = 255 << 4;
    currentData.TouchCalibrationADC2[1] = 191 << 4;
    currentData.TouchCalibrationPixel2[0] = 255;
    currentData.TouchCalibrationPixel2[1] = 191;

    firmware.UpdateChecksums();
}

// First, load the system files
// Then, validate the system files
// Then, fall back to other system files if needed and possible
// If fallback is needed and not possible, throw an exception
// Finally, install the system files
static NDSArgs GetNdsArgs(const NDSHeader* header, MelonDsDs::BootMode bootMode, MelonDsDs::SysfileMode sysfileMode) {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::system;
    using namespace MelonDsDs;
    retro_assert(!(header && header->IsDSiWare()));

    std::array<u8, ARM7BIOSSize> arm7bios;
    // The rules are somewhat complicated.
    // - Bootable firmware is required if booting without content.
    // - All system files must be native or all must be built-in. (No mixing.)
    // - If BIOS files are built-in, then Direct Boot mode must be used
    optional<Firmware> firmware;
    if (sysfileMode == SysfileMode::Native) {
        optional<string> firmwarePath = retro::get_system_path(FirmwarePath());
        if (!firmwarePath) {
            retro::error("Failed to get system directory");
        }

        firmware = firmwarePath ? LoadFirmware(*firmwarePath) : nullopt;
    }

    if (!header && !(firmware && firmware->IsBootable())) {
        // If we're trying to boot into the NDS menu, but we didn't load bootable firmware...
        if (sysfileMode == SysfileMode::Native) {
            throw nds_firmware_not_bootable_exception(FirmwarePath());
        } else {
            throw nds_firmware_not_bootable_exception();
        }
    }

    if (!firmware) {
        // If we haven't loaded any firmware...
        if (sysfileMode == SysfileMode::Native) {
            // ...but we were trying to...
            retro::warn("Falling back to built-in firmware");
        }
        firmware = make_optional<Firmware>(static_cast<int>(MelonDsDs::ConsoleType::DS));
    }

    if (sysfileMode == SysfileMode::BuiltIn) {
        retro::debug("Not loading native ARM BIOS files");
    }

    NDSArgs ndsargs {};

    // Try to load the ARM7 and ARM9 BIOS files (but don't bother with the ARM9 BIOS if the ARM7 BIOS failed)
    bool bios7Loaded = (sysfileMode == SysfileMode::Native) && LoadBios(Bios7Path(), BiosType::Arm7, ndsargs.ARM7BIOS);
    bool bios9Loaded = bios7Loaded && LoadBios(Bios9Path(), BiosType::Arm9, ndsargs.ARM9BIOS);

    if (sysfileMode == SysfileMode::Native && !(bios7Loaded && bios9Loaded)) {
        // If we're trying to load native BIOS files, but at least one of them failed...
        retro::warn("Falling back to FreeBIOS");
    }

    // Now that we've loaded the system files, let's see if we can use them

    if (bootMode == MelonDsDs::BootMode::Native && !(bios7Loaded && bios9Loaded && firmware->IsBootable())) {
        // If we want to try a native boot, but the BIOS files aren't all native or the firmware isn't bootable...
        retro::warn("Native boot requires bootable firmware and native BIOS files; forcing Direct Boot mode");

        _bootMode = MelonDsDs::BootMode::Direct;
    }

    if (!header && !(firmware && firmware->IsBootable() && bios7Loaded && bios9Loaded)) {
        // If we're trying to boot into the NDS menu, but we don't have all the required files...
        throw nds_sysfiles_incomplete_exception();
    }


    if (bios7Loaded && bios9Loaded) {
        retro::debug("Installed native ARM7 and ARM9 NDS BIOS images");
    } else {
        ndsargs.ARM9BIOS = bios_arm9_bin;
        ndsargs.ARM7BIOS = bios_arm7_bin;
        retro::debug("Installed built-in ARM7 and ARM9 NDS BIOS images");
    }

    CustomizeFirmware(*firmware);
    ndsargs.Firmware = std::move(*firmware);

    return ndsargs;
}

static void CustomizeNAND(DSi_NAND::NANDMount& mount, const NDSHeader* header, string_view nandName) {
    using namespace MelonDsDs::config::system;
    using namespace MelonDsDs::config::firmware;
    using namespace MelonDsDs;

    DSi_NAND::DSiSerialData dataS {};
    memset(&dataS, 0, sizeof(dataS));
    if (!mount.ReadSerialData(dataS)) {
        throw emulator_exception("Failed to read serial data from NAND image");
    }

    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...

        u32 consoleRegionMask = (1 << (int)dataS.Region);
        if (!(consoleRegionMask & header->DSiRegionMask)) {
            // If the console's region isn't compatible with the game's regions...
            throw dsi_region_mismatch_exception(nandName, dataS.Region, header->DSiRegionMask);
        }

        retro::debug("Console region ({}) and game regions ({}) match", dataS.Region, header->DSiRegionMask);
    }

    DSi_NAND::DSiFirmwareSystemSettings settings;
    if (!mount.ReadUserData(settings)) {
        throw emulator_exception("Failed to read user data from NAND image");
    }

    // Right now, I only modify the user data with the firmware overrides defined by core options
    // If there are any problems, I may want to completely synchronize the user data and firmware myself.

    // setting up username
    if (_usernameMode != UsernameMode::Firmware) {
        // If we want to override the existing username...
        string username = MelonDsDs::config::GetUsername(_usernameMode);
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string convertedUsername = converter.from_bytes(username);
        size_t usernameLength = std::min(convertedUsername.length(), (size_t) MelonDsDs::config::DS_NAME_LIMIT);

        memset(settings.Nickname, 0, sizeof(settings.Nickname));
        memcpy(settings.Nickname, convertedUsername.data(), usernameLength * sizeof(char16_t));
    }

    switch (_language) {
        case FirmwareLanguage::Auto:
            if (optional<retro_language> retroLanguage = retro::get_language()) {
                // If we can't query the frontend's language, just leave that part of the firmware alone
                Firmware::Language firmwareLanguage = get_firmware_language(*retroLanguage);
                if (dataS.SupportedLanguages & (1 << firmwareLanguage)) {
                    // If the NAND supports the frontend's language...
                    settings.Language = firmwareLanguage;
                    settings.ConfigFlags |= (1 << 2); // LanguageSet? (usually 1) flag
                } else {
                    retro::warn("The frontend's preferred language ({}) isn't supported by this NAND image; not overriding it.", *retroLanguage);
                }
            } else {
                retro::warn("Can't query the frontend's preferred language, not overriding it.");
            }

            break;

        case FirmwareLanguage::Default:
            // do nothing, leave the existing language in place
            break;
        default: {
            Firmware::Language firmwareLanguage = static_cast<Firmware::Language>(_language);
            if (dataS.SupportedLanguages & (1 << firmwareLanguage)) {
                // If the NAND supports the core option's specified language...
                settings.Language = firmwareLanguage;
                settings.ConfigFlags |= (1 << 2); // LanguageSet? (usually 1) flag
            } else {
                retro::warn("The configured language ({}) is not supported by this NAND image; not overriding it.", firmwareLanguage);
            }

        }
            break;
    }
    settings.ConfigFlags |= (1 << 24); // EULA flag (agreed)

    if (_favoriteColor != Color::Default) {
        settings.FavoriteColor = static_cast<u8>(_favoriteColor);
    }

    if (_birthdayMonth != 0) {
        // If the frontend specifies a birth month (rather than using the existing value)...
        settings.BirthdayMonth = _birthdayMonth;
    }

    if (_birthdayDay != 0) {
        // If the frontend specifies a birthday (rather than using the existing value)...
        settings.BirthdayDay = _birthdayDay;
    }

    switch (_alarmMode) {
        case AlarmMode::Disabled:
            settings.AlarmEnable = false;
            break;
        case AlarmMode::Default:
            // do nothing, leave the existing alarm in place
            break;
        case AlarmMode::Enabled:
            settings.AlarmEnable = true;
            if (_alarmHour) {
                settings.AlarmHour = *_alarmHour;
            }
            if (_alarmMinute) {
                settings.AlarmMinute = *_alarmMinute;
            }
            break;
    }

    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...

        memcpy(&settings.SystemMenuMostRecentTitleID[0], &header->DSiTitleIDLow, sizeof(header->DSiTitleIDLow));
        memcpy(&settings.SystemMenuMostRecentTitleID[4], &header->DSiTitleIDHigh, sizeof(header->DSiTitleIDHigh));
    }

    // The DNS entries and MAC address aren't stored on the NAND,
    // so we don't need to try to update them here.

    // fix touchscreen coords
    settings.TouchCalibrationADC1 = {0, 0};
    settings.TouchCalibrationPixel1 = {0, 0};
    settings.TouchCalibrationADC2 = {255 << 4, 191 << 4};
    settings.TouchCalibrationPixel2 = {255, 191};

    settings.UpdateHash();

    if (!mount.ApplyUserData(settings)) {
        throw emulator_exception("Failed to write user data to NAND image");
    }
}

static DSiArgs GetDSiArgs(const NDSHeader* header) {
    ZoneScopedN("MelonDsDs::config::InitDsiSystemConfig");
    using namespace MelonDsDs::config::system;
    using namespace MelonDsDs::config::firmware;
    using namespace MelonDsDs;

    retro_assert(_consoleType == ConsoleType::DSi);

    string_view nandName = DsiNandPath();
    if (nandName == MelonDsDs::config::values::NOT_FOUND) {
        throw dsi_no_nand_found_exception();
    }

    if (DsiFirmwarePath() == MelonDsDs::config::values::NOT_FOUND) {
        throw dsi_no_firmware_found_exception();
    }

    // DSi mode requires all native BIOS files
    std::array<u8, DSiBIOSSize> arm7i;
    if (!LoadBios(DsiBios7Path(), BiosType::Arm7i, arm7i)) {
        throw dsi_missing_bios_exception(BiosType::Arm7i, DsiBios7Path());
    }

    std::array<u8, DSiBIOSSize> arm9i;
    if (!LoadBios(DsiBios9Path(), BiosType::Arm9i, arm9i)) {
        throw dsi_missing_bios_exception(BiosType::Arm9i, DsiBios9Path());
    }

    std::array<u8, ARM7BIOSSize> arm7;
    if (!LoadBios(Bios7Path(), BiosType::Arm7, arm7)) {
        throw dsi_missing_bios_exception(BiosType::Arm7, Bios7Path());
    }

    std::array<u8, ARM9BIOSSize> arm9;
    if (!LoadBios(Bios9Path(), BiosType::Arm9, arm9)) {
        throw dsi_missing_bios_exception(BiosType::Arm9, Bios9Path());
    }

    optional<string> firmwarePath = retro::get_system_path(DsiFirmwarePath());
    retro_assert(firmwarePath.has_value());
    // If we couldn't get the system directory, we wouldn't have gotten this far

    optional<Firmware> firmware = LoadFirmware(*firmwarePath);
    if (!firmware) {
        throw firmware_missing_exception(DsiFirmwarePath());
    }

    if (firmware->GetHeader().ConsoleType != Firmware::FirmwareConsoleType::DSi) {
        retro::warn("Expected firmware of type DSi, got {}", firmware->GetHeader().ConsoleType);
        throw wrong_firmware_type_exception(DsiFirmwarePath(), MelonDsDs::ConsoleType::DSi, firmware->GetHeader().ConsoleType);
    }
    // DSi firmware isn't bootable, so we don't need to check for that here.

    retro::debug("Installed native ARM7, ARM9, DSi ARM7, and DSi ARM9 BIOS images.");

    // TODO: Customize the NAND first, then use the final value of TWLCFG to patch the firmware
    CustomizeFirmware(*firmware);

    optional<string> nandPath = retro::get_system_path(nandName);
    if (!nandPath) {
        throw environment_exception("Failed to get the system directory, which means the NAND image can't be loaded.");
    }

    DSi_NAND::NANDImage nand = LoadNANDImage(*nandPath, &arm7i[0x8308]);
    {
        DSi_NAND::NANDMount mount(nand);
        if (!mount) {
            throw dsi_nand_corrupted_exception(nandName);
        }
        retro::debug("Opened and mounted the DSi NAND image file at {}", *nandPath);

        CustomizeNAND(mount, header, nandName);
    }

    DSiArgs dsiargs {
        {
            .NDSROM = nullptr, // Inserted later
            .GBAROM = nullptr, // Irrelevant on DSi
            .ARM9BIOS = arm9,
            .ARM7BIOS = arm7,
            .Firmware = std::move(*firmware),
        },
        arm9i,
        arm7i,
        std::move(nand),
        LoadDSiSDCardImage(),
    };

    return dsiargs;
}

static void MelonDsDs::config::apply_system_options(MelonDsDs::CoreState& core, const NDSHeader* header) {
    ZoneScopedN("MelonDsDs::config::apply_system_options");
    using namespace MelonDsDs::config::system;
    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...
        _consoleType = ConsoleType::DSi;
        retro::warn("Forcing DSi mode for DSiWare game");
    }

    if (_consoleType == ConsoleType::DSi) {
        // If we're in DSi mode...
        core.Console = std::make_unique<DSi>(GetDSiArgs(header));
    } else {
        // If we're in DS mode...
        core.Console = std::make_unique<NDS>(GetNdsArgs(header, _bootMode, _sysfileMode));
    }

    NDS::Current = core.Console.get();
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
        flagNames.push_back("Loopback");
    }

    if (interface.flags & PCAP_IF_UP) {
        flagNames.push_back("Up");
    }

    if (interface.flags & PCAP_IF_RUNNING) {
        flagNames.push_back("Running");
    }

    if (interface.flags & PCAP_IF_WIRELESS) {
        flagNames.push_back("Wireless");
    }

    switch (interface.flags & PCAP_IF_CONNECTION_STATUS) {
        case PCAP_IF_CONNECTION_STATUS_UNKNOWN:
            flagNames.push_back("UnknownStatus");
            break;
        case PCAP_IF_CONNECTION_STATUS_CONNECTED:
            flagNames.push_back("Connected");
            break;
        case PCAP_IF_CONNECTION_STATUS_DISCONNECTED:
            flagNames.push_back("Disconnected");
            break;
        case PCAP_IF_CONNECTION_STATUS_NOT_APPLICABLE:
            flagNames.push_back("ConnectionStatusNotApplicable");
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
