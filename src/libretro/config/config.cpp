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
#include "core.hpp"
#include "embedded/melondsds_default_wfc_config.h"
#include "environment.hpp"
#include "exceptions.hpp"
#include "format.hpp"
#include "input.hpp"
#include "libretro.hpp"
#include "microphone.hpp"
#include "opengl.hpp"
#include "render.hpp"
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

namespace melonds::config {
    static void set_core_options() noexcept;
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

    static void parse_jit_options() noexcept;
    static void parse_osd_options() noexcept;
    static void parse_system_options() noexcept;
    static void parse_firmware_options() noexcept;
    static void parse_audio_options() noexcept;
    static void parse_network_options() noexcept;

    /// @returns true if the OpenGL state needs to be rebuilt
    static bool parse_video_options(bool initializing) noexcept;
    static void parse_screen_options() noexcept;
    static void parse_homebrew_save_options(const NDSHeader* header);
    static void parse_dsi_storage_options() noexcept;

    static void apply_system_options(melondsds::CoreState& core, const NDSHeader* header);

    static void apply_audio_options(NDS& nds) noexcept;
    static void apply_save_options(const NDSHeader* header);
    static void apply_screen_options(ScreenLayoutData& screenLayout, InputState& inputState) noexcept;
    static void apply_network_options() noexcept;

    namespace audio {
        melonds::MicButtonMode _micButtonMode = melonds::MicButtonMode::Hold;
        melonds::MicButtonMode MicButtonMode() noexcept { return _micButtonMode; }

        melonds::MicInputMode _micInputMode;
        melonds::MicInputMode MicInputMode() noexcept { return _micInputMode; }

        melonds::BitDepth _bitDepth;
        melonds::BitDepth BitDepth() noexcept { return _bitDepth; }

        melonds::AudioInterpolation _interpolation;
        melonds::AudioInterpolation Interpolation() noexcept { return _interpolation; }
    }

    namespace firmware {
        [[deprecated("Override settings individually")]]
        static bool _firmwareSettingsOverrideEnable = false;
        bool FirmwareSettingsOverrideEnable() noexcept { return _firmwareSettingsOverrideEnable; }

        static AlarmMode _alarmMode;
        static optional<unsigned> _alarmHour;
        static optional<unsigned> _alarmMinute;
        static FirmwareLanguage _language;
        static unsigned _birthdayMonth = 1;
        static unsigned _birthdayDay = 1;
        static Color _favoriteColor;
        static UsernameMode _usernameMode;
        [[deprecated("Make this a flat array instead")]] static string _message;
        static optional<MacAddress> _macAddress;
        static optional<IpAddress> _dnsServer;
    }

    namespace jit {
#ifdef HAVE_JIT
        static bool _jitEnable;
        bool Enable() noexcept { return _jitEnable; }

        unsigned _maxBlockSize;
        unsigned MaxBlockSize() noexcept { return _maxBlockSize; }

        static bool _literalOptimizations;
        bool LiteralOptimizations() noexcept { return _literalOptimizations; }

        static bool _branchOptimizations;
        bool BranchOptimizations() noexcept { return _branchOptimizations; }
#else
        bool Enable() noexcept { return false; }
        unsigned MaxBlockSize() noexcept { return 0; }
        bool LiteralOptimizations() noexcept { return false; }
        bool BranchOptimizations() noexcept { return false; }
#endif

#if defined(HAVE_JIT) && defined(HAVE_JIT_FASTMEM)
        static bool _fastMemory;
        bool FastMemory() noexcept { return _fastMemory; }
#else
        bool FastMemory() noexcept { return false; }
#endif
    }

    namespace net {
#ifdef HAVE_NETWORKING
        static enum NetworkMode _networkMode;
        enum NetworkMode NetworkMode() noexcept { return _networkMode; }
#endif

#ifdef HAVE_NETWORKING_DIRECT_MODE
        static bool _interfacesInitialized = false;
        static char _networkInterface[PATH_MAX];
        string_view NetworkInterface() noexcept { return _networkInterface; }
#endif
    }

    namespace osd {
#ifndef NDEBUG
        static bool showPointerCoordinates = false;
        [[nodiscard]] bool ShowPointerCoordinates() noexcept { return showPointerCoordinates; }
#endif

        static bool showUnsupportedFeatureWarnings = true;
        [[nodiscard]] bool ShowUnsupportedFeatureWarnings() noexcept { return showUnsupportedFeatureWarnings; }

        static bool showMicState = true;
        [[nodiscard]] bool ShowMicState() noexcept { return showMicState; }

        static bool showCameraState = true;
        [[nodiscard]] bool ShowCameraState() noexcept { return showCameraState; }

        static bool showBiosWarnings = true;
        [[nodiscard]] bool ShowBiosWarnings() noexcept { return showBiosWarnings; }

        static bool showCurrentLayout = true;
        [[nodiscard]] bool ShowCurrentLayout() noexcept { return showCurrentLayout; }

        static bool showLidState = false;
        [[nodiscard]] bool ShowLidState() noexcept { return showLidState; }

        static bool showBrightnessState = false;
        [[nodiscard]] bool ShowBrightnessState() noexcept { return showBrightnessState; }
    }

    namespace save {
        bool _dldiEnable;
        bool DldiEnable() noexcept { return _dldiEnable; }

        bool _dldiFolderSync;
        bool DldiFolderSync() noexcept { return _dldiFolderSync; }

        [[deprecated("Make this a flat array instead")]] string _dldiFolderPath;
        string DldiFolderPath() noexcept { return _dldiFolderPath; }

        bool _dldiReadOnly;
        bool DldiReadOnly() noexcept { return _dldiReadOnly; }

        [[deprecated("Make this a flat array instead")]] string _dldiImagePath;
        string DldiImagePath() noexcept { return _dldiImagePath; }

        unsigned _dldiImageSize;
        unsigned DldiImageSize() noexcept { return _dldiImageSize; }

        bool _dsiSdEnable;
        bool DsiSdEnable() noexcept { return _dsiSdEnable; }

        bool _dsiSdFolderSync;
        bool DsiSdFolderSync() noexcept { return _dsiSdFolderSync; }

        [[deprecated("Make this a flat array instead")]] string _dsiSdFolderPath;
        string DsiSdFolderPath() noexcept { return _dsiSdFolderPath; }

        bool _dsiSdReadOnly;
        bool DsiSdReadOnly() noexcept { return _dsiSdReadOnly; }

        [[deprecated("Make this a flat array instead")]] string _dsiSdImagePath;
        string DsiSdImagePath() noexcept { return _dsiSdImagePath; }

        unsigned _dsiSdImageSize;
        unsigned DsiSdImageSize() noexcept { return _dsiSdImageSize; }

        unsigned _flushDelay = 120; // TODO: Make configurable
        unsigned FlushDelay() noexcept { return _flushDelay; }
    }

    namespace screen {
        static unsigned _numberOfScreenLayouts = 1;
        unsigned NumberOfScreenLayouts() noexcept { return _numberOfScreenLayouts; }

        static std::array<melonds::ScreenLayout, MAX_SCREEN_LAYOUTS> _screenLayouts;
        std::array<melonds::ScreenLayout, MAX_SCREEN_LAYOUTS> ScreenLayouts() noexcept { return _screenLayouts; }

        static unsigned _screenGap;
        unsigned ScreenGap() noexcept { return _screenGap; }

        static unsigned _hybridRatio;
        unsigned HybridRatio() noexcept { return _hybridRatio; }

        static melonds::HybridSideScreenDisplay _smallScreenLayout;
        melonds::HybridSideScreenDisplay SmallScreenLayout() noexcept { return _smallScreenLayout; }

        static unsigned _cursorSize = 2.0f;
        float CursorSize() noexcept { return _cursorSize; }

        static enum CursorMode _cursorMode = CursorMode::Always;
        enum CursorMode CursorMode() noexcept { return _cursorMode; }

        static unsigned _cursorTimeout;
        unsigned CursorTimeout() noexcept { return _cursorTimeout; }

        static enum TouchMode _touchMode;
        enum TouchMode TouchMode() noexcept { return _touchMode; }
    }

    namespace system {
        static melonds::ConsoleType _consoleType;
        melonds::ConsoleType ConsoleType() noexcept { return _consoleType; }

        static BootMode _bootMode;
        bool DirectBoot() noexcept { return _bootMode == BootMode::Direct; }

        static SysfileMode _sysfileMode;
        bool ExternalBiosEnable() noexcept { return _sysfileMode == SysfileMode::Native; }

        static unsigned _dsPowerOkayThreshold = 20;
        unsigned DsPowerOkayThreshold() noexcept { return _dsPowerOkayThreshold; }

        static unsigned _powerUpdateInterval;
        unsigned PowerUpdateInterval() noexcept { return _powerUpdateInterval; }

        // TODO: Allow these paths to be customized
        string_view Bios9Path() noexcept { return "bios9.bin"; }
        string_view Bios7Path() noexcept { return "bios7.bin"; }
        string_view DsiBios9Path() noexcept { return "dsi_bios9.bin"; }
        string_view DsiBios7Path() noexcept { return "dsi_bios7.bin"; }

        static char _firmwarePath[PATH_MAX];
        string_view FirmwarePath() noexcept { return _firmwarePath; }

        static char _dsiFirmwarePath[PATH_MAX];
        string_view DsiFirmwarePath() noexcept { return _dsiFirmwarePath; }

        static char _dsiNandPath[PATH_MAX];
        string_view DsiNandPath() noexcept { return _dsiNandPath; }

        string_view GeneratedFirmwareSettingsPath() noexcept { return "melonDS DS/wfcsettings.bin"; }
    }

    namespace video {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES) || defined(HAVE_THREADS)
        static melonDS::RenderSettings _renderSettings = {false, 1, false};
        melonDS::RenderSettings RenderSettings() noexcept { return _renderSettings; }
#else
        RenderSettings RenderSettings() noexcept { return {false, 1, false}; }
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
        static melonds::Renderer _configuredRenderer;
        melonds::Renderer ConfiguredRenderer() noexcept { return _configuredRenderer; }
#else
        melonds::Renderer ConfiguredRenderer() noexcept { return melonds::Renderer::Software; }
#endif

        static melonds::ScreenFilter _screenFilter;
        melonds::ScreenFilter ScreenFilter() noexcept { return _screenFilter; }

        int ScaleFactor() noexcept { return RenderSettings().GL_ScaleFactor; }
    }
}


void melonds::InitConfig(
    melondsds::CoreState& core,
    const melonDS::NDSHeader* header,
    ScreenLayoutData& screenLayout,
    InputState& inputState
) {
    ZoneScopedN("melonds::InitConfig");
    config::set_core_options();
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

    retro_assert(core.Console == nullptr);    config::apply_system_options(core, header);
    retro_assert(core.Console != nullptr);
    config::apply_save_options(header);
    config::apply_audio_options(*core.Console);
    config::apply_screen_options(screenLayout, inputState);

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (melonds::opengl::UsingOpenGl() && (openGlNeedsRefresh || screenLayout.Dirty())) {
        // If we're using OpenGL and the settings changed, or the screen layout changed...
        melonds::opengl::RequestOpenGlRefresh();
    }
#endif

    if (melonds::render::CurrentRenderer() == Renderer::None) {
        screenLayout.Update(config::video::ConfiguredRenderer());
    } else {
        screenLayout.Update(melonds::render::CurrentRenderer());
    }

    update_option_visibility();
}

void melonds::UpdateConfig(melondsds::CoreState& core, ScreenLayoutData& screenLayout, InputState& inputState) noexcept {
    ZoneScopedN("melonds::config::UpdateConfig");
    config::parse_audio_options();
    bool openGlNeedsRefresh = config::parse_video_options(false);
    config::parse_screen_options();
    config::parse_osd_options();

    retro_assert(core.Console != nullptr);

    config::apply_audio_options(*core.Console);
    config::apply_screen_options(screenLayout, inputState);

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (melonds::opengl::UsingOpenGl() && (openGlNeedsRefresh || screenLayout.Dirty())) {
        // If we're using OpenGL and the settings changed, or the screen layout changed...
        melonds::opengl::RequestOpenGlRefresh();
    }
#endif

    update_option_visibility();
}

bool melonds::update_option_visibility() {
    ZoneScopedN("melonds::update_option_visibility");
    using namespace melonds::config;
    using namespace melonds::config::visibility;
    using retro::environment;
    using retro::get_variable;
    using retro::set_option_visible;
    bool updated = false;

    retro::debug("melonds::update_option_visibility");

    // Convention: if an option is not found, show any dependent options
    bool oldShowMicButtonMode = ShowMicButtonMode;
    optional<MicInputMode> micInputMode = ParseMicInputMode(get_variable(audio::MIC_INPUT));
    ShowMicButtonMode = !micInputMode || *micInputMode != MicInputMode::None;
    if (ShowMicButtonMode != oldShowMicButtonMode) {
        set_option_visible(audio::MIC_INPUT_BUTTON, ShowMicButtonMode);
        updated = true;
    }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    // Show/hide OpenGL core options
    bool oldShowOpenGlOptions = ShowOpenGlOptions;
    bool oldShowSoftwareRenderOptions = ShowSoftwareRenderOptions;
    optional<Renderer> renderer = ParseRenderer(get_variable(video::RENDER_MODE));
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
    optional<ConsoleType> consoleType = ParseConsoleType(get_variable(system::CONSOLE_MODE));
    ShowDsiOptions = !consoleType || *consoleType == ConsoleType::DSi;
    if (ShowDsiOptions != oldShowDsiOptions) {
        set_option_visible(config::system::FIRMWARE_DSI_PATH, ShowDsiOptions);
        set_option_visible(config::storage::DSI_NAND_PATH, ShowDsiOptions);
        set_option_visible(storage::DSI_SD_SAVE_MODE, ShowDsiOptions);
        updated = true;
    }

    bool oldShowDsiSdCardOptions = ShowDsiSdCardOptions && ShowDsiOptions;
    optional<bool> dsiSdEnable = ParseBoolean(get_variable(storage::DSI_SD_SAVE_MODE));
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
    optional<bool> homebrewSdCardEnabled = ParseBoolean(get_variable(storage::HOMEBREW_SAVE_MODE));
    ShowHomebrewSdOptions = !homebrewSdCardEnabled || *homebrewSdCardEnabled;
    if (ShowHomebrewSdOptions != oldShowHomebrewSdOptions) {
        set_option_visible(storage::HOMEBREW_READ_ONLY, ShowHomebrewSdOptions);
        set_option_visible(storage::HOMEBREW_SYNC_TO_HOST, ShowHomebrewSdOptions);
        updated = true;
    }

    bool oldShowCursorTimeout = ShowCursorTimeout;
    optional<melonds::CursorMode> cursorMode = ParseCursorMode(get_variable(screen::SHOW_CURSOR));
    ShowCursorTimeout = !cursorMode || *cursorMode == melonds::CursorMode::Timeout;
    if (ShowCursorTimeout != oldShowCursorTimeout) {
        set_option_visible(screen::CURSOR_TIMEOUT, ShowCursorTimeout);
        updated = true;
    }

    unsigned oldNumberOfShownScreenLayouts = NumberOfShownScreenLayouts;
    optional<unsigned> numberOfScreenLayouts = ParseIntegerInRange(get_variable(screen::NUMBER_OF_SCREEN_LAYOUTS), 1u, screen::MAX_SCREEN_LAYOUTS);
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
        optional<melonds::ScreenLayout> parsedLayout = ParseScreenLayout(get_variable(screen::SCREEN_LAYOUTS[i]));
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
    optional<AlarmMode> alarmMode = ParseAlarmMode(get_variable(firmware::ENABLE_ALARM));
    ShowAlarm = !alarmMode || *alarmMode == AlarmMode::Enabled;
    if (ShowAlarm != oldShowAlarm) {
        set_option_visible(firmware::ALARM_HOUR, ShowAlarm);
        set_option_visible(firmware::ALARM_MINUTE, ShowAlarm);
        updated = true;
    }

#ifdef JIT_ENABLED
    // Show/hide JIT core options
    bool oldShowJitOptions = ShowJitOptions;
    optional<bool> jitEnabled = ParseBoolean(get_variable(cpu::JIT_ENABLE));
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
    optional<NetworkMode> networkMode = ParseNetworkMode(get_variable(network::NETWORK_MODE));

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

static void melonds::config::parse_osd_options() noexcept {
    ZoneScopedN("melonds::config::parse_osd_options");
    using namespace melonds::config::osd;
    using retro::get_variable;

#ifndef NDEBUG
    if (optional<bool> value = ParseBoolean(get_variable(osd::POINTER_COORDINATES))) {
        showPointerCoordinates = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", POINTER_COORDINATES, values::DISABLED);
        showPointerCoordinates = false;
    }
#endif

    if (optional<bool> value = ParseBoolean(get_variable(osd::UNSUPPORTED_FEATURES))) {
        showUnsupportedFeatureWarnings = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", UNSUPPORTED_FEATURES, values::ENABLED);
        showUnsupportedFeatureWarnings = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::MIC_STATE))) {
        showMicState = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", MIC_STATE, values::ENABLED);
        showMicState = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::CAMERA_STATE))) {
        showCameraState = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CAMERA_STATE, values::ENABLED);
        showCameraState = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::BIOS_WARNINGS))) {
        showBiosWarnings = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", BIOS_WARNINGS, values::ENABLED);
        showBiosWarnings = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::CURRENT_LAYOUT))) {
        showCurrentLayout = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CURRENT_LAYOUT, values::ENABLED);
        showCurrentLayout = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::LID_STATE))) {
        showLidState = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", LID_STATE, values::DISABLED);
        showLidState = false;
    }
}

static void melonds::config::parse_jit_options() noexcept {
    ZoneScopedN("melonds::config::parse_jit_options");
#ifdef HAVE_JIT
    using namespace melonds::config::jit;
    using retro::get_variable;

    if (const char* value = get_variable(cpu::JIT_ENABLE); !string_is_empty(value)) {
        _jitEnable = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_ENABLE, values::ENABLED);
        _jitEnable = true;
    }

    if (const char* value = get_variable(cpu::JIT_BLOCK_SIZE); !string_is_empty(value)) {
        _maxBlockSize = std::stoi(value);
    } else {
        retro::warn("Failed to get value for {}; defaulting to 32", cpu::JIT_BLOCK_SIZE);
        _maxBlockSize = 32;
    }

    if (const char* value = get_variable(cpu::JIT_BRANCH_OPTIMISATIONS); !string_is_empty(value)) {
        _branchOptimizations = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_BRANCH_OPTIMISATIONS, values::ENABLED);
        _branchOptimizations = true;
    }

    if (const char* value = get_variable(cpu::JIT_LITERAL_OPTIMISATIONS); !string_is_empty(value)) {
        _literalOptimizations = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_LITERAL_OPTIMISATIONS, values::ENABLED);
        _literalOptimizations = true;
    }

#ifdef HAVE_JIT_FASTMEM
    if (const char* value = get_variable(cpu::JIT_FAST_MEMORY); !string_is_empty(value)) {
        _fastMemory = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", cpu::JIT_FAST_MEMORY, values::ENABLED);
        _fastMemory = true;
    }
#endif
#endif
}

static void melonds::config::parse_system_options() noexcept {
    ZoneScopedN("melonds::config::parse_system_options");
    using namespace melonds::config::system;
    using retro::get_variable;

    // All of these options take effect when a game starts, so there's no need to update them mid-game

    if (optional<melonds::ConsoleType> type = ParseConsoleType(get_variable(CONSOLE_MODE))) {
        _consoleType = *type;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CONSOLE_MODE, values::DS);
        _consoleType = ConsoleType::DS;
    }

    if (optional<BootMode> value = ParseBootMode(get_variable(BOOT_MODE))) {
        _bootMode = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", BOOT_MODE, values::NATIVE);
        _bootMode = BootMode::Direct;
    }

    if (optional<SysfileMode> value = ParseSysfileMode(get_variable(SYSFILE_MODE))) {
        _sysfileMode = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SYSFILE_MODE, values::BUILT_IN);
        _sysfileMode = SysfileMode::BuiltIn;
    }

    if (optional<unsigned> value = ParseIntegerInList(get_variable(DS_POWER_OK), DS_POWER_OK_THRESHOLDS)) {
        _dsPowerOkayThreshold = *value;
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to 20%", DS_POWER_OK);
        _dsPowerOkayThreshold = 20;
    }

    if (optional<unsigned> value = ParseIntegerInList(get_variable(BATTERY_UPDATE_INTERVAL), POWER_UPDATE_INTERVALS)) {
        _powerUpdateInterval = *value;
    }
    else {
        retro::warn("Failed to get value for {}; defaulting to 15 seconds", BATTERY_UPDATE_INTERVAL);
        _powerUpdateInterval = 15;
    }
}

static void melonds::config::parse_firmware_options() noexcept {
    ZoneScopedN("melonds::config::parse_firmware_options");
    using namespace melonds::config::firmware;
    using retro::get_variable;

    if (optional<melonds::FirmwareLanguage> value = ParseLanguage(get_variable(firmware::LANGUAGE))) {
        _language = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::LANGUAGE);
        _language = FirmwareLanguage::Default;
    }

    if (const char* value = get_variable(firmware::FAVORITE_COLOR); string_is_equal(value, values::DEFAULT)) {
        _favoriteColor = Color::Default;
    } else if (!string_is_empty(value)) {
        _favoriteColor = static_cast<Color>(std::clamp(std::stoi(value), 0, 15));
        // TODO: Warn if invalid
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::FAVORITE_COLOR);
        _favoriteColor = Color::Default;
    }

    if (optional<UsernameMode> username = ParseUsernameMode(get_variable(firmware::USERNAME))) {
        _usernameMode = *username;
    } else {
        retro::warn("Failed to get the user's name; defaulting to \"melonDS DS\"");
        _usernameMode = UsernameMode::MelonDSDS;
    }

    if (optional<AlarmMode> alarmMode = ParseAlarmMode(get_variable(firmware::ENABLE_ALARM))) {
        _alarmMode = *alarmMode;
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ENABLE_ALARM);
        _alarmHour = nullopt;
    }

    if (const char* alarmHourText = get_variable(firmware::ALARM_HOUR); string_is_equal(alarmHourText, values::DEFAULT)) {
        _alarmHour = nullopt;
    } else if (optional<unsigned> alarmHour = ParseIntegerInRange(alarmHourText, 0u, 23u)) {
        _alarmHour = alarmHour;
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ALARM_HOUR);
        _alarmHour = nullopt;
    }

    if (const char* alarmMinuteText = get_variable(firmware::ALARM_MINUTE); string_is_equal(alarmMinuteText, values::DEFAULT)) {
        _alarmMinute = nullopt;
    } else if (optional<unsigned> alarmMinute = ParseIntegerInRange(alarmMinuteText, 0u, 59u)) {
        _alarmMinute = alarmMinute;
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::ALARM_MINUTE);
        _alarmMinute = nullopt;
    }

    if (const char* birthMonthText = get_variable(firmware::BIRTH_MONTH); string_is_equal(birthMonthText, values::DEFAULT)) {
        _birthdayMonth = 0;
    } else if (optional<unsigned> birthMonth = ParseIntegerInRange(birthMonthText, 1u, 12u)) {
        _birthdayMonth = *birthMonth;
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::BIRTH_MONTH);
        _birthdayMonth = 0;
    }

    if (const char* birthDayText = get_variable(firmware::BIRTH_DAY); string_is_equal(birthDayText, values::DEFAULT)) {
        _birthdayDay = 0;
    } else if (optional<unsigned> birthDay = ParseIntegerInRange(birthDayText, 1u, 31u)) {
        _birthdayDay = *birthDay;
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::BIRTH_DAY);
        _birthdayDay = 0;
    }

    if (const char* wfcDnsText = get_variable(firmware::WFC_DNS); string_is_equal(wfcDnsText, values::DEFAULT)) {
        _dnsServer = nullopt;
    } else if (optional<IpAddress> wfcDns = ParseIpAddress(wfcDnsText)) {
        _dnsServer = *wfcDns;
    } else {
        retro::warn("Failed to get value for {}; defaulting to existing firmware value", firmware::WFC_DNS);
        _dnsServer = nullopt;
    }

    // TODO: Make MAC address configurable with a file at runtime
}

static void melonds::config::parse_audio_options() noexcept {
    ZoneScopedN("melonds::config::parse_audio_options");
    using namespace melonds::config::audio;
    using retro::get_variable;

    if (const char* value = get_variable(MIC_INPUT_BUTTON); !string_is_empty(value)) {
        if (string_is_equal(value, values::HOLD)) {
            _micButtonMode = MicButtonMode::Hold;
        } else if (string_is_equal(value, values::TOGGLE)) {
            _micButtonMode = MicButtonMode::Toggle;
        } else if (string_is_equal(value, values::ALWAYS)) {
            _micButtonMode = MicButtonMode::Always;
        } else {
            _micButtonMode = MicButtonMode::Hold;
        }
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", MIC_INPUT_BUTTON, values::HOLD);
        _micButtonMode = MicButtonMode::Hold;
    }

    if (optional<melonds::MicInputMode> value = ParseMicInputMode(get_variable(MIC_INPUT))) {
        _micInputMode = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", MIC_INPUT, values::SILENCE);
        _micInputMode = MicInputMode::None;
    }

    if (const char* value = get_variable(AUDIO_BITDEPTH); !string_is_empty(value)) {
        if (string_is_equal(value, values::_10BIT))
            _bitDepth = BitDepth::_10Bit;
        else if (string_is_equal(value, values::_16BIT))
            _bitDepth = BitDepth::_16Bit;
        else
            _bitDepth = BitDepth::Auto;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", AUDIO_BITDEPTH, values::AUTO);
        _bitDepth = BitDepth::Auto;
    }

    if (const char* value = get_variable(AUDIO_INTERPOLATION); !string_is_empty(value)) {
        if (string_is_equal(value, values::CUBIC))
            _interpolation = AudioInterpolation::Cubic;
        else if (string_is_equal(value, values::COSINE))
            _interpolation = AudioInterpolation::Cosine;
        else if (string_is_equal(value, values::LINEAR))
            _interpolation = AudioInterpolation::Linear;
        else
            _interpolation = AudioInterpolation::None;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", AUDIO_INTERPOLATION, values::DISABLED);
        _interpolation = AudioInterpolation::None;
    }
}

#ifdef HAVE_NETWORKING
static void melonds::config::parse_network_options() noexcept {
    ZoneScopedN("melonds::config::parse_network_options");
    using retro::get_variable;

    if (optional<NetworkMode> networkMode = ParseNetworkMode(get_variable(network::NETWORK_MODE))) {
#ifdef HAVE_NETWORKING_DIRECT_MODE
        if (*networkMode == NetworkMode::Direct && !net::_interfacesInitialized) {
            // If we're using direct mode, but we couldn't enumerate the interfaces when we tried...
            retro::warn("Failed to enumerate network devices, falling back to Indirect mode.");
            networkMode = NetworkMode::Indirect;
        }
#endif
        net::_networkMode = *networkMode;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", network::NETWORK_MODE, values::INDIRECT);
        net::_networkMode = NetworkMode::Indirect;
    }

#ifdef HAVE_NETWORKING_DIRECT_MODE
    if (const char* value = get_variable(network::DIRECT_NETWORK_INTERFACE); !string_is_empty(value)) {
        strlcpy(net::_networkInterface, value, sizeof(net::_networkInterface));
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", network::DIRECT_NETWORK_INTERFACE, values::AUTO);
        strlcpy(net::_networkInterface, values::AUTO, sizeof(net::_networkInterface));
    }
#endif
}
#endif

static bool melonds::config::parse_video_options(bool initializing) noexcept {
    ZoneScopedN("melonds::config::parse_video_options");
    using namespace melonds::config::video;
    using retro::get_variable;

    bool needsOpenGlRefresh = false;

#if defined(HAVE_THREADS) && defined(HAVE_THREADED_RENDERER)
    if (const char* value = get_variable(THREADED_RENDERER); !string_is_empty(value)) {
        // Only relevant for software-rendered 3D, so no OpenGL state reset needed
        _renderSettings.Soft_Threaded = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", THREADED_RENDERER, values::ENABLED);
        _renderSettings.Soft_Threaded = true;
    }
#else
    _renderSettings.Soft_Threaded = false;
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (initializing) {
        // Can't change the renderer mid-game
        if (optional<Renderer> renderer = ParseRenderer(get_variable(RENDER_MODE))) {
            _configuredRenderer = *renderer;
        } else {
            retro::warn("Failed to get value for {}; defaulting to {}", RENDER_MODE, values::SOFTWARE);
            _configuredRenderer = Renderer::Software;
        }
    }

    if (const char* value = get_variable(OPENGL_RESOLUTION); !string_is_empty(value)) {
        int newScaleFactor = std::clamp(atoi(value), 1, 8);

        if (_renderSettings.GL_ScaleFactor != newScaleFactor)
            needsOpenGlRefresh = true;

        _renderSettings.GL_ScaleFactor = newScaleFactor;
    } else {
        retro::warn("Failed to get value for {}; defaulting to 1", OPENGL_RESOLUTION);
        _renderSettings.GL_ScaleFactor = 1;
    }

    if (const char* value = get_variable(OPENGL_BETTER_POLYGONS); !string_is_empty(value)) {
        bool enabled = string_is_equal(value, values::ENABLED);

        if (_renderSettings.GL_BetterPolygons != enabled)
            needsOpenGlRefresh = true;

        _renderSettings.GL_BetterPolygons = enabled;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", OPENGL_BETTER_POLYGONS, values::DISABLED);
        _renderSettings.GL_BetterPolygons = false;
    }

    if (const char* value = get_variable(OPENGL_FILTERING); !string_is_empty(value)) {
        if (string_is_equal(value, values::LINEAR))
            _screenFilter = ScreenFilter::Linear;
        else
            _screenFilter = ScreenFilter::Nearest;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", OPENGL_FILTERING, values::NEAREST);
        _screenFilter = ScreenFilter::Nearest;
    }
#endif

    return needsOpenGlRefresh;
}

static void melonds::config::parse_screen_options() noexcept {
    ZoneScopedN("melonds::config::parse_screen_options");
    using namespace melonds::config::screen;
    using retro::get_variable;

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(SCREEN_GAP), SCREEN_GAP_LENGTHS)) {
        _screenGap = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SCREEN_GAP, 0);
        _screenGap = 0;
    }

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(CURSOR_TIMEOUT), CURSOR_TIMEOUTS)) {
        _cursorTimeout = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", CURSOR_TIMEOUT, 3);
        _cursorTimeout = 3;
    }

    if (optional<melonds::TouchMode> value = ParseTouchMode(get_variable(TOUCH_MODE))) {
        _touchMode = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", TOUCH_MODE, values::AUTO);
        _touchMode = TouchMode::Auto;
    }

    if (optional<melonds::CursorMode> value = ParseCursorMode(get_variable(SHOW_CURSOR))) {
        _cursorMode = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", SHOW_CURSOR, values::ALWAYS);
        _cursorMode = CursorMode::Always;
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(HYBRID_RATIO), 2u, 3u)) {
        _hybridRatio = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", HYBRID_RATIO, 2);
        _hybridRatio = 2;
    }

    if (optional<HybridSideScreenDisplay> value = ParseHybridSideScreenDisplay(get_variable(HYBRID_SMALL_SCREEN))) {
        _smallScreenLayout = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", HYBRID_SMALL_SCREEN, values::BOTH);
        _smallScreenLayout = HybridSideScreenDisplay::Both;
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(NUMBER_OF_SCREEN_LAYOUTS), 1u, MAX_SCREEN_LAYOUTS)) {
        _numberOfScreenLayouts = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", NUMBER_OF_SCREEN_LAYOUTS, 2);
        _numberOfScreenLayouts = 2;
    }

    for (unsigned i = 0; i < MAX_SCREEN_LAYOUTS; i++) {
        if (optional<melonds::ScreenLayout> value = ParseScreenLayout(get_variable(SCREEN_LAYOUTS[i]))) {
            _screenLayouts[i] = *value;
        } else {
            retro::warn("Failed to get value for {}; defaulting to {}", SCREEN_LAYOUTS[i], values::TOP_BOTTOM);
            _screenLayouts[i] = ScreenLayout::TopBottom;
        }
    }
}

/**
 * Reads the frontend's saved homebrew save data options and applies them to the emulator.
 */
static void melonds::config::parse_homebrew_save_options(const NDSHeader* header) {
    ZoneScopedN("melonds::config::parse_homebrew_save_options");
    using namespace melonds::config::save;
    using retro::get_variable;

    if (!header || !header->IsHomebrew()) {
        // If no game is loaded, or if a non-homebrew game is loaded...
        _dldiEnable = false;
        retro::debug("Not parsing homebrew save options, as no homebrew game is loaded");
        return;
    }

    optional<string> save_directory = retro::get_save_directory();
    if (!save_directory) {
        _dldiEnable = false;
        retro::error("Failed to get save directory; disabling homebrew SD card");
        return;
    }

    if (const char* value = get_variable(storage::HOMEBREW_READ_ONLY); !string_is_empty(value)) {
        _dldiReadOnly = string_is_equal(value, values::ENABLED);
    } else {
        _dldiReadOnly = false;
        retro::warn("Failed to get value for {}; defaulting to {}", storage::HOMEBREW_READ_ONLY, values::DISABLED);
    }

    if (const char* value = get_variable(storage::HOMEBREW_SYNC_TO_HOST); !string_is_empty(value)) {
        _dldiFolderSync = string_is_equal(value, values::ENABLED);
    } else {
        _dldiFolderSync = true;
        retro::warn("Failed to get value for {}; defaulting to {}", storage::HOMEBREW_SYNC_TO_HOST, values::ENABLED);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::HOMEBREW_SAVE_MODE))) {
        _dldiEnable = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::HOMEBREW_SAVE_MODE, values::ENABLED);
        _dldiEnable = true;
    }
}

/**
 * Reads the frontend's saved DSi save data options and applies them to the emulator.
 */
static void melonds::config::parse_dsi_storage_options() noexcept {
    ZoneScopedN("melonds::config::parse_dsi_storage_options");
    using namespace melonds::config::save;
    using retro::get_variable;

    if (const char* value = get_variable(storage::DSI_SD_READ_ONLY); !string_is_empty(value)) {
        _dsiSdReadOnly = string_is_equal(value, values::ENABLED);
    } else {
        _dsiSdReadOnly = false;
        retro::warn("Failed to get value for {}; defaulting to {}", storage::DSI_SD_READ_ONLY, values::DISABLED);
    }

    if (const char* value = get_variable(storage::DSI_SD_SYNC_TO_HOST); !string_is_empty(value)) {
        _dsiSdFolderSync = string_is_equal(value, values::ENABLED);
    } else {
        _dsiSdFolderSync = true;
        retro::warn("Failed to get value for {}; defaulting to {}", storage::DSI_SD_SYNC_TO_HOST, values::ENABLED);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::DSI_SD_SAVE_MODE))) {
        _dsiSdEnable = *value;
    } else {
        retro::warn("Failed to get value for {}; defaulting to {}", storage::DSI_SD_SAVE_MODE, values::ENABLED);
        _dsiSdEnable = true;
    }

    // If these firmware/BIOS files don't exist, an exception will be thrown later
    if (const char* value = get_variable(storage::DSI_NAND_PATH); !string_is_empty(value)) {
        strncpy(system::_dsiNandPath, value, sizeof(system::_dsiNandPath));
    } else {
        strncpy(system::_dsiNandPath, values::NOT_FOUND, sizeof(system::_dsiNandPath));
        retro::warn("Failed to get value for {}", storage::DSI_NAND_PATH);
    }

    if (const char* value = get_variable(system::FIRMWARE_PATH); !string_is_empty(value)) {
        strncpy(system::_firmwarePath, value, sizeof(system::_firmwarePath));
    } else {
        strncpy(system::_firmwarePath, values::NOT_FOUND, sizeof(system::_firmwarePath));
        retro::warn("Failed to get value for {}; defaulting to built-in firmware", system::FIRMWARE_PATH);
    }

    if (const char* value = get_variable(system::FIRMWARE_DSI_PATH); !string_is_empty(value)) {
        strncpy(system::_dsiFirmwarePath, value, sizeof(system::_dsiFirmwarePath));
    } else {
        strncpy(system::_dsiFirmwarePath, values::NOT_FOUND, sizeof(system::_dsiFirmwarePath));
        retro::warn("Failed to get value for {}; defaulting to built-in firmware", system::FIRMWARE_DSI_PATH);
    }
}

static unique_ptr<u8[]> LoadBios(const string_view& name, melonds::BiosType type, size_t expectedLength) noexcept {
    ZoneScopedN("melonds::config::LoadBios");

    auto LoadBiosImpl = [&](const string& path) -> unique_ptr<u8[]> {
        RFILE* file = filestream_open(path.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

        if (!file) {
            retro::error("Failed to open {} file \"{}\" for reading", type, path);
            return nullptr;
        }

        int64_t size = 0;
        if (size = filestream_get_size(file); size != (int64_t)expectedLength) {
            retro::error("Expected {} file \"{}\" to be exactly {} bytes long, got {} bytes", type, path, expectedLength, size);
            filestream_close(file);
            return nullptr;
        }

        unique_ptr<u8[]> result = make_unique<u8[]>(size);
        if (int64_t bytesRead = filestream_read(file, result.get(), expectedLength); bytesRead != (int64_t)expectedLength) {
            retro::error("Failed to read {} bytes from {} file \"{}\"; got {} bytes", expectedLength, type, path, bytesRead);
            filestream_close(file);
            return nullptr;
        }

        filestream_close(file);
        retro::info("Successfully loaded {}-byte {} file \"{}\"", expectedLength, type, path);

        return result;
    };

    // Prefer looking in "system/melonDS DS/${name}", but fall back to "system/${name}" if that fails

    unique_ptr<u8[]> result;
    if (optional<string> path = retro::get_system_subdir_path(name); path && (result = LoadBiosImpl(*path))) {
        // Get the path where we're expecting a BIOS file. If it's there and we loaded it...
        return result;
    }

    if (optional<string> path = retro::get_system_path(name); path && (result = LoadBiosImpl(*path))) {
        // Get the path where we're expecting a BIOS file. If it's there and we loaded it...
        return result;
    }

    retro::error("Failed to load {} file \"{}\"", type, name);

    return nullptr;
}

/// Loads firmware, does not patch it.
static unique_ptr<Firmware> LoadFirmware(const string& firmwarePath) noexcept {
    ZoneScopedN("melonds::config::LoadFirmware");
    using namespace melonds;
    using namespace melonds::config::firmware;
    using namespace Platform;

    // Try to open the configured firmware dump.
    RFILE* file = filestream_open(firmwarePath.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!file) {
        // If that fails...
        retro::error("Failed to open firmware file \"{}\" for reading", firmwarePath);
        return nullptr;
    }

    int64_t fileSize = filestream_get_size(file);
    unique_ptr<u8[]> buffer = make_unique<u8[]>(fileSize);
    int64_t bytesRead = filestream_read(file, buffer.get(), fileSize);
    filestream_close(file);

    if (bytesRead != fileSize) {
        // If we couldn't read the firmware file...
        retro::error("Failed to read firmware file \"{}\"; got {} bytes, expected {} bytes", firmwarePath, bytesRead, fileSize);
        return nullptr;
    }

    // Try to load the firmware dump into the object.
    unique_ptr<Firmware> firmware = make_unique<Firmware>(buffer.get(), fileSize);

    if (!firmware->Buffer()) {
        // If we failed to load the firmware...
        retro::error("Failed to read opened firmware file \"{}\"", firmwarePath);
        return nullptr;
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

/// Loads the DSi NAND, does not patch it
static DSi_NAND::NANDImage LoadNANDImage(const string& nandPath, const u8* es_keyY) {
    using namespace melonds;
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

static void CustomizeFirmware(Firmware& firmware) {
    ZoneScopedN("melonds::config::CustomizeFirmware");
    using namespace melonds;
    using namespace melonds::config::firmware;
    using namespace Platform;

    // We don't need to save the whole firmware, just the part that may actually change.
    optional<string> wfcsettingspath = retro::get_system_subdir_path(config::system::GeneratedFirmwareSettingsPath());
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

            if (config::osd::ShowBiosWarnings()) {
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
        string username = melonds::config::GetUsername(_usernameMode);
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string convertedUsername = converter.from_bytes(username);
        size_t usernameLength = std::min(convertedUsername.length(), (size_t) melonds::config::DS_NAME_LIMIT);
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
        MacAddress mac = *_macAddress;
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
static void InitNdsSystemConfig(NDS& nds, const NDSHeader* header, melonds::BootMode bootMode, melonds::SysfileMode sysfileMode) {
    ZoneScopedN("melonds::config::InitNdsSystemConfig");
    using namespace melonds::config::system;
    using namespace melonds;
    retro_assert(!(header && header->IsDSiWare()));

    // The rules are somewhat complicated.
    // - Bootable firmware is required if booting without content.
    // - All system files must be native or all must be built-in. (No mixing.)
    // - If BIOS files are built-in, then Direct Boot mode must be used
    unique_ptr<Firmware> firmware;
    if (sysfileMode == SysfileMode::Native) {
        optional<string> firmwarePath = retro::get_system_path(FirmwarePath());
        if (!firmwarePath) {
            retro::error("Failed to get system directory");
        }

        firmware = firmwarePath ? LoadFirmware(*firmwarePath) : nullptr;
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
        firmware = make_unique<Firmware>(static_cast<int>(melonds::ConsoleType::DS));
    }

    if (sysfileMode == SysfileMode::BuiltIn) {
        retro::debug("Not loading native ARM BIOS files");
    }

    // Try to load the ARM7 and ARM9 BIOS files (but don't bother with the ARM9 BIOS if the ARM7 BIOS failed)
    unique_ptr<u8[]> bios7 = (sysfileMode == SysfileMode::Native) ? LoadBios(Bios7Path(), BiosType::Arm7, sizeof(NDS::ARM7BIOS)) : nullptr;
    unique_ptr<u8[]> bios9 = bios7 ? LoadBios(Bios9Path(), BiosType::Arm9, sizeof(NDS::ARM9BIOS)) : nullptr;

    if (sysfileMode == SysfileMode::Native && !(bios7 && bios9)) {
        // If we're trying to load native BIOS files, but at least one of them failed...
        retro::warn("Falling back to FreeBIOS");
    }

    // Now that we've loaded the system files, let's see if we can use them

    if (bootMode == melonds::BootMode::Native && (!bios7 || !bios9 || !firmware->IsBootable())) {
        // If we want to try a native boot, but the BIOS files aren't all native or the firmware isn't bootable...
        retro::warn("Native boot requires bootable firmware and native BIOS files; forcing Direct Boot mode");

        _bootMode = melonds::BootMode::Direct;
    }

    if (!header && (!firmware || !firmware->IsBootable() || !bios7 || !bios9)) {
        // If we're trying to boot into the NDS menu, but we don't have all the required files...
        throw nds_sysfiles_incomplete_exception();
    }

    if (bios7 && bios9) {
        memcpy(nds.ARM9BIOS, bios9.get(), sizeof(NDS::ARM9BIOS));
        memcpy(nds.ARM7BIOS, bios7.get(), sizeof(NDS::ARM7BIOS));
        retro::debug("Installed loaded native ARM7 and ARM9 NDS BIOS images");
    } else {
        memcpy(nds.ARM9BIOS, bios_arm9_bin, sizeof(NDS::ARM9BIOS));
        memcpy(nds.ARM7BIOS, bios_arm7_bin, sizeof(NDS::ARM7BIOS));
        retro::debug("Installed built-in ARM7 and ARM9 NDS BIOS images");
    }

    CustomizeFirmware(*firmware);
    retro_assert(nds.SPI.GetFirmwareMem() != nullptr);
    nds.SPI.GetFirmwareMem()->InstallFirmware(std::move(firmware));
}

static void InitDsiSystemConfig(DSi& dsi, const NDSHeader* header) {
    ZoneScopedN("melonds::config::InitDsiSystemConfig");
    using namespace melonds::config::system;
    using namespace melonds::config::firmware;
    using namespace melonds;

    retro_assert(_consoleType == ConsoleType::DSi);

    string_view nandName = DsiNandPath();
    if (nandName == melonds::config::values::NOT_FOUND) {
        throw dsi_no_nand_found_exception();
    }

    // DSi mode requires all native BIOS files
    unique_ptr<u8[]> bios7i = LoadBios(DsiBios7Path(), BiosType::Arm7i, sizeof(DSi::ARM7iBIOS));
    if (!bios7i) {
        throw dsi_missing_bios_exception(BiosType::Arm7i, DsiBios7Path());
    }

    unique_ptr<u8[]> bios9i = LoadBios(DsiBios9Path(), BiosType::Arm9i, sizeof(DSi::ARM9iBIOS));
    if (!bios9i) {
        throw dsi_missing_bios_exception(BiosType::Arm9i, DsiBios9Path());
    }

    unique_ptr<u8[]> bios7 = LoadBios(Bios7Path(), BiosType::Arm7, sizeof(NDS::ARM7BIOS));
    if (!bios7) {
        throw dsi_missing_bios_exception(BiosType::Arm7, Bios7Path());
    }

    unique_ptr<u8[]> bios9 = LoadBios(Bios9Path(), BiosType::Arm9, sizeof(NDS::ARM9BIOS));
    if (!bios9) {
        throw dsi_missing_bios_exception(BiosType::Arm9, Bios9Path());
    }

    string_view firmwareName = DsiFirmwarePath();
    optional<string> firmwarePath = retro::get_system_path(firmwareName);
    retro_assert(firmwarePath.has_value());
    // If we couldn't get the system directory, we wouldn't have gotten this far

    if (firmwareName == melonds::config::values::NOT_FOUND) {
        throw dsi_no_firmware_found_exception();
    }

    unique_ptr<Firmware> firmware = LoadFirmware(*firmwarePath);
    if (!firmware) {
        throw firmware_missing_exception(firmwareName);
    }

    if (firmware && firmware->GetHeader().ConsoleType != Firmware::FirmwareConsoleType::DSi) {
        retro::warn("Expected firmware of type DSi, got {}", firmware->GetHeader().ConsoleType);
        throw wrong_firmware_type_exception(firmwareName, melonds::ConsoleType::DSi, firmware->GetHeader().ConsoleType);
    }
    // DSi firmware isn't bootable, so we don't need to check for that here.

    memcpy(dsi.ARM9iBIOS, bios9i.get(), sizeof(DSi::ARM9iBIOS));
    memcpy(dsi.ARM7iBIOS, bios7i.get(), sizeof(DSi::ARM7iBIOS));
    memcpy(dsi.ARM7BIOS, bios7.get(), sizeof(NDS::ARM7BIOS));
    memcpy(dsi.ARM9BIOS, bios9.get(), sizeof(NDS::ARM9BIOS));
    retro::debug("Installed native ARM7, ARM9, DSi ARM7, and DSi ARM9 BIOS images.");

    // TODO: Customize the NAND first, then use the final value of TWLCFG to patch the firmware
    CustomizeFirmware(*firmware);
    retro_assert(dsi.SPI.GetFirmwareMem() != nullptr);
    dsi.SPI.GetFirmwareMem()->InstallFirmware(std::move(firmware));

    optional<string> nandPath = retro::get_system_path(nandName);
    if (!nandPath) {
        throw environment_exception("Failed to get the system directory, which means the NAND image can't be loaded.");
    }

    DSi_NAND::NANDImage nand = LoadNANDImage(*nandPath, &dsi.ARM7iBIOS[0x8308]);
    if (!nand) {
        throw dsi_nand_missing_exception(nandName);
    }

    DSi_NAND::NANDMount mount(nand);
    if (!mount) {
        throw dsi_nand_corrupted_exception(nandName);
    }

    retro::debug("Opened and mounted the DSi NAND image file at {}", *nandPath);

    DSi_NAND::DSiSerialData dataS {};
    memset(&dataS, 0, sizeof(dataS));
    if (!mount.ReadSerialData(dataS)) {
        throw environment_exception("Failed to read serial data from NAND image");
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
        throw environment_exception("Failed to read user data from NAND image");
    }

    // Right now, I only modify the user data with the firmware overrides defined by core options
    // If there are any problems, I may want to completely synchronize the user data and firmware myself.

    // setting up username
    if (_usernameMode != UsernameMode::Firmware) {
        // If we want to override the existing username...
        string username = melonds::config::GetUsername(_usernameMode);
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string convertedUsername = converter.from_bytes(username);
        size_t usernameLength = std::min(convertedUsername.length(), (size_t) melonds::config::DS_NAME_LIMIT);

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
        throw environment_exception("Failed to write user data to NAND image");
    }

    dsi.NANDImage = make_unique<DSi_NAND::NANDImage>(std::move(nand));
    retro::debug("Installed the DSi NAND image file at {}", *nandPath);
}

static void melonds::config::apply_system_options(melondsds::CoreState& core, const NDSHeader* header) {
    ZoneScopedN("melonds::config::apply_system_options");
    using namespace melonds::config::system;
    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...
        _consoleType = ConsoleType::DSi;
        retro::warn("Forcing DSi mode for DSiWare game");
    }

    if (_consoleType == ConsoleType::DSi) {
        // If we're in DSi mode...
        core.Console = std::make_unique<DSi>();
        InitDsiSystemConfig(*static_cast<DSi*>(core.Console.get()), header);
    } else {
        // If we're in DS mode...
        core.Console = std::make_unique<NDS>();
        InitNdsSystemConfig(*core.Console, header, _bootMode, _sysfileMode);
    }
}

static void melonds::config::apply_audio_options(NDS& nds) noexcept {
    ZoneScopedN("melonds::config::apply_audio_options");
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

    nds.SPU.SetInterpolation(static_cast<int>(config::audio::Interpolation()));
}

static void melonds::config::apply_save_options(const NDSHeader* header) {
    ZoneScopedN("melonds::config::apply_save_options");
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

static void melonds::config::apply_screen_options(ScreenLayoutData& screenLayout, InputState& inputState) noexcept {
    ZoneScopedN("melonds::config::apply_screen_options");
    using namespace config::video;
    using namespace render;

    Renderer renderer = CurrentRenderer() == Renderer::None ?  ConfiguredRenderer() : CurrentRenderer();
    screenLayout.SetScale(renderer == Renderer::OpenGl ? ScaleFactor() : 1);
    screenLayout.SetLayouts(screen::ScreenLayouts(), screen::NumberOfScreenLayouts());
    screenLayout.HybridSmallScreenLayout(screen::SmallScreenLayout());
    screenLayout.ScreenGap(screen::ScreenGap());
    screenLayout.HybridRatio(screen::HybridRatio());

    inputState.SetCursorMode(screen::CursorMode());
    inputState.SetMaxCursorTimeout(screen::CursorTimeout());
    inputState.SetTouchMode(screen::TouchMode());
}

struct FirmwareEntry {
    std::string path;
    Firmware::FirmwareHeader header;
    struct stat stat;
};

static time_t NewestTimestamp(const struct stat& statbuf) noexcept {
    return std::max({statbuf.st_atime, statbuf.st_mtime, statbuf.st_ctime});
}

static bool ConsoleTypeMatches(const Firmware::FirmwareHeader& header, melonds::ConsoleType type) noexcept {
    if (type == melonds::ConsoleType::DS) {
        return header.ConsoleType == Firmware::FirmwareConsoleType::DS || header.ConsoleType == Firmware::FirmwareConsoleType::DSLite;
    }
    else {
        return header.ConsoleType == Firmware::FirmwareConsoleType::DSi;
    }
}

static const char* SelectDefaultFirmware(const vector<FirmwareEntry>& images, melonds::ConsoleType type) noexcept {
    ZoneScopedN("melonds::config::SelectDefaultFirmware");
    using namespace melonds;

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
static void melonds::config::set_core_options() noexcept {
    ZoneScopedN("melonds::config::set_core_options");

    array categories = definitions::OptionCategories<RETRO_LANGUAGE_ENGLISH>;
    array definitions = definitions::CoreOptionDefinitions<RETRO_LANGUAGE_ENGLISH>;

    optional<string> subdir = retro::get_system_subdirectory();

    vector<string> dsiNandPaths;
    vector<FirmwareEntry> firmware;
    const optional<string>& sysdir = retro::get_system_directory();

    if (subdir) {
        ZoneScopedN("melonds::config::set_core_options::find_system_files");
        retro_assert(sysdir.has_value());
        u8 headerBytes[sizeof(Firmware::FirmwareHeader)];
        Firmware::FirmwareHeader& header = *reinterpret_cast<Firmware::FirmwareHeader*>(headerBytes);
        memset(headerBytes, 0, sizeof(headerBytes));
        array paths = {*sysdir, *subdir};
        for (const string& path: paths) {
            ZoneScopedN("melonds::config::set_core_options::find_system_files::paths");
            for (const retro::dirent& d : retro::readdir(path, true)) {
                ZoneScopedN("melonds::config::set_core_options::find_system_files::paths::dirent");
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
        ZoneScopedN("melonds::config::set_core_options::init_dsi_nand_options");
        // If we found at least one DSi NAND image...
        retro_core_option_v2_definition* dsiNandPathOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, melonds::config::storage::DSI_NAND_PATH);
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
        ZoneScopedN("melonds::config::set_core_options::init_firmware_options");
        // If we found at least one firmware image...
        retro_core_option_v2_definition* firmwarePathOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, melonds::config::system::FIRMWARE_PATH);
        });
        retro_core_option_v2_definition* firmwarePathDsiOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, melonds::config::system::FIRMWARE_DSI_PATH);
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
        ZoneScopedN("melonds::config::set_core_options::init_adapter_options");
        // If we successfully initialized PCap and got some adapters...
        retro_core_option_v2_definition* wifiAdapterOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, melonds::config::network::DIRECT_NETWORK_INTERFACE);
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
        net::_interfacesInitialized = true;
    } else {
        retro::warn("Failed to enumerate Wi-fi adapters; falling back to indirect mode");
        net::_interfacesInitialized = false;
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
    }
}

using namespace melonds::config;

int Platform::GetConfigInt(ConfigEntry entry)
{
    switch (entry)
    {
#ifdef JIT_ENABLED
        case JIT_MaxBlockSize: return jit::MaxBlockSize();
#endif

        case DLDI_ImageSize: return save::DldiImageSize();

        case DSiSD_ImageSize: return save::DsiSdImageSize();

        case AudioBitDepth: return static_cast<int>(audio::BitDepth());
        default: return 0;
    }
}

bool Platform::GetConfigBool(ConfigEntry entry)
{
    switch (entry)
    {
#ifdef JIT_ENABLED
        case JIT_Enable: return jit::Enable();
        case JIT_LiteralOptimizations: return jit::LiteralOptimizations();
        case JIT_BranchOptimizations: return jit::BranchOptimizations();
        case JIT_FastMemory: return jit::FastMemory();
#endif

        case ExternalBIOSEnable:
            return
            system::ExternalBiosEnable() &&
            !melondsds::Core.Console->IsLoadedARM7BIOSBuiltIn() &&
            !melondsds::Core.Console->IsLoadedARM9BIOSBuiltIn() &&
            !melondsds::Core.Console->SPI.GetFirmwareMem()->IsLoadedFirmwareBuiltIn();

        case DLDI_Enable: return save::DldiEnable();
        case DLDI_ReadOnly: return save::DldiReadOnly();
        case DLDI_FolderSync: return save::DldiFolderSync();

        case DSiSD_Enable: return save::DsiSdEnable();
        case DSiSD_ReadOnly: return save::DsiSdReadOnly();
        case DSiSD_FolderSync: return save::DsiSdFolderSync();
        default: return false;
    }
}

std::string Platform::GetConfigString(ConfigEntry entry)
{
    using std::string;
    switch (entry)
    {
        case DLDI_ImagePath: return save::DldiImagePath();
        case DLDI_FolderPath: return save::DldiFolderPath();

        case DSiSD_ImagePath: return save::DsiSdImagePath();
        case DSiSD_FolderPath: return save::DsiSdFolderPath();
        case WifiSettingsPath: return "wfcsettings.bin";
        default: return "";
    }
}

