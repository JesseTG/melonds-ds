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

#include <charconv>
#include <cstring>
#include <initializer_list>

#include <file/file_path.h>
#include <libretro.h>
#include <retro_assert.h>
#include <string/stdstring.h>

#include <GPU.h>
#include <SPU.h>

#include "config/constants.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "input.hpp"
#include "libretro.hpp"
#include "microphone.hpp"
#include "opengl.hpp"
#include "render.hpp"
#include "screenlayout.hpp"
#include "tracy.hpp"

using std::from_chars;
using std::from_chars_result;
using std::initializer_list;
using std::nullopt;
using std::optional;
using std::string;

constexpr unsigned DS_NAME_LIMIT = 10;
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
    std::string LANDevice;
}

namespace melonds::config {
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
        static unsigned NumberOfShownScreenLayouts = screen::MAX_SCREEN_LAYOUTS;

#ifdef JIT_ENABLED
        static bool ShowJitOptions = true;
#endif
    }

    static void parse_jit_options() noexcept;
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
    static void parse_dsi_sd_options() noexcept;

    static void verify_nds_bios(bool ds_game_loaded);
    static void verify_dsi_bios();
    static void apply_system_options(const optional<NDSHeader>& header);
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
        static bool _firmwareSettingsOverrideEnable;
        bool FirmwareSettingsOverrideEnable() noexcept { return _firmwareSettingsOverrideEnable; }

        static FirmwareLanguage _language;
        FirmwareLanguage Language() noexcept { return _language; }

        static unsigned _birthdayMonth = 1;
        unsigned BirthdayMonth() noexcept { return _birthdayMonth; }

        static unsigned _birthdayDay = 1;
        unsigned BirthdayDay() noexcept { return _birthdayDay; }

        static Color _favoriteColor;
        Color FavoriteColor() noexcept { return _favoriteColor; }

        static string _username;
        std::string Username() noexcept { return _username; }

        static string _message;
        std::string Message() noexcept { return _message; }

        static ::melonds::MacAddress _macAddress;
        ::melonds::MacAddress MacAddress() noexcept { return _macAddress; }
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

        static bool _externalBiosFound;
        static bool _externalBiosEnable;
        bool ExternalBiosEnable() noexcept { return _externalBiosEnable && _externalBiosFound; }

        static unsigned _dsPowerOkayThreshold = 20;
        unsigned DsPowerOkayThreshold() noexcept { return _dsPowerOkayThreshold; }

        static unsigned _powerUpdateInterval;
        unsigned PowerUpdateInterval() noexcept { return _powerUpdateInterval; }

        // TODO: Allow these paths to be customized
        string Bios9Path() noexcept { return "bios9.bin"; }
        string Bios7Path() noexcept { return "bios7.bin"; }
        string FirmwarePath() noexcept { return "firmware.bin"; }
        string DsiBios9Path() noexcept { return "dsi_bios9.bin"; }
        string DsiBios7Path() noexcept { return "dsi_bios7.bin"; }
        string DsiFirmwarePath() noexcept { return "dsi_firmware.bin"; }
        string DsiNandPath() noexcept { return "dsi_nand.bin"; }
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
    config::parse_system_options();
    config::parse_jit_options();
    config::parse_homebrew_save_options(nds_info, header);
    config::parse_dsi_sd_options();
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
    set_option_visible(keys::RENDER_MODE, false);
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

    if (const char* value = get_variable(system::OVERRIDE_FIRMWARE_SETTINGS); !string_is_empty(value)) {
        _firmwareSettingsOverrideEnable = string_is_equal(value, values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", system::OVERRIDE_FIRMWARE_SETTINGS, values::DISABLED);
        _firmwareSettingsOverrideEnable = false;
    }

    if (!_firmwareSettingsOverrideEnable)
        return;

    if (const char* value = get_variable(system::LANGUAGE); !string_is_empty(value)) {
        if (string_is_equal(value, values::AUTO))
            _language = get_firmware_language(retro::get_language());
        else if (string_is_equal(value, values::JAPANESE))
            _language = FirmwareLanguage::Japanese;
        else if (string_is_equal(value, values::ENGLISH))
            _language = FirmwareLanguage::English;
        else if (string_is_equal(value, values::FRENCH))
            _language = FirmwareLanguage::French;
        else if (string_is_equal(value, values::GERMAN))
            _language = FirmwareLanguage::German;
        else if (string_is_equal(value, values::ITALIAN))
            _language = FirmwareLanguage::Italian;
        else if (string_is_equal(value, values::SPANISH))
            _language = FirmwareLanguage::Spanish;
    } else {
        retro::warn("Failed to get value for %s; defaulting to English", system::LANGUAGE);
        _language = FirmwareLanguage::English;
    }

    if (const char* value = get_variable(system::FAVORITE_COLOR); !string_is_empty(value)) {
        _favoriteColor = static_cast<Color>(std::clamp(std::stoi(value), 0, 15));
        // TODO: Warn if invalid
    } else {
        retro::warn("Failed to get value for %s; defaulting to gray", system::FAVORITE_COLOR);
        _favoriteColor = Color::Gray;
    }

    const char* retro_username;
    if (retro::environment(RETRO_ENVIRONMENT_GET_USERNAME, &retro_username) && !string_is_empty(retro_username)) {
        unsigned length = strlen(retro_username);
        _username = string(retro_username, 0, std::max(length, DS_NAME_LIMIT));

        retro::info("Overridden username: %s", _username.c_str());
    }
    else {
        retro::warn("Failed to get the user's name; defaulting to \"melonDS\"");
        _username = "melonDS";
    }

    // TODO: Make birthday configurable
    // TODO: Make message configurable with a file at runtime
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
static void melonds::config::parse_dsi_sd_options() noexcept {
    ZoneScopedN("melonds::config::parse_dsi_sd_options");
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
}

static void melonds::config::verify_nds_bios(bool ds_game_loaded) {
    ZoneScopedN("melonds::config::verify_nds_bios");
    using retro::log;
    using namespace melonds::config::system;
    retro_assert(config::system::ConsoleType() == ConsoleType::DS);

    // TODO: Allow user to force the use of a specific BIOS, and throw an exception if that's not possible
    if (_externalBiosEnable) {
        // If the player wants to use their own BIOS dumps...

        // melonDS doesn't properly fall back to FreeBIOS if the external bioses are missing,
        // so we have to do it ourselves

        std::array<std::string, 3> required_roms = {
            config::system::Bios7Path(),
            config::system::Bios9Path(),
            config::system::FirmwarePath(),
        };
        std::vector<std::string> missing_roms;

        // Check if any of the bioses / firmware files are missing
        for (const std::string& rom: required_roms) {
            if (Platform::LocalFileExists(rom)) {
                log(RETRO_LOG_INFO, "Found %s", rom.c_str());
            } else {
                missing_roms.push_back(rom);
                log(RETRO_LOG_WARN, "Could not find %s", rom.c_str());
            }
        }

        // TODO: Check both $SYSTEM/filename and $SYSTEM/melonDS DS/filename
        _externalBiosFound = missing_roms.empty();

        // Abort if there are any of the required roms are missing
        if (!_externalBiosFound) {
            retro::log(RETRO_LOG_WARN, "Using FreeBIOS instead of the aforementioned missing files.");
        }
    } else {
        retro::log(RETRO_LOG_INFO, "External BIOS is disabled, using internal FreeBIOS instead.");
    }

    if (!ExternalBiosEnable() && !ds_game_loaded) {
        // If we're using FreeBIOS and trying to boot without a game...

        throw melonds::unsupported_bios_exception("Booting without content requires a native BIOS.");
    }
}

static void melonds::config::verify_dsi_bios() {
    ZoneScopedN("melonds::config::verify_dsi_bios");
    using retro::info;
    using retro::warn;
    using namespace melonds::config::system;

    retro_assert(config::system::ConsoleType() == ConsoleType::DSi);
    if (!_externalBiosEnable) {
        throw melonds::unsupported_bios_exception(
            "DSi mode requires native BIOS to be enabled. Please enable it in the options menu.");
    }

    std::array<std::string, 4> required_roms = {
        config::system::DsiBios7Path(),
        config::system::DsiBios9Path(),
        config::system::DsiFirmwarePath(),
        config::system::DsiNandPath()
    };
    std::vector<std::string> missing_roms;

    // Check if any of the bioses / firmware files are missing
    for (const std::string& rom: required_roms) {
        if (Platform::LocalFileExists(rom)) {
            info("Found %s", rom.c_str());
        } else {
            missing_roms.push_back(rom);
            warn("Could not find %s", rom.c_str());
        }
    }

    _externalBiosFound = missing_roms.empty();
    // TODO: Check both $SYSTEM/filename and $SYSTEM/melonDS DS/filename

    // Abort if there are any of the required roms are missing
    if (!missing_roms.empty()) {
        throw melonds::missing_bios_exception(missing_roms);
    }
}

static void melonds::config::apply_system_options(const optional<NDSHeader>& header) {
    ZoneScopedN("melonds::config::apply_system_options");
    using namespace melonds::config::system;
    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...
        _consoleType = ConsoleType::DSi;
        retro::warn("Forcing DSi mode for DSiWare game");
    }

    switch (_consoleType) {
        case ConsoleType::DS:
            verify_nds_bios(header.has_value());
            break;
        case ConsoleType::DSi:
            verify_dsi_bios();
            break;
    }
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
        if (is_using_host_mic) {
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

struct retro_core_option_v2_category option_cats_us[] = {
    {
        melonds::config::system::CATEGORY,
        "System",
        "Change system settings."
    },
    {
        melonds::config::video::CATEGORY,
        "Video",
        "Change video settings."
    },
    {
        melonds::config::audio::CATEGORY,
        "Audio",
        "Change audio settings."
    },
    {
        melonds::config::screen::CATEGORY,
        "Screen",
        "Change screen settings."
    },
    {
        melonds::config::storage::CATEGORY,
        "Storage",
        "Change emulated SD card, NAND image, and save data settings."
    },
    {
        melonds::config::network::CATEGORY,
        "Network",
        "Change Nintendo Wi-Fi emulation settings."
    },
#ifdef JIT_ENABLED
    {
        melonds::config::cpu::CATEGORY,
        "CPU Emulation",
        "Change CPU emulation settings."
    },
#endif
    {nullptr, nullptr, nullptr},
};

/// All descriptive text uses semantic line breaks. https://sembr.org
struct retro_core_option_v2_definition melonds::option_defs_us[] = {
    // System
    {
        config::system::CONSOLE_MODE,
        "Console Type",
        nullptr,
        "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. "
        "Some features may not be available in DSi mode. "
        "DSi mode will be used if loading a DSiWare application.",
        nullptr,
        config::system::CATEGORY,
        {
            {melonds::config::values::DS, "DS"},
            {melonds::config::values::DSI, "DSi (experimental)"},
            {nullptr, nullptr},
        },
        melonds::config::values::DS
    },
    {
        config::system::BOOT_DIRECTLY,
        "Boot Game Directly",
        nullptr,
        "If enabled, melonDS will bypass the native DS menu and boot the loaded game directly. "
        "If disabled, native BIOS and firmware files must be provided in the system directory. "
        "Ignored if any of the following is true:\n"
        "\n"
        "- The core is loaded without a game\n"
        "- Native BIOS/firmware files weren't found\n"
        "- The loaded game is a DSiWare game\n",
        nullptr,
        config::system::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
    {
        config::system::OVERRIDE_FIRMWARE_SETTINGS,
        "Override Firmware Settings",
        nullptr,
        "Use language and username specified in the frontend, "
        "rather than those provided by the firmware itself. "
        "If disabled or the firmware is unavailable, these values will be provided by the frontend. "
        "If a name couldn't be found, \"melonDS\" will be used as the default.",
        nullptr,
        melonds::config::system::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },
    {
        config::system::LANGUAGE,
        "Language",
        nullptr,
        "The language mode of the emulated console. "
        "Not every game honors this setting. "
        "Automatic uses the frontend's language if supported by the DS, or English if not.",
        nullptr,
        melonds::config::system::CATEGORY,
        {
            {melonds::config::values::AUTO, "Automatic"},
            {melonds::config::values::ENGLISH, "English"},
            {melonds::config::values::JAPANESE, "Japanese"},
            {melonds::config::values::FRENCH, "French"},
            {melonds::config::values::GERMAN, "German"},
            {melonds::config::values::ITALIAN, "Italian"},
            {melonds::config::values::SPANISH, "Spanish"},
            {nullptr, nullptr},
        },
        melonds::config::values::AUTO
    },
    {
        config::system::FAVORITE_COLOR,
        "Favorite Color",
        nullptr,
        "The theme (\"favorite color\") of the emulated console.",
        nullptr,
        melonds::config::system::CATEGORY,
        {
            {"0", "Gray"},
            {"1", "Brown"},
            {"2", "Red"},
            {"3", "Light Pink"},
            {"4", "Orange"},
            {"5", "Yellow"},
            {"6", "Lime"},
            {"7", "Light Green"},
            {"8", "Dark Green"},
            {"9", "Turquoise"},
            {"10", "Light Blue"},
            {"11", "Blue"},
            {"12", "Dark Blue"},
            {"13", "Dark Purple"},
            {"14", "Light Purple"},
            {"15", "Dark Pink"},
            {nullptr, nullptr},
        },
        "0"
    },
    {
        config::system::USE_EXTERNAL_BIOS,
        "Use external BIOS if available",
        nullptr,
        "If enabled, melonDS will attempt to load a BIOS file from the system directory. "
        "If no valid BIOS is present, melonDS will fall back to its built-in FreeBIOS. "
        "Note that GBA connectivity requires a native BIOS. "
        "Takes effect at the next restart. "
        "If unsure, leave this enabled.",
        nullptr,
        melonds::config::system::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
    {
        config::system::BATTERY_UPDATE_INTERVAL,
        "Battery Update Interval",
        nullptr,
        "How often the emulated console's battery should be updated.",
        nullptr,
        config::system::CATEGORY,
        {
            {"1", "1 second"},
            {"2", "2 seconds"},
            {"3", "3 seconds"},
            {"5", "5 seconds"},
            {"10", "10 seconds"},
            {"15", "15 seconds"},
            {"20", "20 seconds"},
            {"30", "30 seconds"},
            {"60", "60 seconds"},
            {nullptr, nullptr}
        },
        "15"
    },
    {
        config::system::DS_POWER_OK,
        "DS Low Battery Threshold",
        nullptr,
        "If the host's battery level falls below this percentage, "
        "the emulated DS will report that its battery level is low. "
        "Ignored if running in DSi mode, "
        "no battery is available, "
        "or the frontend can't query the power status.",
        nullptr,
        config::system::CATEGORY,
        {
            {"0", "Always OK"},
            {"10", "10%"},
            {"20", "20%"},
            {"30", "30%"},
            {"40", "40%"},
            {"50", "50%"},
            {"60", "60%"},
            {"70", "70%"},
            {"80", "80%"},
            {"90", "90%"},
            {"100", "Always Low"},
            {nullptr, nullptr},
        },
        "20"
    },

    // DSi
    {
        config::storage::DSI_SD_SAVE_MODE,
        "Virtual SD Card (DSi)",
        nullptr,
        "If enabled, a virtual SD card will be made available to the emulated DSi. "
        "The card image must be within the frontend's system directory and be named dsi_sd_card.bin. "
        "If no image exists, a 4GB virtual SD card will be created. "
        "Ignored when in DS mode. "
        "Changes take effect at next boot.",
        nullptr,
        config::storage::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
    {
        config::storage::DSI_SD_READ_ONLY,
        "Read-Only Mode (DSi)",
        nullptr,
        "If enabled, the emulated DSi sees the virtual SD card as read-only. "
        "Changes take effect with next restart.",
        nullptr,
        config::storage::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },
    {
        config::storage::DSI_SD_SYNC_TO_HOST,
        "Sync SD Card to Host (DSi)",
        nullptr,
        "If enabled, the virtual SD card's files will be synced to this core's save directory. "
        "Enable this if you want to add files to the virtual SD card from outside the core. "
        "Syncing happens when loading and unloading a game, "
        "so external changes won't have any effect while the core is running. "
        "Takes effect at the next boot. "
        "Adjusting this setting may overwrite existing save data.",
        nullptr,
        config::storage::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },

    // Video
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    {
        config::video::RENDER_MODE,
        "Render Mode",
        nullptr,
        "OpenGL mode uses OpenGL for rendering graphics. "
        "If that doesn't work, software rendering is used as a fallback. "
        "Changes take effect next time the core restarts. ",
        nullptr,
        config::video::CATEGORY,
        {
            {melonds::config::values::SOFTWARE, "Software"},
            {melonds::config::values::OPENGL, "OpenGL"},
            {nullptr, nullptr},
        },
        "software"
    },
    {
        config::video::OPENGL_RESOLUTION,
        "Internal Resolution",
        nullptr,
        "The degree to which the emulated 3D engine's graphics are scaled up. "
        "Dimensions are given per screen. "
        "OpenGL renderer only.",
        nullptr,
        config::video::CATEGORY,
        {
            {"1", "1x native (256 x 192)"},
            {"2", "2x native (512 x 384)"},
            {"3", "3x native (768 x 576)"},
            {"4", "4x native (1024 x 768)"},
            {"5", "5x native (1280 x 960)"},
            {"6", "6x native (1536 x 1152)"},
            {"7", "7x native (1792 x 1344)"},
            {"8", "8x native (2048 x 1536)"},
            {nullptr, nullptr},
        },
        "1"
    },
    {
        config::video::OPENGL_BETTER_POLYGONS,
        "Improved Polygon Splitting",
        nullptr,
        "Enable this if your game's 3D models are not rendering correctly. "
        "OpenGL renderer only.",
        nullptr,
        config::video::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },
    {
        config::video::OPENGL_FILTERING,
        "Screen Filtering",
        nullptr,
        "Affects how the emulated screens are scaled to fit the real screen. "
        "Performance impact is minimal. "
        "OpenGL renderer only.\n"
        "\n"
        "Nearest: No filtering. Graphics look blocky.\n"
        "Linear: Smooth scaling.\n",
        nullptr,
        config::video::CATEGORY,
        {
            {melonds::config::values::NEAREST, "Nearest"},
            {melonds::config::values::LINEAR, "Linear"},
            {nullptr, nullptr},
        },
        melonds::config::values::NEAREST
    },
#endif
#ifdef HAVE_THREADS
    {
        config::video::THREADED_RENDERER,
        "Threaded Software Renderer",
        nullptr,
        "If enabled, the software renderer will run on a separate thread if possible. "
        "Otherwise, it will run on the main thread. "
        "Ignored if using the OpenGL renderer ."
        "Takes effect next time the core restarts. ",
        nullptr,
        config::video::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },
#endif
    // Audio Settings
    {
        config::audio::MIC_INPUT,
        "Microphone Input Mode",
        nullptr,
        "Choose the sound that the emulated microphone will receive:\n"
        "\n"
        "Silence: No audio input.\n"
        "Blow: Loop a built-in blowing sound.\n"
        "Noise: Random white noise.\n"
        "Microphone: Use your real microphone if available, fall back to Silence if not.",
        nullptr,
        config::audio::CATEGORY,
        {
            {melonds::config::values::SILENCE, "Silence"},
            {melonds::config::values::BLOW, "Blow"},
            {melonds::config::values::NOISE, "Noise"},
            {melonds::config::values::MICROPHONE, "Microphone"},
            {nullptr, nullptr},
        },
        melonds::config::values::MICROPHONE
    },
    {
        config::audio::MIC_INPUT_BUTTON,
        "Microphone Button Mode",
        nullptr,
        "Set the behavior of the Microphone button, "
        "even if Microphone Input Mode is set to Blow or Noise. "
        "The microphone receives silence when disabled by the button.\n"
        "\n"
        "Hold: Button enables mic input while held.\n"
        "Toggle: Button enables mic input when pressed, disables it when pressed again.\n"
        "Always: Button is ignored, mic input is always enabled.\n"
        "\n"
        "Ignored if Microphone Input Mode is set to Silence.",
        nullptr,
        config::audio::CATEGORY,
        {
            {melonds::config::values::HOLD, "Hold"},
            {melonds::config::values::TOGGLE, "Toggle"},
            {melonds::config::values::ALWAYS, "Always"},
            {nullptr, nullptr},
        },
        melonds::config::values::HOLD
    },
    {
        config::audio::AUDIO_BITDEPTH,
        "Audio Bit Depth",
        nullptr,
        "The audio playback bit depth. "
        "Automatic uses 10-bit audio for DS mode "
        "and 16-bit audio for DSi mode.\n"
        "\n"
        "Takes effect at next restart. "
        "If unsure, leave this set to Automatic.",
        nullptr,
        config::audio::CATEGORY,
        {
            {melonds::config::values::AUTO, "Automatic"},
            {melonds::config::values::_10BIT, "10-bit"},
            {melonds::config::values::_16BIT, "16-bit"},
            {nullptr, nullptr},
        },
        melonds::config::values::AUTO
    },
    {
        config::audio::AUDIO_INTERPOLATION,
        "Audio Interpolation",
        nullptr,
        "Interpolates audio output for improved quality. "
        "Disable this to match the behavior of the original DS hardware.",
        nullptr,
        config::audio::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::LINEAR, "Linear"},
            {melonds::config::values::COSINE, "Cosine"},
            {melonds::config::values::CUBIC, "Cubic"},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },

    {
        config::network::NETWORK_MODE,
        "Networking Mode",
        nullptr,
        "Configures how melonDS DS emulates Nintendo WFC. If unsure, use Indirect mode.\n"
        "\n"
        "Indirect: Use libslirp to emulate the DS's network stack. Simple and needs no setup.\n"
#ifdef HAVE_NETWORKING_DIRECT_MODE
        "Direct: Routes emulated Wi-fi packets to the host's network interface. "
        "Faster and more reliable, but requires an ethernet connection and "
#ifdef _WIN32
        "that WinPcap or Npcap is installed. "
#else
        "that libpcap is installed. "
#endif
        "If unavailable, falls back to Indirect mode.\n"
#endif
        "\n"
        "Changes take effect at next restart. "
        "Not related to local multiplayer.",
        nullptr,
        config::network::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::INDIRECT, "Indirect"},
            {melonds::config::values::DIRECT, "Direct"},
            {nullptr, nullptr},
        },
        melonds::config::values::INDIRECT
    },

    {
        config::screen::SHOW_CURSOR,
        "Cursor Mode",
        nullptr,
        "Determines when a cursor should appear on the bottom screen. "
        "Never is recommended for touch screens; "
        "the other settings are best suited for mouse or joystick input.",
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::DISABLED, "Never"},
            {melonds::config::values::TOUCHING, "While Touching"},
            {melonds::config::values::TIMEOUT, "Until Timeout"},
            {melonds::config::values::ALWAYS, "Always"},
            {nullptr, nullptr},
        },
        melonds::config::values::ALWAYS
    },
    {
        config::screen::CURSOR_TIMEOUT,
        "Cursor Timeout",
        nullptr,
        "If Cursor Mode is set to \"Until Timeout\", "
        "then the cursor will be hidden if the pointer hasn't moved for a certain time.",
        nullptr,
        config::screen::CATEGORY,
        {
            {"1", "1 second"},
            {"2", "2 seconds"},
            {"3", "3 seconds"},
            {"5", "5 seconds"},
            {"10", "10 seconds"},
            {"15", "15 seconds"},
            {"20", "20 seconds"},
            {"30", "30 seconds"},
            {"60", "60 seconds"},
            {nullptr, nullptr},
        },
        "3"
    },
    {
        config::screen::HYBRID_RATIO,
        "Hybrid Ratio",
        nullptr,
        "The size of the larger screen relative to the smaller ones when using a hybrid layout.",
        nullptr,
        config::screen::CATEGORY,
        {
            {"2", "2:1"},
            {"3", "3:1"},
            {nullptr, nullptr},
        },
        "2"
    },
    {
        config::screen::HYBRID_SMALL_SCREEN,
        "Hybrid Small Screen Mode",
        nullptr,
        "Choose which screens will be shown when using a hybrid layout.",
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::ONE, "Show Opposite Screen"},
            {melonds::config::values::BOTH, "Show Both Screens"},
            {nullptr, nullptr},
        },
        melonds::config::values::BOTH
    },
    {
        config::screen::SCREEN_GAP,
        "Screen Gap",
        nullptr,
        "Choose how large the gap between the 2 screens should be.",
        nullptr,
        config::screen::CATEGORY,
        {
            {"0", "None"},
            {"1", "1px"},
            {"2", "2px"},
            {"8", "8px"},
            {"16", "16px"},
            {"24", "24px"},
            {"32", "32px"},
            {"48", "48px"},
            {"64", "64px"},
            {"72", "72px"},
            {"88", "88px"},
            {"90", "90px"},
            {"128", "128px"},
            {nullptr, nullptr},
        },
        "0"
    },
    {
        config::screen::NUMBER_OF_SCREEN_LAYOUTS,
        "# of Screen Layouts",
        nullptr,
        "The number of screen layouts to cycle through with the Next Layout button.",
        nullptr,
        config::screen::CATEGORY,
        {
            {"1", nullptr},
            {"2", nullptr},
            {"3", nullptr},
            {"4", nullptr},
            {"5", nullptr},
            {"6", nullptr},
            {"7", nullptr},
            {"8", nullptr},
            {nullptr, nullptr},
        },
        "2"
    },
    {
        config::screen::SCREEN_LAYOUT1,
        "Screen Layout #1",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::TOP_BOTTOM
    },
    {
        config::screen::SCREEN_LAYOUT2,
        "Screen Layout #2",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::LEFT_RIGHT
    },
    {
        config::screen::SCREEN_LAYOUT3,
        "Screen Layout #3",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::TOP
    },
    {
        config::screen::SCREEN_LAYOUT4,
        "Screen Layout #4",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::BOTTOM
    },
    {
        config::screen::SCREEN_LAYOUT5,
        "Screen Layout #5",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::HYBRID_TOP
    },
    {
        config::screen::SCREEN_LAYOUT6,
        "Screen Layout #6",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::HYBRID_BOTTOM
    },
    {
        config::screen::SCREEN_LAYOUT7,
        "Screen Layout #7",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::BOTTOM_TOP
    },
    {
        config::screen::SCREEN_LAYOUT8,
        "Screen Layout #8",
        nullptr,
        nullptr,
        nullptr,
        config::screen::CATEGORY,
        {
            {melonds::config::values::TOP_BOTTOM, "Top/Bottom"},
            {melonds::config::values::BOTTOM_TOP, "Bottom/Top"},
            {melonds::config::values::LEFT_RIGHT, "Left/Right"},
            {melonds::config::values::RIGHT_LEFT, "Right/Left"},
            {melonds::config::values::TOP, "Top Only"},
            {melonds::config::values::BOTTOM, "Bottom Only"},
            {melonds::config::values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {melonds::config::values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {melonds::config::values::ROTATE_LEFT, "Rotated Left"},
            {melonds::config::values::ROTATE_RIGHT, "Rotated Right"},
            {melonds::config::values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        melonds::config::values::RIGHT_LEFT
    },

    // Homebrew Save Data
    {
        config::storage::HOMEBREW_SAVE_MODE,
        "Virtual SD Card",
        nullptr,
        "If enabled, a virtual SD card will be made available to homebrew DS games. "
        "The card image must be within the frontend's system directory and be named dldi_sd_card.bin. "
        "If no image exists, a 4GB virtual SD card will be created. "
        "Ignored for retail games. "
        "Changes take effect at next boot.",
        nullptr,
        config::storage::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
    {
        config::storage::HOMEBREW_READ_ONLY,
        "Read-Only Mode",
        nullptr,
        "If enabled, homebrew applications will see the virtual SD card as read-only. "
        "Changes take effect with next restart.",
        nullptr,
        config::storage::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },
    {
        config::storage::HOMEBREW_SYNC_TO_HOST,
        "Sync SD Card to Host",
        nullptr,
        "If enabled, the virtual SD card's files will be synced to this core's save directory. "
        "Enable this if you want to add files to the virtual SD card from outside the core. "
        "Syncing happens when loading and unloading a game, "
        "so external changes won't have any effect while the core is running. "
        "Takes effect at the next boot. "
        "Adjusting this setting may overwrite existing save data.",
        nullptr,
        config::storage::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::DISABLED
    },
#ifdef JIT_ENABLED
    {
        config::cpu::JIT_ENABLE,
        "JIT Enable (Restart)",
        nullptr,
        "Recompiles emulated machine code as it runs. "
        "Restart required to take effect. "
        "If unsure, leave enabled.",
        nullptr,
        melonds::config::cpu::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
    {
        config::cpu::JIT_BLOCK_SIZE,
        "JIT Block Size",
        nullptr,
        nullptr,
        nullptr,
        melonds::config::cpu::CATEGORY,
        {
            {"1", nullptr},
            {"2", nullptr},
            {"3", nullptr},
            {"4", nullptr},
            {"5", nullptr},
            {"6", nullptr},
            {"7", nullptr},
            {"8", nullptr},
            {"9", nullptr},
            {"10", nullptr},
            {"11", nullptr},
            {"12", nullptr},
            {"13", nullptr},
            {"14", nullptr},
            {"15", nullptr},
            {"16", nullptr},
            {"17", nullptr},
            {"18", nullptr},
            {"19", nullptr},
            {"20", nullptr},
            {"21", nullptr},
            {"22", nullptr},
            {"23", nullptr},
            {"24", nullptr},
            {"25", nullptr},
            {"26", nullptr},
            {"27", nullptr},
            {"28", nullptr},
            {"29", nullptr},
            {"30", nullptr},
            {"31", nullptr},
            {"32", nullptr},
            {nullptr, nullptr},
        },
        "32"
    },
    {
        config::cpu::JIT_BRANCH_OPTIMISATIONS,
        "JIT Branch Optimisations",
        nullptr,
        nullptr,
        nullptr,
        melonds::config::cpu::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
    {
        config::cpu::JIT_LITERAL_OPTIMISATIONS,
        "JIT Literal Optimisations",
        nullptr,
        nullptr,
        nullptr,
        melonds::config::cpu::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
#ifdef HAVE_JIT_FASTMEM
    {
        config::cpu::JIT_FAST_MEMORY,
        "JIT Fast Memory",
        nullptr,
        nullptr,
        nullptr,
        melonds::config::cpu::CATEGORY,
        {
            {melonds::config::values::DISABLED, nullptr},
            {melonds::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        melonds::config::values::ENABLED
    },
#endif
#endif
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, {{nullptr}}, nullptr},
};


struct retro_core_options_v2 melonds::options_us = {
    option_cats_us,
    option_defs_us
};


#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2* melonds::options_intl[] = {
    &options_us, /* RETRO_LANGUAGE_ENGLISH */
    nullptr,        /* RETRO_LANGUAGE_JAPANESE */
    nullptr,        /* RETRO_LANGUAGE_FRENCH */
    nullptr,        /* RETRO_LANGUAGE_SPANISH */
    nullptr,        /* RETRO_LANGUAGE_GERMAN */
    nullptr,        /* RETRO_LANGUAGE_ITALIAN */
    nullptr,        /* RETRO_LANGUAGE_DUTCH */
    nullptr,        /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
    nullptr,        /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
    nullptr,        /* RETRO_LANGUAGE_RUSSIAN */
    nullptr,        /* RETRO_LANGUAGE_KOREAN */
    nullptr,        /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
    nullptr,        /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
    nullptr,        /* RETRO_LANGUAGE_ESPERANTO */
    nullptr,        /* RETRO_LANGUAGE_POLISH */
    nullptr,        /* RETRO_LANGUAGE_VIETNAMESE */
    nullptr,        /* RETRO_LANGUAGE_ARABIC */
    nullptr,        /* RETRO_LANGUAGE_GREEK */
    nullptr,        /* RETRO_LANGUAGE_TURKISH */
    nullptr,        /* RETRO_LANGUAGE_SLOVAK */
    nullptr,        /* RETRO_LANGUAGE_PERSIAN */
    nullptr,        /* RETRO_LANGUAGE_HEBREW */
    nullptr,        /* RETRO_LANGUAGE_ASTURIAN */
    nullptr,        /* RETRO_LANGUAGE_FINNISH */
    nullptr,        /* RETRO_LANGUAGE_INDONESIAN */
    nullptr,        /* RETRO_LANGUAGE_SWEDISH */
    nullptr,        /* RETRO_LANGUAGE_UKRAINIAN */
    nullptr,        /* RETRO_LANGUAGE_CZECH */
    nullptr,        /* RETRO_LANGUAGE_CATALAN_VALENCIA */
    nullptr,        /* RETRO_LANGUAGE_CATALAN */
};
#endif