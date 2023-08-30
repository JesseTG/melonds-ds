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
#include <vector>

#include <file/file_path.h>
#include <streams/file_stream.h>
#include <libretro.h>
#include <retro_assert.h>
#include <string/stdstring.h>

#include <DSi.h>
#include <NDS.h>
#include <GPU.h>
#include <SPU.h>
#include <SPI.h>
#include <SPI_Firmware.h>
#include <FreeBIOS.h>

#include "config/constants.hpp"
#include "config/definitions.hpp"
#include "config/definitions/categories.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "input.hpp"
#include "libretro.hpp"
#include "microphone.hpp"
#include "opengl.hpp"
#include "render.hpp"
#include "retro/dirent.hpp"
#include "screenlayout.hpp"
#include "tracy.hpp"

using std::array;
using std::find_if;
using std::from_chars;
using std::from_chars_result;
using std::initializer_list;
using std::make_unique;
using std::nullopt;
using std::optional;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;

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

namespace Config {
    // Needed by melonDS's wi-fi implementation
    std::string LANDevice;
}

namespace melonds::config {
    static void set_core_options(
        const optional<retro_game_info> &nds_info,
        const optional<NDSHeader> &nds_header
    );
    namespace visibility {
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
    static void parse_homebrew_save_options(
        const optional<struct retro_game_info>& nds_info,
        const optional<NDSHeader>& header
    );
    static void parse_dsi_storage_options() noexcept;

    static void apply_system_options(const optional <NDSHeader>& header);

    static void apply_audio_options() noexcept;
    static void apply_save_options(const optional<NDSHeader>& header);
    static void apply_screen_options(ScreenLayoutData& screenLayout, InputState& inputState) noexcept;

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
        static string _message;
        static optional<SPI_Firmware::MacAddress> _macAddress;
        static optional<SPI_Firmware::IpAddress> _dnsServer;
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

        string _dldiFolderPath;
        string DldiFolderPath() noexcept { return _dldiFolderPath; }

        bool _dldiReadOnly;
        bool DldiReadOnly() noexcept { return _dldiReadOnly; }

        string _dldiImagePath;
        string DldiImagePath() noexcept { return _dldiImagePath; }

        unsigned _dldiImageSize;
        unsigned DldiImageSize() noexcept { return _dldiImageSize; }

        bool _dsiSdEnable;
        bool DsiSdEnable() noexcept { return _dsiSdEnable; }

        bool _dsiSdFolderSync;
        bool DsiSdFolderSync() noexcept { return _dsiSdFolderSync; }

        string _dsiSdFolderPath;
        string DsiSdFolderPath() noexcept { return _dsiSdFolderPath; }

        bool _dsiSdReadOnly;
        bool DsiSdReadOnly() noexcept { return _dsiSdReadOnly; }

        string _dsiSdImagePath;
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
    }

    namespace system {
        static melonds::ConsoleType _consoleType;
        melonds::ConsoleType ConsoleType() noexcept { return _consoleType; }

        static bool _directBoot;
        bool DirectBoot() noexcept { return _directBoot; }

        static bool _externalBiosEnable;
        bool ExternalBiosEnable() noexcept { return _externalBiosEnable; }

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

        string_view EffectiveFirmwarePath() noexcept {
            string_view firmware = FirmwarePath(ConsoleType());
            return (firmware == config::values::BUILT_IN) ? GeneratedFirmwareSettingsPath() : firmware;
        }
    }

    namespace video {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES) || defined(HAVE_THREADS)
        static GPU::RenderSettings _renderSettings = {false, 1, false};
        GPU::RenderSettings RenderSettings() noexcept { return _renderSettings; }
#else
        GPU::RenderSettings RenderSettings() noexcept { return {false, 1, false}; }
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


void melonds::InitConfig(const optional<struct retro_game_info>& nds_info, const optional<NDSHeader>& header,
                         ScreenLayoutData& screenLayout, InputState& inputState) {
    ZoneScopedN("melonds::InitConfig");
    config::set_core_options(nds_info, header);
    config::parse_system_options();
    config::parse_osd_options();
    config::parse_jit_options();
    config::parse_homebrew_save_options(nds_info, header);
    config::parse_dsi_storage_options();
    config::parse_firmware_options();
    config::parse_audio_options();
    config::parse_network_options();
    bool openGlNeedsRefresh = config::parse_video_options(true);
    config::parse_screen_options();

    config::apply_system_options(header);
    config::apply_save_options(header);
    config::apply_audio_options();
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

void melonds::UpdateConfig(ScreenLayoutData& screenLayout, InputState& inputState) noexcept {
    config::parse_audio_options();
    bool openGlNeedsRefresh = config::parse_video_options(false);
    config::parse_screen_options();
    config::parse_osd_options();

    config::apply_audio_options();
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
    using namespace melonds::config;
    using namespace melonds::config::visibility;
    using retro::environment;
    using retro::get_variable;
    using retro::set_option_visible;
    bool updated = false;

    // Convention: if an option is not found, show any dependent options
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

    if (ShowSoftwareRenderOptions != oldShowSoftwareRenderOptions) {
        set_option_visible(video::THREADED_RENDERER, ShowSoftwareRenderOptions);

        updated = true;
    }
#else
    set_option_visible(video::RENDER_MODE, false);
#endif

    bool oldShowDsiOptions = ShowDsiOptions;
    bool oldShowDsiSdCardOptions = ShowDsiSdCardOptions;
    optional<ConsoleType> consoleType = ParseConsoleType(get_variable(system::CONSOLE_MODE));

    ShowDsiOptions = !consoleType || *consoleType == ConsoleType::DSi;
    if (ShowDsiOptions != oldShowDsiOptions) {
        set_option_visible(storage::DSI_SD_SAVE_MODE, ShowDsiOptions);
        set_option_visible(storage::DSI_SD_READ_ONLY, ShowDsiOptions);
        set_option_visible(storage::DSI_SD_SYNC_TO_HOST, ShowDsiOptions);

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


    return updated;
}

static melonds::FirmwareLanguage get_firmware_language(const optional<retro_language>& language) {
    using melonds::FirmwareLanguage;

    if (!language)
        return FirmwareLanguage::English;

    switch (*language) {
        case RETRO_LANGUAGE_ENGLISH:
        case RETRO_LANGUAGE_BRITISH_ENGLISH:
            return FirmwareLanguage::English;
        case RETRO_LANGUAGE_JAPANESE:
            return FirmwareLanguage::Japanese;
        case RETRO_LANGUAGE_FRENCH:
            return FirmwareLanguage::French;
        case RETRO_LANGUAGE_GERMAN:
            return FirmwareLanguage::German;
        case RETRO_LANGUAGE_ITALIAN:
            return FirmwareLanguage::Italian;
        case RETRO_LANGUAGE_SPANISH:
            return FirmwareLanguage::Spanish;
        default:
            return FirmwareLanguage::English;
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
        retro::warn("Failed to get value for %s; defaulting to %s", POINTER_COORDINATES, values::DISABLED);
        showPointerCoordinates = false;
    }
#endif

    if (optional<bool> value = ParseBoolean(get_variable(osd::UNSUPPORTED_FEATURES))) {
        showUnsupportedFeatureWarnings = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", UNSUPPORTED_FEATURES, values::ENABLED);
        showUnsupportedFeatureWarnings = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::MIC_STATE))) {
        showMicState = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", MIC_STATE, values::ENABLED);
        showMicState = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::CAMERA_STATE))) {
        showCameraState = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", CAMERA_STATE, values::ENABLED);
        showCameraState = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::BIOS_WARNINGS))) {
        showBiosWarnings = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", BIOS_WARNINGS, values::ENABLED);
        showBiosWarnings = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::CURRENT_LAYOUT))) {
        showCurrentLayout = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", CURRENT_LAYOUT, values::ENABLED);
        showCurrentLayout = true;
    }

    if (optional<bool> value = ParseBoolean(get_variable(osd::LID_STATE))) {
        showLidState = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", LID_STATE, values::DISABLED);
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
        retro::warn("Failed to get value for %s; defaulting to %s", cpu::JIT_ENABLE, values::ENABLED);
        _jitEnable = true;
    }

    if (const char* value = get_variable(cpu::JIT_BLOCK_SIZE); !string_is_empty(value)) {
        _maxBlockSize = std::stoi(value);
    } else {
        retro::warn("Failed to get value for %s; defaulting to 32", cpu::JIT_BLOCK_SIZE);
        _maxBlockSize = 32;
    }

    if (const char* value = get_variable(cpu::JIT_BRANCH_OPTIMISATIONS); !string_is_empty(value)) {
        _branchOptimizations = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", cpu::JIT_BRANCH_OPTIMISATIONS, values::ENABLED);
        _branchOptimizations = true;
    }

    if (const char* value = get_variable(cpu::JIT_LITERAL_OPTIMISATIONS); !string_is_empty(value)) {
        _literalOptimizations = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", cpu::JIT_LITERAL_OPTIMISATIONS, values::ENABLED);
        _literalOptimizations = true;
    }

#ifdef HAVE_JIT_FASTMEM
    if (const char* value = get_variable(cpu::JIT_FAST_MEMORY); !string_is_empty(value)) {
        _fastMemory = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", cpu::JIT_FAST_MEMORY, values::ENABLED);
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

    if (const optional<melonds::ConsoleType> type = ParseConsoleType(get_variable(CONSOLE_MODE))) {
        _consoleType = *type;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", CONSOLE_MODE, values::DS);
        _consoleType = ConsoleType::DS;
    }

    if (const optional<bool> value = ParseBoolean(get_variable(BOOT_DIRECTLY))) {
        _directBoot = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", BOOT_DIRECTLY, values::ENABLED);
        _directBoot = true;
    }

    if (const char* value = get_variable(USE_EXTERNAL_BIOS); !string_is_empty(value)) {
        _externalBiosEnable = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", USE_EXTERNAL_BIOS, values::ENABLED);
        _externalBiosEnable = true;
    }

    if (const optional<unsigned> value = ParseIntegerInList(get_variable(DS_POWER_OK), DS_POWER_OK_THRESHOLDS)) {
        _dsPowerOkayThreshold = *value;
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to 20%%", DS_POWER_OK);
        _dsPowerOkayThreshold = 20;
    }

    if (const optional<unsigned> value = ParseIntegerInList(get_variable(BATTERY_UPDATE_INTERVAL), POWER_UPDATE_INTERVALS)) {
        _powerUpdateInterval = *value;
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to 15 seconds", BATTERY_UPDATE_INTERVAL);
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
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::LANGUAGE);
        _language = FirmwareLanguage::Default;
    }

    if (const char* value = get_variable(firmware::FAVORITE_COLOR); string_is_equal(value, values::DEFAULT)) {
        _favoriteColor = Color::Default;
    } else if (!string_is_empty(value)) {
        _favoriteColor = static_cast<Color>(std::clamp(std::stoi(value), 0, 15));
        // TODO: Warn if invalid
    } else {
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::FAVORITE_COLOR);
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
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::ENABLE_ALARM);
        _alarmHour = nullopt;
    }

    if (const char* alarmHourText = get_variable(firmware::ALARM_HOUR); string_is_equal(alarmHourText, values::DEFAULT)) {
        _alarmHour = nullopt;
    } else if (optional<unsigned> alarmHour = ParseIntegerInRange(alarmHourText, 0u, 23u)) {
        _alarmHour = alarmHour;
    } else {
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::ALARM_HOUR);
        _alarmHour = nullopt;
    }

    if (const char* alarmMinuteText = get_variable(firmware::ALARM_MINUTE); string_is_equal(alarmMinuteText, values::DEFAULT)) {
        _alarmMinute = nullopt;
    } else if (optional<unsigned> alarmMinute = ParseIntegerInRange(alarmMinuteText, 0u, 59u)) {
        _alarmMinute = alarmMinute;
    } else {
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::ALARM_MINUTE);
        _alarmMinute = nullopt;
    }

    if (const char* birthMonthText = get_variable(firmware::BIRTH_MONTH); string_is_equal(birthMonthText, values::DEFAULT)) {
        _birthdayMonth = 0;
    } else if (optional<unsigned> birthMonth = ParseIntegerInRange(birthMonthText, 1u, 12u)) {
        _birthdayMonth = *birthMonth;
    } else {
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::BIRTH_MONTH);
        _birthdayMonth = 0;
    }

    if (const char* birthDayText = get_variable(firmware::BIRTH_DAY); string_is_equal(birthDayText, values::DEFAULT)) {
        _birthdayDay = 0;
    } else if (optional<unsigned> birthDay = ParseIntegerInRange(birthDayText, 1u, 31u)) {
        _birthdayDay = *birthDay;
    } else {
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::BIRTH_DAY);
        _birthdayDay = 0;
    }

    if (const char* wfcDnsText = get_variable(firmware::WFC_DNS); string_is_equal(wfcDnsText, values::DEFAULT)) {
        _dnsServer = nullopt;
    } else if (optional<SPI_Firmware::IpAddress> wfcDns = ParseIpAddress(wfcDnsText)) {
        _dnsServer = *wfcDns;
    } else {
        retro::warn("Failed to get value for %s; defaulting to existing firmware value", firmware::WFC_DNS);
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
        retro::warn("Failed to get value for %s; defaulting to %s", MIC_INPUT_BUTTON, values::HOLD);
        _micButtonMode = MicButtonMode::Hold;
    }

    if (const char* value = get_variable(MIC_INPUT); !string_is_empty(value)) {
        if (string_is_equal(value, values::MICROPHONE))
            _micInputMode = MicInputMode::HostMic;
        else if (string_is_equal(value, values::BLOW))
            _micInputMode = MicInputMode::BlowNoise;
        else if (string_is_equal(value, values::NOISE))
            _micInputMode = MicInputMode::WhiteNoise;
        else
            _micInputMode = MicInputMode::None;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", MIC_INPUT, values::SILENCE);
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
        retro::warn("Failed to get value for %s; defaulting to %s", AUDIO_BITDEPTH, values::AUTO);
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
        retro::warn("Failed to get value for %s; defaulting to %s", AUDIO_INTERPOLATION, values::DISABLED);
        _interpolation = AudioInterpolation::None;
    }
}


static void melonds::config::parse_network_options() noexcept {
    ZoneScopedN("melonds::config::parse_network_options");
    using retro::get_variable;

    if (optional<NetworkMode> networkMode = ParseNetworkMode(get_variable(network::NETWORK_MODE))) {
        net::_networkMode = *networkMode;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", network::NETWORK_MODE, values::INDIRECT);
        net::_networkMode = NetworkMode::Indirect;
    }
}

static bool melonds::config::parse_video_options(bool initializing) noexcept {
    ZoneScopedN("melonds::config::parse_video_options");
    using namespace melonds::config::video;
    using retro::get_variable;

    bool needsOpenGlRefresh = false;

#ifdef HAVE_THREADS
    if (const char* value = get_variable(THREADED_RENDERER); !string_is_empty(value)) {
        // Only relevant for software-rendered 3D, so no OpenGL state reset needed
        _renderSettings.Soft_Threaded = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", THREADED_RENDERER, values::ENABLED);
        _renderSettings.Soft_Threaded = true;
    }
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (initializing) {
        // Can't change the renderer mid-game
        if (optional<Renderer> renderer = ParseRenderer(get_variable(RENDER_MODE))) {
            _configuredRenderer = *renderer;
        } else {
            retro::warn("Failed to get value for %s; defaulting to %s", RENDER_MODE, values::SOFTWARE);
            _configuredRenderer = Renderer::Software;
        }
    }

    if (const char* value = get_variable(OPENGL_RESOLUTION); !string_is_empty(value)) {
        int newScaleFactor = std::clamp(atoi(value), 1, 8);

        if (_renderSettings.GL_ScaleFactor != newScaleFactor)
            needsOpenGlRefresh = true;

        _renderSettings.GL_ScaleFactor = newScaleFactor;
    } else {
        retro::warn("Failed to get value for %s; defaulting to 1", OPENGL_RESOLUTION);
        _renderSettings.GL_ScaleFactor = 1;
    }

    if (const char* value = get_variable(OPENGL_BETTER_POLYGONS); !string_is_empty(value)) {
        bool enabled = string_is_equal(value, values::ENABLED);

        if (_renderSettings.GL_BetterPolygons != enabled)
            needsOpenGlRefresh = true;

        _renderSettings.GL_BetterPolygons = enabled;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", OPENGL_BETTER_POLYGONS, values::DISABLED);
        _renderSettings.GL_BetterPolygons = false;
    }

    if (const char* value = get_variable(OPENGL_FILTERING); !string_is_empty(value)) {
        if (string_is_equal(value, values::LINEAR))
            _screenFilter = ScreenFilter::Linear;
        else
            _screenFilter = ScreenFilter::Nearest;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", OPENGL_FILTERING, values::NEAREST);
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
        retro::warn("Failed to get value for %s; defaulting to %d", SCREEN_GAP, 0);
        _screenGap = 0;
    }

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(CURSOR_TIMEOUT), CURSOR_TIMEOUTS)) {
        _cursorTimeout = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", CURSOR_TIMEOUT, 3);
        _cursorTimeout = 3;
    }

    if (optional<melonds::CursorMode> value = ParseCursorMode(get_variable(SHOW_CURSOR))) {
        _cursorMode = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", SHOW_CURSOR, values::ALWAYS);
        _cursorMode = CursorMode::Always;
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(HYBRID_RATIO), 2u, 3u)) {
        _hybridRatio = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", HYBRID_RATIO, 2);
        _hybridRatio = 2;
    }

    if (optional<HybridSideScreenDisplay> value = ParseHybridSideScreenDisplay(get_variable(HYBRID_SMALL_SCREEN))) {
        _smallScreenLayout = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", HYBRID_SMALL_SCREEN, values::BOTH);
        _smallScreenLayout = HybridSideScreenDisplay::Both;
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(NUMBER_OF_SCREEN_LAYOUTS), 1u, MAX_SCREEN_LAYOUTS)) {
        _numberOfScreenLayouts = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", NUMBER_OF_SCREEN_LAYOUTS, 2);
        _numberOfScreenLayouts = 2;
    }

    for (unsigned i = 0; i < MAX_SCREEN_LAYOUTS; i++) {
        if (optional<melonds::ScreenLayout> value = ParseScreenLayout(get_variable(SCREEN_LAYOUTS[i]))) {
            _screenLayouts[i] = *value;
        } else {
            retro::warn("Failed to get value for %s; defaulting to %s", SCREEN_LAYOUTS[i], values::TOP_BOTTOM);
            _screenLayouts[i] = ScreenLayout::TopBottom;
        }
    }
}

/**
 * Reads the frontend's saved homebrew save data options and applies them to the emulator.
 */
static void melonds::config::parse_homebrew_save_options(
    const optional<struct retro_game_info>& nds_info,
    const optional<NDSHeader>& header
) {
    ZoneScopedN("melonds::config::parse_homebrew_save_options");
    using namespace melonds::config::save;
    using retro::get_variable;

    if (!nds_info || !header || !header->IsHomebrew()) {
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
        retro::warn("Failed to get value for %s; defaulting to %s", storage::HOMEBREW_READ_ONLY, values::DISABLED);
    }

    if (const char* value = get_variable(storage::HOMEBREW_SYNC_TO_HOST); !string_is_empty(value)) {
        _dldiFolderSync = string_is_equal(value, values::ENABLED);
    } else {
        _dldiFolderSync = true;
        retro::warn("Failed to get value for %s; defaulting to %s", storage::HOMEBREW_SYNC_TO_HOST, values::ENABLED);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::HOMEBREW_SAVE_MODE))) {
        _dldiEnable = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", storage::HOMEBREW_SAVE_MODE, values::ENABLED);
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
        retro::warn("Failed to get value for %s; defaulting to %s", storage::DSI_SD_READ_ONLY, values::DISABLED);
    }

    if (const char* value = get_variable(storage::DSI_SD_SYNC_TO_HOST); !string_is_empty(value)) {
        _dsiSdFolderSync = string_is_equal(value, values::ENABLED);
    } else {
        _dsiSdFolderSync = true;
        retro::warn("Failed to get value for %s; defaulting to %s", storage::DSI_SD_SYNC_TO_HOST, values::ENABLED);
    }

    if (optional<bool> value = ParseBoolean(get_variable(storage::DSI_SD_SAVE_MODE))) {
        _dsiSdEnable = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", storage::DSI_SD_SAVE_MODE, values::ENABLED);
        _dsiSdEnable = true;
    }

    // If these firmware/BIOS files don't exist, an exception will be thrown later
    if (const char* value = get_variable(storage::DSI_NAND_PATH); !string_is_empty(value)) {
        strncpy(system::_dsiNandPath, value, sizeof(system::_dsiNandPath));
    } else {
        strncpy(system::_dsiNandPath, values::NOT_FOUND, sizeof(system::_dsiNandPath));
        retro::warn("Failed to get value for %s", storage::DSI_NAND_PATH);
    }

    if (const char* value = get_variable(firmware::FIRMWARE_PATH); !string_is_empty(value)) {
        strncpy(system::_firmwarePath, value, sizeof(system::_firmwarePath));
    } else {
        strncpy(system::_firmwarePath, values::NOT_FOUND, sizeof(system::_firmwarePath));
        retro::warn("Failed to get value for %s; defaulting to built-in firmware", firmware::FIRMWARE_PATH);
    }

    if (const char* value = get_variable(firmware::FIRMWARE_DSI_PATH); !string_is_empty(value)) {
        strncpy(system::_dsiFirmwarePath, value, sizeof(system::_dsiFirmwarePath));
    } else {
        strncpy(system::_dsiFirmwarePath, values::NOT_FOUND, sizeof(system::_dsiFirmwarePath));
        retro::warn("Failed to get value for %s; defaulting to built-in firmware", firmware::FIRMWARE_DSI_PATH);
    }
}

static bool LoadBios(const string_view& name, const string& type, u8* buffer, size_t bufferLength) noexcept {
    ZoneScopedN("melonds::config::LoadBios");

    auto LoadBiosImpl = [&](const string& path) {
        RFILE* file = filestream_open(path.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

        if (!file) {
            retro::error("Failed to open %s file \"%s\" for reading", type.c_str(), path.c_str());
            return false;
        }

        if (int64_t size = filestream_get_size(file); size != (int64_t)bufferLength) {
            retro::error("Expected %s file \"%s\" to be exactly %llu bytes long, got %lld bytes", type.c_str(), path.c_str(), bufferLength, size);
            filestream_close(file);
            return false;
        }

        if (int64_t bytesRead = filestream_read(file, buffer, bufferLength); bytesRead != (int64_t)bufferLength) {
            retro::error("Failed to read %lluB bytes from %s file \"%s\"; got %lld bytes", bufferLength, type.c_str(), path.c_str(), bytesRead);
            filestream_close(file);
            return false;
        }

        filestream_close(file);
        retro::info("Successfully installed %llu-byte %s file \"%s\"", bufferLength, type.c_str(), path.c_str());

        return true;
    };

    if (optional<string> path = retro::get_system_subdir_path(name); path && LoadBiosImpl(*path)) {
        return true;
    }

    if (optional<string> path = retro::get_system_path(name); path || !LoadBiosImpl(*path)) {
        return true;
    }

    retro::error("Failed to load %s file \"%s\"", type.c_str(), name.data());

    return false;
}

static unique_ptr<SPI_Firmware::Firmware> InitFirmware(melonds::ConsoleType type) {
    ZoneScopedN("melonds::config::InitFirmware");
    using namespace melonds;
    using namespace melonds::config::firmware;
    using namespace SPI_Firmware;
    using namespace Platform;

    unique_ptr<SPI_Firmware::Firmware> firmware;
    string_view firmwareName = config::system::FirmwarePath(type);

    if (firmwareName != config::values::BUILT_IN) {
        if (optional<string> firmwarePath = retro::get_system_path(firmwareName)) {
            if (FileHandle* file = OpenLocalFile(*firmwarePath, FileMode::Read)) {
                // Try to load the configured firmware dump. If that fails...
                if (firmware = make_unique<SPI_Firmware::Firmware>(file); !firmware->Buffer()) {
                    retro::warn("Failed to read firmware from file\n");
                    firmware = nullptr;
                } else {
                    retro::info("Loaded firmware (Identifier: %4s)", firmware->Header().Identifier.data());
                }

                CloseFile(file);
            }
        }

        if (!(firmware && firmware->Buffer())) {
            // If the configured firmware dump failed to load...
            char warning[256];
            snprintf(warning, sizeof(warning), "Failed to load %s firmware at \"%s\", falling back to built-in default", ConsoleTypeName(type).data(), firmwareName.data());
            retro::set_warn_message(warning);
        }
    }

    bool generated = false;
    if (!firmware) {
        // If we haven't yet loaded firmware (either because the load failed or we want to use the default...)

        generated = true;
        firmware = make_unique<SPI_Firmware::Firmware>(static_cast<int>(type));
        retro_assert(firmware != nullptr);

        // We don't need to save the whole firmware, just the part that may actually change.
        optional<string> wfcsettingspath = retro::get_system_subdir_path(config::system::GeneratedFirmwareSettingsPath());
        if (!wfcsettingspath) {
            throw std::runtime_error("Failed to get path to generated firmware settings");
        }

        // If using generated firmware, we keep the wi-fi settings on the host disk separately.
        // Wi-fi access point data includes Nintendo WFC settings,
        // and if we didn't keep them then the player would have to reset them in each session.
        if (RFILE* file = filestream_open(wfcsettingspath->c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE)) {
            // If we have Wi-fi settings to load...
            constexpr unsigned TOTAL_WFC_SETTINGS_SIZE = 3 * (sizeof(WifiAccessPoint) + sizeof(ExtendedWifiAccessPoint));

            // The access point and extended access point segments might
            // be in different locations depending on the firmware revision,
            // but our generated firmware always keeps them next to each other.
            // (Extended access points first, then regular ones.)
            u8* userdata = firmware->ExtendedAccessPointPosition();

            if (filestream_read(file, userdata, TOTAL_WFC_SETTINGS_SIZE) != TOTAL_WFC_SETTINGS_SIZE) {
                // If we couldn't read the Wi-fi settings from this file...
                retro::warn("Failed to read Wi-fi settings from \"%s\"; using defaults instead\n", wfcsettingspath->c_str());

                firmware->AccessPoints() = {
                    WifiAccessPoint(static_cast<int>(type)),
                    WifiAccessPoint(),
                    WifiAccessPoint(),
                };

                firmware->ExtendedAccessPoints() = {
                    ExtendedWifiAccessPoint(),
                    ExtendedWifiAccessPoint(),
                    ExtendedWifiAccessPoint(),
                };
            }

            filestream_close(file);
        }

        // If we don't have Wi-fi settings to load,
        // then the defaults will have already been populated by the constructor.
    }

    if (!generated && config::system::ConsoleType() == ConsoleType::DS) {
        // If we're using externally-loaded DS (not DSi) firmware...

        u8 chk1[0x180], chk2[0x180];

        // I don't really know how this works, it's just adapted from upstream
        memcpy(chk1, firmware->Buffer(), sizeof(chk1));
        memcpy(chk2, firmware->Buffer() + firmware->Length() - 0x380, sizeof(chk2));

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

    UserData& currentData = firmware->EffectiveUserData();

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
            currentData.Settings &= ~Language::Reserved; // clear the existing language bits
            currentData.Settings |= static_cast<SPI_Firmware::Language>(get_firmware_language(retro::get_language()));
            break;
        case FirmwareLanguage::Default:
            // do nothing, leave the existing language in place
            break;
        default:
            currentData.Settings &= ~Language::Reserved;
            currentData.Settings |= static_cast<SPI_Firmware::Language>(_language);
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
        firmware->AccessPoints()[0].PrimaryDns = *_dnsServer;
        firmware->AccessPoints()[0].SecondaryDns = *_dnsServer;
    }

    if (_macAddress) {
        MacAddress mac = *_macAddress;
        mac[0] &= 0xFC; // ensure the MAC isn't a broadcast MAC
        firmware->Header().MacAddress = mac;
    }

    firmware->UpdateChecksums();

    return firmware;
}

static void melonds::config::apply_system_options(const optional<NDSHeader>& header) {
    ZoneScopedN("melonds::config::apply_system_options");
    using namespace melonds::config::system;
    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...
        _consoleType = ConsoleType::DSi;
        retro::warn("Forcing DSi mode for DSiWare game");
    }

    if (_consoleType == ConsoleType::DSi) {
        // If we're in DSi mode...
        if (!LoadBios(DsiBios7Path(), "DSi ARM7", DSi::ARM7iBIOS, sizeof(DSi::ARM7iBIOS))) {
            // Try to load the DSi ARM7 BIOS. If that fails...
            throw bios_exception("Failed to load DSi ARM7 BIOS");
        }

        if (!LoadBios(DsiBios9Path(), "DSi ARM9", DSi::ARM9iBIOS, sizeof(DSi::ARM9iBIOS))) {
            // Try to load the DSi ARM9 BIOS. If that fails...
            throw bios_exception("Failed to load DSi ARM9 BIOS");
        }
    }

    if (!(_externalBiosEnable && LoadBios(Bios7Path(), "ARM7", NDS::ARM7BIOS, sizeof(NDS::ARM7BIOS)))) {
        // If we're not using external BIOS files, or if loading the ARM7 file failed...
        retro::info("Using internal ARM7 BIOS.");
        memcpy(NDS::ARM7BIOS, bios_arm7_bin, sizeof(bios_arm7_bin));
    }

    if (!(_externalBiosEnable && LoadBios(Bios9Path(), "ARM9", NDS::ARM9BIOS, sizeof(NDS::ARM9BIOS)))) {
        // If we're not using external BIOS files, or if loading the ARM9 file failed...
        retro::info("Using internal ARM9 BIOS.");
        memcpy(NDS::ARM9BIOS, bios_arm9_bin, sizeof(bios_arm9_bin));
    }

    unique_ptr<SPI_Firmware::Firmware> firmware = InitFirmware(_consoleType);

    if (!firmware) {
        throw emulator_exception("Failed to load firmware");
    }

    if (firmware->Header().Identifier == SPI_Firmware::GENERATED_FIRMWARE_IDENTIFIER) {
        if (!header) {
            // If we're using generated firmware and trying to boot without a game...
            throw emulator_exception("Booting without content requires native firmware.");
        }

        if (header->IsDSiWare()) {
            // If we're using generated firmware and trying to boot a DSiWare game...
            throw emulator_exception("Booting into DSi mode requires native DSi firmware.");
        }

        retro::warn("Forcing direct boot due to generated firmware");
        _directBoot = true;
    }

    SPI_Firmware::InstallFirmware(std::move(firmware));
}

static void melonds::config::apply_audio_options() noexcept {
    ZoneScopedN("melonds::config::apply_audio_options");
    bool is_using_host_mic = audio::MicInputMode() == MicInputMode::HostMic;
    if (retro::microphone::is_interface_available()) {
        // Open the mic if the user wants it (and it isn't already open)
        // Close the mic if the user wants it (and it is open)
        bool ok = retro::microphone::set_open(is_using_host_mic);
        if (!ok) {
            // If we couldn't open or close the microphone...
            retro::warn("Failed to %s microphone", is_using_host_mic ? "open" : "close");
        }
    } else {
        if (is_using_host_mic && osd::ShowUnsupportedFeatureWarnings()) {
            retro::set_warn_message("This frontend doesn't support microphones.");
        }
    }

    SPU::SetInterpolation(static_cast<int>(config::audio::Interpolation()));
}

static void melonds::config::apply_save_options(const optional<NDSHeader>& header) {
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
            retro::info("Using existing homebrew SD card image \"%s\"", _dldiImagePath.c_str());
            _dldiImageSize = AUTO_SDCARD_SIZE;
        } else {
            retro::info("No homebrew SD card image found at \"%s\"; will create an image.", _dldiImagePath.c_str());
            _dldiImageSize = DEFAULT_SDCARD_SIZE;
        }

        if (DldiFolderSync()) {
            // If we want to sync the homebrew SD card to the host...
            if (!path_mkdir(_dldiFolderPath.c_str())) {
                // Create the save directory. If that failed...
                throw emulator_exception("Failed to create homebrew save directory at " + _dldiFolderPath);
            }

            retro::info("Created (or using existing) homebrew save directory \"%s\"", _dldiFolderPath.c_str());
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
            retro::info("Using existing DSi SD card image \"%s\"", _dsiSdImagePath.c_str());
            _dsiSdImageSize = AUTO_SDCARD_SIZE;
        } else {
            retro::info("No DSi SD card image found at \"%s\"; will create an image.", _dsiSdImagePath.c_str());
            _dsiSdImageSize = DEFAULT_SDCARD_SIZE;
        }

        if (DsiSdFolderSync()) {
            // If we want to sync the homebrew SD card to the host...
            if (!path_mkdir(_dsiSdFolderPath.c_str())) {
                // Create the save directory. If that failed...
                throw emulator_exception("Failed to create DSi SD card save directory at " + _dsiSdFolderPath);
            }

            retro::info("Created (or using existing) DSi SD card save directory \"%s\"", _dsiSdFolderPath.c_str());
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
}

static void melonds::config::set_core_options(
    const optional<retro_game_info> &nds_info,
    const optional<NDSHeader> &nds_header
)  {
    ZoneScopedN("retro::set_core_options");

    array categories = definitions::OptionCategories<RETRO_LANGUAGE_ENGLISH>;
    array definitions = definitions::CoreOptionDefinitions<RETRO_LANGUAGE_ENGLISH>;

    optional<string> subdir = retro::get_system_subdirectory();

    vector<string> dsiNandPaths, firmwarePaths;
    optional<string> sysdir = retro::get_system_directory();

    if (subdir) {
        retro_assert(sysdir.has_value());

        array paths = {*sysdir, *subdir};
        for (const string& path: paths) {
            for (const retro::dirent& d : retro::readdir(path, true)) {
                if (IsDsiNandImage(d)) {
                    dsiNandPaths.emplace_back(d.path);
                }
                if (IsFirmwareImage(d)) {
                    firmwarePaths.emplace_back(d.path);
                }
            }
        }

    } else {
        retro::set_error_message("Failed to get system directory, anything that needs it won't work.");
    }

    if (!dsiNandPaths.empty()) {
        // If we found at least one DSi NAND image...
        retro_core_option_v2_definition* dsiNandPathOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, melonds::config::storage::DSI_NAND_PATH);
        });

        retro_assert(dsiNandPathOption != definitions.end());

        memset(dsiNandPathOption->values, 0, sizeof(dsiNandPathOption->values));
        int length = std::min((int)dsiNandPaths.size(), (int)RETRO_NUM_CORE_OPTION_VALUES_MAX - 1);
        for (int i = 0; i < length; ++i) {
            retro::debug("Found a DSi NAND image at \"%s\"", dsiNandPaths[i].c_str());
            string_view path = dsiNandPaths[i];
            path.remove_prefix(sysdir->size() + 1);
            dsiNandPathOption->values[i].value = path.data();
            dsiNandPathOption->values[i].label = nullptr;
        }

        dsiNandPathOption->default_value = dsiNandPaths[0].c_str();
    }

    if (!firmwarePaths.empty()) {
        // If we found at least one firmware image...
        retro_core_option_v2_definition* firmwarePathOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, melonds::config::firmware::FIRMWARE_PATH);
        });
        retro_core_option_v2_definition* firmwarePathDsiOption = find_if(definitions.begin(), definitions.end(), [](const auto& def) {
            return string_is_equal(def.key, melonds::config::firmware::FIRMWARE_DSI_PATH);
        });

        retro_assert(firmwarePathOption != definitions.end());
        retro_assert(firmwarePathDsiOption != definitions.end());

        // Keep the first element, it's for built-in firmware
        // We subtract 2 because we need room for the terminating element and the built-in firmware
        int length = std::min((int)firmwarePaths.size(), (int)RETRO_NUM_CORE_OPTION_VALUES_MAX - 2);
        for (int i = 0; i < length; ++i) {
            retro::debug("Found a firmware image at \"%s\"", firmwarePaths[i].c_str());
            string_view path = firmwarePaths[i];
            path.remove_prefix(sysdir->size() + 1);
            firmwarePathOption->values[i + 1] = { path.data(), nullptr };
            firmwarePathDsiOption->values[i + 1] = { path.data(), nullptr };
        }
        firmwarePathOption->values[length + 2] = { nullptr, nullptr };
        firmwarePathDsiOption->values[length + 2] = { nullptr, nullptr };

        firmwarePathOption->default_value = firmwarePathOption->values[0].value;
        firmwarePathDsiOption->default_value = firmwarePathDsiOption->values[0].value;

        retro_assert(firmwarePathOption->values[0].value != nullptr);
        retro_assert(firmwarePathDsiOption->values[0].value != nullptr);
    }

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
        throw emulator_exception("Failed to set core options");
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

        case Firm_Language: return static_cast<int>(firmware::_language);
        case Firm_BirthdayMonth: return firmware::_birthdayMonth;
        case Firm_BirthdayDay: return firmware::_birthdayDay;
        case Firm_Color: return static_cast<int>(firmware::_favoriteColor);

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

        case ExternalBIOSEnable: return system::ExternalBiosEnable();

        case DLDI_Enable: return save::DldiEnable();
        case DLDI_ReadOnly: return save::DldiReadOnly();
        case DLDI_FolderSync: return save::DldiFolderSync();

        case DSiSD_Enable: return save::DsiSdEnable();
        case DSiSD_ReadOnly: return save::DsiSdReadOnly();
        case DSiSD_FolderSync: return save::DsiSdFolderSync();

        case Firm_OverrideSettings: return false;
        default: return false;
    }
}

std::string Platform::GetConfigString(ConfigEntry entry)
{
    using std::string;
    switch (entry)
    {
        case BIOS9Path: return string(system::Bios9Path());
        case BIOS7Path: return string(system::Bios7Path());
        case FirmwarePath: return string(system::FirmwarePath());

        case DSi_BIOS9Path: return string(system::DsiBios9Path());
        case DSi_BIOS7Path: return string(system::DsiBios7Path());
        case DSi_FirmwarePath: return string(system::DsiFirmwarePath());
        case DSi_NANDPath: return string(system::DsiNandPath());

        case DLDI_ImagePath: return save::DldiImagePath();
        case DLDI_FolderPath: return save::DldiFolderPath();

        case DSiSD_ImagePath: return save::DsiSdImagePath();
        case DSiSD_FolderPath: return save::DsiSdFolderPath();
        case WifiSettingsPath: return "wfcsettings.bin";

        case Firm_Username: return GetUsername(melonds::config::firmware::_usernameMode);
        case Firm_Message: return "";
        default: return "";
    }
}

bool Platform::GetConfigArray(ConfigEntry entry, void* data)
{
    using SPI_Firmware::MacAddress;
    switch (entry)
    {
        case Firm_MAC:
        {
            MacAddress* mac = (MacAddress*)data;
            MacAddress current_mac = *firmware::_macAddress;
            memcpy(mac, &current_mac, sizeof(MacAddress));
        }
        default:
            return false;
    }
}
