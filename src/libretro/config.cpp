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
#include <cstring>
#include <GPU.h>
#include <string/stdstring.h>
#include <file/file_path.h>
#include <SPU.h>
#include <retro_assert.h>

#include "content.hpp"
#include "libretro.hpp"
#include "environment.hpp"
#include "screenlayout.hpp"
#include "input.hpp"
#include "opengl.hpp"
#include "microphone.hpp"
#include "utils.hpp"
#include "render.hpp"
#include "exceptions.hpp"

using std::string;
using std::nullopt;
using std::optional;

constexpr unsigned DS_NAME_LIMIT = 10;

namespace Config {
    namespace Retro {
        // bool RandomizeMac = false;
        //melonds::ScreenSwapMode ScreenSwapMode;
        //melonds::Renderer CurrentRenderer;
        //float CursorSize = 2.0;

        namespace Category {
            static const char* const VIDEO = "video";
            static const char* const AUDIO = "audio";
            static const char* const SYSTEM = "system";
            static const char* const SCREEN = "screen";
        }

        namespace Keys {
            static const char* const FAVORITE_COLOR = "melonds_firmware_favorite_color";
            static const char* const OPENGL_RESOLUTION = "melonds_opengl_resolution";
            static const char* const THREADED_RENDERER = "melonds_threaded_renderer";
            static const char* const OPENGL_BETTER_POLYGONS = "melonds_opengl_better_polygons";
            static const char* const OPENGL_FILTERING = "melonds_opengl_filtering";
            static const char* const RENDER_MODE = "melonds_render_mode";
            static const char* const SCREEN_LAYOUT = "melonds_screen_layout";
            static const char* const HYBRID_SMALL_SCREEN = "melonds_hybrid_small_screen";
            static const char* const HYBRID_RATIO = "melonds_hybrid_ratio";
            static const char* const JIT_ENABLE = "melonds_jit_enable";
            static const char* const JIT_BLOCK_SIZE = "melonds_jit_block_size";
            static const char* const JIT_BRANCH_OPTIMISATIONS = "melonds_jit_branch_optimisations";
            static const char* const JIT_LITERAL_OPTIMISATIONS = "melonds_jit_literal_optimisations";
            static const char* const JIT_FAST_MEMORY = "melonds_jit_fast_memory";
            static const char* const USE_EXTERNAL_BIOS = "melonds_use_external_bios";
            static const char* const CONSOLE_MODE = "melonds_console_mode";
            static const char* const BOOT_DIRECTLY = "melonds_boot_directly";
            static const char* const SCREEN_GAP = "melonds_screen_gap";
            static const char* const SWAPSCREEN_MODE = "melonds_swapscreen_mode";
            static const char* const RANDOMIZE_MAC_ADDRESS = "melonds_randomize_mac_address";
            static const char* const TOUCH_MODE = "melonds_touch_mode";
            static const char* const MIC_INPUT_BUTTON = "melonds_mic_input_active";
            static const char* const MIC_INPUT = "melonds_mic_input";
            static const char* const AUDIO_BITDEPTH = "melonds_audio_bitdepth";
            static const char* const AUDIO_INTERPOLATION = "melonds_audio_interpolation";
            static const char* const OVERRIDE_FIRMWARE_SETTINGS = "melonds_override_fw_settings";
            static const char* const LANGUAGE = "melonds_language";
            static const char* const HOMEBREW_SAVE_MODE = "melonds_homebrew_sdcard";
            static const char* const HOMEBREW_READ_ONLY = "melonds_homebrew_readonly";
            static const char* const HOMEBREW_SYNC_TO_HOST = "melonds_homebrew_sync_sdcard_to_host";
            static const char* const DSI_SD_SAVE_MODE = "melonds_dsi_sdcard";
            static const char* const DSI_SD_READ_ONLY = "melonds_dsi_sdcard_readonly";
            static const char* const DSI_SD_SYNC_TO_HOST = "melonds_dsi_sdcard_sync_sdcard_to_host";
            static const char* const GBA_FLUSH_DELAY = "melonds_gba_flush_delay";
        }

        namespace Values {
            static const char* const _10BIT = "10bit";
            static const char* const _16BIT = "16bit";
            static const char* const ALWAYS = "always";
            static const char* const AUTO = "auto";
            static const char* const BLOW = "blow";
            static const char* const BOTTOM_TOP = "bottom-top";
            static const char* const BOTH = "both";
            static const char* const BOTTOM = "bottom";
            static const char* const COSINE = "cosine";
            static const char* const CUBIC = "cubic";
            static const char* const DEDICATED = "dedicated";
            static const char* const DEFAULT = "default";
            static const char* const DISABLED = "disabled";
            static const char* const DS = "ds";
            static const char* const DSI = "dsi";
            static const char* const ENABLED = "enabled";
            static const char* const ENGLISH = "en";
            static const char* const FRENCH = "fr";
            static const char* const GERMAN = "de";
            static const char* const HOLD = "hold";
            static const char* const HYBRID_BOTTOM = "hybrid-bottom";
            static const char* const HYBRID_TOP = "hybrid-top";
            static const char* const ITALIAN = "it";
            static const char* const JAPANESE = "ja";
            static const char* const JOYSTICK = "joystick";
            static const char* const LEFT_RIGHT = "left-right";
            static const char* const LINEAR = "linear";
            static const char* const NEAREST = "nearest";
            static const char* const MICROPHONE = "microphone";
            static const char* const MOUSE = "mouse";
            static const char* const NOISE = "noise";
            static const char* const OPENGL = "opengl";
            static const char* const RIGHT_LEFT = "right-left";
            static const char* const ROTATE_LEFT = "rotate-left";
            static const char* const ROTATE_RIGHT = "rotate-right";
            static const char* const SHARED = "shared";
            static const char* const SHARED256M = "shared0256m";
            static const char* const SHARED512M = "shared0512m";
            static const char* const SHARED1G = "shared1024m";
            static const char* const SHARED2G = "shared2048m";
            static const char* const SHARED4G = "shared4096m";
            static const char* const SHARED4GDSI = "shared4096m-dsi";
            static const char* const SILENCE = "silence";
            static const char* const SOFTWARE = "software";
            static const char* const SPANISH = "es";
            static const char* const TOGGLE = "toggle";
            static const char* const TOP_BOTTOM = "top-bottom";
            static const char* const TOP = "top";
            static const char* const TOUCH = "touch";
            static const char* const UPSIDE_DOWN = "rotate-180";
        }
    }
}

namespace melonds::config {
    namespace visibility {
        static bool ShowDsiOptions = true;
        static bool ShowDsiSdCardOptions = true;
#ifdef HAVE_OPENGL
        static bool ShowOpenGlOptions = true;
#endif
        static bool ShowSoftwareRenderOptions = true;
        static bool ShowHybridOptions = true;
        static bool ShowVerticalLayoutOptions = true;

#ifdef JIT_ENABLED
        static bool ShowJitOptions = true;
#endif
    }

    static optional<Renderer> ParseRenderer(const char* value) noexcept;


    static void parse_jit_options() noexcept;
    static void parse_system_options() noexcept;
    static void parse_firmware_options() noexcept;
    static void parse_audio_options() noexcept;

    /// @returns true if the OpenGL state needs to be rebuilt
    static bool parse_video_options(bool initializing) noexcept;
    static bool parse_screen_options() noexcept;
    static void parse_homebrew_save_options(const optional<struct retro_game_info>& nds_info,
                                            const optional <NDSHeader>& header) noexcept;
    static void
    parse_dsi_sd_options(const optional<struct retro_game_info>& nds_info, const optional <NDSHeader>& header) noexcept;

    static void verify_nds_bios(bool ds_game_loaded);
    static void verify_dsi_bios();
    static void apply_system_options(const optional<NDSHeader>& header);
    static void apply_audio_options() noexcept;
    static void apply_save_options(const optional<NDSHeader>& header);
    static void apply_screen_options() noexcept;

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

    namespace save {
        SdCardMode _dldiSdCardMode;
        SdCardMode DldiSdCardMode() noexcept { return _dldiSdCardMode; }

        bool DldiEnable() noexcept { return _dldiSdCardMode != SdCardMode::None; }

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

        SdCardMode _dsiSdCardMode;
        SdCardMode DsiSdCardMode() noexcept { return _dsiSdCardMode; }

        bool DsiSdEnable() noexcept { return _dsiSdCardMode != SdCardMode::None; }

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
        static melonds::ScreenLayout _screenLayout;
        melonds::ScreenLayout ScreenLayout() noexcept { return _screenLayout; }

        static unsigned _screenGap;
        unsigned ScreenGap() noexcept { return _screenGap; }

#ifdef HAVE_OPENGL
        static unsigned _hybridRatio;
        unsigned HybridRatio() noexcept { return _hybridRatio; }
#else
        unsigned HybridRatio() noexcept { return 2; }
#endif

        static melonds::ScreenSwapMode _screenSwapMode;
        melonds::ScreenSwapMode ScreenSwapMode() noexcept { return _screenSwapMode; }

        static melonds::SmallScreenLayout _smallScreenLayout;
        melonds::SmallScreenLayout SmallScreenLayout() noexcept { return _smallScreenLayout; }

        static melonds::TouchMode _touchMode;
        melonds::TouchMode TouchMode() noexcept { return _touchMode; }
    }

    namespace system {
        static melonds::ConsoleType _consoleType;
        melonds::ConsoleType ConsoleType() noexcept { return _consoleType; }

        static bool _directBoot;
        bool DirectBoot() noexcept { return _directBoot; }

        static bool _externalBiosFound;
        static bool _externalBiosEnable;
        bool ExternalBiosEnable() noexcept { return _externalBiosEnable && _externalBiosFound; }

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
#if defined(HAVE_OPENGL) || defined(HAVE_THREADS)
        static GPU::RenderSettings _renderSettings = {false, 1, false};
        GPU::RenderSettings RenderSettings() noexcept { return _renderSettings; }
#else
        GPU::RenderSettings RenderSettings() noexcept { return {false, 1, false}; }
#endif

        // TODO: Make configurable
        float CursorSize() noexcept { return 2.0; }

#ifdef HAVE_OPENGL
        static melonds::Renderer _configuredRenderer;
        melonds::Renderer ConfiguredRenderer() noexcept { return _configuredRenderer; }
#else
        melonds::Renderer ConfiguredRenderer() noexcept { return melonds::Renderer::Software; }
#endif

#ifdef HAVE_OPENGL
        static melonds::ScreenFilter _screenFilter;
        melonds::ScreenFilter ScreenFilter() noexcept { return _screenFilter; }
#endif

        int ScaleFactor() noexcept { return RenderSettings().GL_ScaleFactor; }
    }
}



static optional<melonds::Renderer> melonds::config::ParseRenderer(const char* value) noexcept {
    if (string_is_equal(value, Config::Retro::Values::SOFTWARE)) return melonds::Renderer::Software;
    if (string_is_equal(value, Config::Retro::Values::OPENGL)) return melonds::Renderer::OpenGl;
    return nullopt;
}

static optional<melonds::ConsoleType> ParseConsoleType(const char* value) noexcept {
    if (string_is_equal(value, Config::Retro::Values::DS)) return melonds::ConsoleType::DS;
    if (string_is_equal(value, Config::Retro::Values::DSI)) return melonds::ConsoleType::DSi;
    return nullopt;
}

static optional<bool> ParseBoolean(const char* value) noexcept {
    if (string_is_equal(value, Config::Retro::Values::ENABLED)) return true;
    if (string_is_equal(value, Config::Retro::Values::DISABLED)) return false;
    return nullopt;
}

static optional<melonds::ScreenLayout> ParseScreenLayout(const char* value) noexcept {
    using melonds::ScreenLayout;
    using namespace Config::Retro;
    if (string_is_equal(value, Values::TOP_BOTTOM)) return ScreenLayout::TopBottom;
    if (string_is_equal(value, Values::BOTTOM_TOP)) return ScreenLayout::BottomTop;
    if (string_is_equal(value, Values::LEFT_RIGHT)) return ScreenLayout::LeftRight;
    if (string_is_equal(value, Values::RIGHT_LEFT)) return ScreenLayout::RightLeft;
    if (string_is_equal(value, Values::TOP)) return ScreenLayout::TopOnly;
    if (string_is_equal(value, Values::BOTTOM)) return ScreenLayout::BottomOnly;
    if (string_is_equal(value, Values::HYBRID_TOP)) return ScreenLayout::HybridTop;
    if (string_is_equal(value, Values::HYBRID_BOTTOM)) return ScreenLayout::HybridBottom;
    if (string_is_equal(value, Values::ROTATE_LEFT)) return ScreenLayout::TurnLeft;
    if (string_is_equal(value, Values::ROTATE_RIGHT)) return ScreenLayout::TurnRight;
    if (string_is_equal(value, Values::UPSIDE_DOWN)) return ScreenLayout::UpsideDown;

    return nullopt;
}

void melonds::InitConfig(const optional<struct retro_game_info>& nds_info, const optional<NDSHeader>& header) {
    config::parse_system_options();
    config::parse_jit_options();
    config::parse_homebrew_save_options(nds_info, header);
    config::parse_dsi_sd_options(nds_info, header);
    config::parse_firmware_options();
    config::parse_audio_options();
    bool openGlNeedsRefresh = config::parse_video_options(true);
    openGlNeedsRefresh |= config::parse_screen_options();

    config::apply_system_options(header);
    config::apply_save_options(header);
    config::apply_audio_options();
    config::apply_screen_options();


    if (melonds::opengl::UsingOpenGl() && openGlNeedsRefresh) {
        // If we're using OpenGL and the settings changed, or the screen layout changed...
        melonds::opengl::RequestOpenGlRefresh();
    }

    update_option_visibility();
}

void melonds::UpdateConfig(const std::optional<struct retro_game_info>& nds_info,
                           const std::optional<NDSHeader>& header) noexcept {
    config::parse_audio_options();
    bool openGlNeedsRefresh = config::parse_video_options(false);
    openGlNeedsRefresh |= config::parse_screen_options();

    config::apply_audio_options();
    config::apply_screen_options();

    if (melonds::opengl::UsingOpenGl() && openGlNeedsRefresh) {
        // If we're using OpenGL and the settings changed, or the screen layout changed...
        melonds::opengl::RequestOpenGlRefresh();
    }

    update_option_visibility();
}

bool melonds::update_option_visibility() {
    using namespace Config::Retro;
    using namespace melonds::config;
    using namespace melonds::config::visibility;
    using retro::environment;
    using retro::get_variable;
    using retro::set_option_visible;
    bool updated = false;

    // Convention: if an option is not found, show any dependent options
#ifdef HAVE_OPENGL
    // Show/hide OpenGL core options
    bool oldShowOpenGlOptions = ShowOpenGlOptions;
    bool oldShowSoftwareRenderOptions = ShowSoftwareRenderOptions;

    optional<Renderer> renderer = ParseRenderer(get_variable(Keys::RENDER_MODE));
    ShowOpenGlOptions = !renderer || *renderer == Renderer::OpenGl;
    ShowSoftwareRenderOptions = !ShowOpenGlOptions;

    if (ShowOpenGlOptions != oldShowOpenGlOptions) {
        set_option_visible(Keys::OPENGL_RESOLUTION, ShowOpenGlOptions);
        set_option_visible(Keys::OPENGL_FILTERING, ShowOpenGlOptions);
        set_option_visible(Keys::OPENGL_BETTER_POLYGONS, ShowOpenGlOptions);

        updated = true;
    }

    if (ShowSoftwareRenderOptions != oldShowSoftwareRenderOptions) {
        set_option_visible(Keys::THREADED_RENDERER, ShowSoftwareRenderOptions);

        updated = true;
    }
#else
    set_option_visible(Keys::RENDER_MODE, false);
#endif

    bool oldShowDsiOptions = ShowDsiOptions;
    bool oldShowDsiSdCardOptions = ShowDsiSdCardOptions;
    optional<ConsoleType> consoleType = ParseConsoleType(get_variable(Keys::CONSOLE_MODE));

    ShowDsiOptions = !consoleType || *consoleType == ConsoleType::DSi;
    if (ShowDsiOptions != oldShowDsiOptions) {
        set_option_visible(Keys::DSI_SD_SAVE_MODE, ShowDsiOptions);
        set_option_visible(Keys::DSI_SD_READ_ONLY, ShowDsiOptions);
        set_option_visible(Keys::DSI_SD_SYNC_TO_HOST, ShowDsiOptions);

        updated = true;
    }

    // Show/hide Hybrid screen options
    bool oldShowHybridOptions = ShowHybridOptions;
    bool oldShowVerticalLayoutOptions = ShowVerticalLayoutOptions;
    optional<ScreenLayout> layout = ParseScreenLayout(get_variable(Keys::SCREEN_LAYOUT));
    ShowHybridOptions = !layout || *layout == ScreenLayout::HybridTop || *layout == ScreenLayout::HybridBottom;
    ShowVerticalLayoutOptions = !layout || *layout == ScreenLayout::TopBottom || *layout == ScreenLayout::BottomTop;
    if (ShowHybridOptions != oldShowHybridOptions) {
        set_option_visible(Keys::HYBRID_SMALL_SCREEN, ShowHybridOptions);
#ifdef HAVE_OPENGL
        set_option_visible(Keys::HYBRID_RATIO, ShowHybridOptions);
#endif

        updated = true;
    }

    if (ShowVerticalLayoutOptions != oldShowVerticalLayoutOptions) {
        set_option_visible(Keys::SCREEN_GAP, ShowVerticalLayoutOptions);

        updated = true;
    }

#ifdef JIT_ENABLED
    // Show/hide JIT core options
    bool oldShowJitOptions = ShowJitOptions;
    optional<bool> jitEnabled = ParseBoolean(get_variable(Keys::JIT_ENABLE));

    ShowJitOptions = !jitEnabled || *jitEnabled;

    if (ShowJitOptions != oldShowJitOptions) {
        set_option_visible(Keys::JIT_BLOCK_SIZE, ShowJitOptions);
        set_option_visible(Keys::JIT_BRANCH_OPTIMISATIONS, ShowJitOptions);
        set_option_visible(Keys::JIT_LITERAL_OPTIMISATIONS, ShowJitOptions);
#ifdef HAVE_JIT_FASTMEM
        set_option_visible(Keys::JIT_FAST_MEMORY, ShowJitOptions);
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
#ifdef HAVE_JIT
    using namespace Config::Retro;
    using namespace melonds::config::jit;
    using retro::get_variable;

    if (const char* value = get_variable(Keys::JIT_ENABLE); !string_is_empty(value)) {
        _jitEnable = string_is_equal(value, Values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::JIT_ENABLE, Values::ENABLED);
        _jitEnable = true;
    }

    if (const char* value = get_variable(Keys::JIT_BLOCK_SIZE); !string_is_empty(value)) {
        _maxBlockSize = std::stoi(value);
    } else {
        retro::warn("Failed to get value for %s; defaulting to 32", Keys::JIT_BLOCK_SIZE);
        _maxBlockSize = 32;
    }

    if (const char* value = get_variable(Keys::JIT_BRANCH_OPTIMISATIONS); !string_is_empty(value)) {
        _branchOptimizations = string_is_equal(value, Values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::JIT_BRANCH_OPTIMISATIONS, Values::ENABLED);
        _branchOptimizations = true;
    }

    if (const char* value = get_variable(Keys::JIT_LITERAL_OPTIMISATIONS); !string_is_empty(value)) {
        _literalOptimizations = string_is_equal(value, Values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::JIT_LITERAL_OPTIMISATIONS, Values::ENABLED);
        _literalOptimizations = true;
    }

#ifdef HAVE_JIT_FASTMEM
    if (const char* value = get_variable(Keys::JIT_FAST_MEMORY); !string_is_empty(value)) {
        _fastMemory = string_is_equal(value, Values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::JIT_FAST_MEMORY, Values::ENABLED);
        _fastMemory = true;
    }
#endif
#endif
}

static void melonds::config::parse_system_options() noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::system;
    using retro::get_variable;

    // All of these options take effect when a game starts, so there's no need to update them mid-game

    if (const optional<melonds::ConsoleType> type = ParseConsoleType(get_variable(Keys::CONSOLE_MODE))) {
        _consoleType = *type;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::CONSOLE_MODE, Values::DS);
        _consoleType = ConsoleType::DS;
    }

    if (const optional<bool> value = ParseBoolean(get_variable(Keys::BOOT_DIRECTLY))) {
        _directBoot = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::BOOT_DIRECTLY, Values::ENABLED);
        _directBoot = true;
    }

    if (const char* value = get_variable(Keys::USE_EXTERNAL_BIOS); !string_is_empty(value)) {
        _externalBiosEnable = string_is_equal(value, Values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::USE_EXTERNAL_BIOS, Values::ENABLED);
        _externalBiosEnable = true;
    }
}

static void melonds::config::parse_firmware_options() noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::firmware;
    using retro::get_variable;

    if (const char* value = get_variable(Keys::OVERRIDE_FIRMWARE_SETTINGS); !string_is_empty(value)) {
        _firmwareSettingsOverrideEnable = string_is_equal(value, Values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::OVERRIDE_FIRMWARE_SETTINGS, Values::DISABLED);
        _firmwareSettingsOverrideEnable = false;
    }

    if (!_firmwareSettingsOverrideEnable)
        return;

    if (const char* value = get_variable(Keys::LANGUAGE); !string_is_empty(value)) {
        if (string_is_equal(value, Values::AUTO))
            _language = get_firmware_language(retro::get_language());
        else if (string_is_equal(value, Values::JAPANESE))
            _language = FirmwareLanguage::Japanese;
        else if (string_is_equal(value, Values::ENGLISH))
            _language = FirmwareLanguage::English;
        else if (string_is_equal(value, Values::FRENCH))
            _language = FirmwareLanguage::French;
        else if (string_is_equal(value, Values::GERMAN))
            _language = FirmwareLanguage::German;
        else if (string_is_equal(value, Values::ITALIAN))
            _language = FirmwareLanguage::Italian;
        else if (string_is_equal(value, Values::SPANISH))
            _language = FirmwareLanguage::Spanish;
    } else {
        retro::warn("Failed to get value for %s; defaulting to English", Keys::LANGUAGE);
        _language = FirmwareLanguage::English;
    }

    if (const char* value = get_variable(Keys::FAVORITE_COLOR); !string_is_empty(value)) {
        _favoriteColor = static_cast<Color>(std::clamp(std::stoi(value), 0, 15));
        // TODO: Warn if invalid
    } else {
        retro::warn("Failed to get value for %s; defaulting to gray", Keys::FAVORITE_COLOR);
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
    using namespace Config::Retro;
    using namespace melonds::config::audio;
    using retro::get_variable;

    if (const char* value = get_variable(Keys::MIC_INPUT_BUTTON); !string_is_empty(value)) {
        if (string_is_equal(value, Values::HOLD)) {
            _micButtonMode = MicButtonMode::Hold;
        } else if (string_is_equal(value, Values::TOGGLE)) {
            _micButtonMode = MicButtonMode::Toggle;
        } else if (string_is_equal(value, Values::ALWAYS)) {
            _micButtonMode = MicButtonMode::Always;
        } else {
            _micButtonMode = MicButtonMode::Hold;
        }
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::MIC_INPUT_BUTTON, Values::HOLD);
        _micButtonMode = MicButtonMode::Hold;
    }

    // TODO: Support loading WAV files from the system directory (list them and add modify the config object)
    if (const char* value = get_variable(Keys::MIC_INPUT); !string_is_empty(value)) {
        if (string_is_equal(value, Values::MICROPHONE))
            _micInputMode = MicInputMode::HostMic;
        else if (string_is_equal(value, Values::BLOW))
            _micInputMode = MicInputMode::BlowNoise;
        else if (string_is_equal(value, Values::NOISE))
            _micInputMode = MicInputMode::WhiteNoise;
        else
            _micInputMode = MicInputMode::None;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::MIC_INPUT, Values::SILENCE);
        _micInputMode = MicInputMode::None;
    }

    if (const char* value = get_variable(Keys::AUDIO_BITDEPTH); !string_is_empty(value)) {
        if (string_is_equal(value, Values::_10BIT))
            _bitDepth = BitDepth::_10Bit;
        else if (string_is_equal(value, Values::_16BIT))
            _bitDepth = BitDepth::_16Bit;
        else
            _bitDepth = BitDepth::Auto;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::AUDIO_BITDEPTH, Values::AUTO);
        _bitDepth = BitDepth::Auto;
    }

    if (const char* value = get_variable(Keys::AUDIO_INTERPOLATION); !string_is_empty(value)) {
        if (string_is_equal(value, Values::CUBIC))
            _interpolation = AudioInterpolation::Cubic;
        else if (string_is_equal(value, Values::COSINE))
            _interpolation = AudioInterpolation::Cosine;
        else if (string_is_equal(value, Values::LINEAR))
            _interpolation = AudioInterpolation::Linear;
        else
            _interpolation = AudioInterpolation::None;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::AUDIO_INTERPOLATION, Values::DISABLED);
        _interpolation = AudioInterpolation::None;
    }
}

static bool melonds::config::parse_video_options(bool initializing) noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::video;
    using retro::get_variable;

    bool needsOpenGlRefresh = false;

#ifdef HAVE_THREADS
    if (const char* value = get_variable(Keys::THREADED_RENDERER); !string_is_empty(value)) {
        // Only relevant for software-rendered 3D, so no OpenGL state reset needed
        _renderSettings.Soft_Threaded = string_is_equal(value, Values::ENABLED);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::THREADED_RENDERER, Values::ENABLED);
        _renderSettings.Soft_Threaded = true;
    }
#endif

#ifdef HAVE_OPENGL
    // TODO: Fix the OpenGL software only render impl so you can switch at runtime
    if (initializing) {
        // Can't change the renderer mid-game
        if (optional<Renderer> renderer = ParseRenderer(get_variable(Keys::RENDER_MODE))) {
            _configuredRenderer = *renderer;
        } else {
            retro::warn("Failed to get value for %s; defaulting to %s", Keys::RENDER_MODE, Values::SOFTWARE);
            _configuredRenderer = Renderer::Software;
        }
    }

    if (const char* value = get_variable(Keys::OPENGL_RESOLUTION); !string_is_empty(value)) {
        int newScaleFactor = std::clamp(atoi(value), 1, 8);

        if (_renderSettings.GL_ScaleFactor != newScaleFactor)
            needsOpenGlRefresh = true;

        _renderSettings.GL_ScaleFactor = newScaleFactor;
    } else {
        retro::warn("Failed to get value for %s; defaulting to 1", Keys::OPENGL_RESOLUTION);
        _renderSettings.GL_ScaleFactor = 1;
    }

    if (const char* value = get_variable(Keys::OPENGL_BETTER_POLYGONS); !string_is_empty(value)) {
        bool enabled = string_is_equal(value, Values::ENABLED);

        if (_renderSettings.GL_BetterPolygons != enabled)
            needsOpenGlRefresh = true;

        _renderSettings.GL_BetterPolygons = enabled;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::OPENGL_BETTER_POLYGONS, Values::DISABLED);
        _renderSettings.GL_BetterPolygons = false;
    }

    if (const char* value = get_variable(Keys::OPENGL_FILTERING); !string_is_empty(value)) {
        if (string_is_equal(value, Values::LINEAR))
            _screenFilter = ScreenFilter::Linear;
        else
            _screenFilter = ScreenFilter::Nearest;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::OPENGL_FILTERING, Values::NEAREST);
        _screenFilter = ScreenFilter::Nearest;
    }
#endif

    return needsOpenGlRefresh;
}

static bool melonds::config::parse_screen_options() noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::screen;
    using retro::get_variable;

    bool needsOpenGlRefresh = false;

    enum ScreenLayout oldScreenLayout = _screenLayout;
    if (optional<melonds::ScreenLayout> value = ParseScreenLayout(get_variable(Keys::SCREEN_LAYOUT))) {
        _screenLayout = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::SCREEN_LAYOUT, Values::TOP_BOTTOM);
        _screenLayout = ScreenLayout::TopBottom;
    }

    if (oldScreenLayout != _screenLayout)
        needsOpenGlRefresh = true;

    if (const char* value = get_variable(Keys::SCREEN_GAP); !string_is_empty(value)) {
        _screenGap = std::stoi(value); // TODO: Handle errors
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", Keys::SCREEN_GAP, 0);
        _screenGap = 0;
    }

#ifdef HAVE_OPENGL
    if (const char* value = get_variable(Keys::HYBRID_RATIO); !string_is_empty(value)) {
        _hybridRatio = std::stoi(value); // TODO: Handle errors
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", Keys::HYBRID_RATIO, 2);
        _hybridRatio = 2;
    }
#endif

    enum SmallScreenLayout oldHybridSmallScreen = _smallScreenLayout;
    if (const char* value = get_variable(Keys::HYBRID_SMALL_SCREEN); !string_is_empty(value)) {
        if (string_is_equal(value, Values::TOP))
            _smallScreenLayout = SmallScreenLayout::SmallScreenTop;
        else if (string_is_equal(value, Values::BOTTOM))
            _smallScreenLayout = SmallScreenLayout::SmallScreenBottom;
        else
            _smallScreenLayout = SmallScreenLayout::SmallScreenDuplicate;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HYBRID_SMALL_SCREEN, Values::BOTH);
        _smallScreenLayout = SmallScreenLayout::SmallScreenDuplicate;
    }

#ifdef HAVE_OPENGL
    if (_smallScreenLayout != oldHybridSmallScreen) {
        needsOpenGlRefresh = true;
    }
#endif

    enum TouchMode oldTouchMode = _touchMode;
    if (const char* value = get_variable(Keys::TOUCH_MODE); !string_is_empty(value)) {
        if (string_is_equal(value, Values::MOUSE))
            _touchMode = TouchMode::Mouse;
        else if (string_is_equal(value, Values::TOUCH))
            _touchMode = TouchMode::Touch;
        else if (string_is_equal(value, Values::JOYSTICK))
            _touchMode = TouchMode::Joystick;
        else
            _touchMode = TouchMode::Disabled;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::TOUCH_MODE, Values::MOUSE);
        _touchMode = TouchMode::Mouse;
    }

#ifdef HAVE_OPENGL
    if (_touchMode != oldTouchMode) {
        needsOpenGlRefresh = true;
    }
#endif

    if (const char* value = get_variable(Keys::SWAPSCREEN_MODE); !string_is_empty(value)) {
        if (string_is_equal(value, Values::TOGGLE))
            _screenSwapMode = ScreenSwapMode::Toggle;
        else
            _screenSwapMode = ScreenSwapMode::Hold;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::SWAPSCREEN_MODE, Values::TOGGLE);
        _screenSwapMode = ScreenSwapMode::Toggle;
    }

    return needsOpenGlRefresh;
}

/**
 * Reads the frontend's saved homebrew save data options and applies them to the emulator.
 */
static void melonds::config::parse_homebrew_save_options(const optional<struct retro_game_info>& nds_info,
                                                         const optional<NDSHeader>& header) noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::save;
    using retro::get_variable;

    if (!nds_info || !header || !header->IsHomebrew()) {
        // If no game is loaded, or if a non-homebrew game is loaded...
        _dldiSdCardMode = SdCardMode::None;
        retro::debug("Not parsing homebrew save options, as no homebrew game is loaded");
        return;
    }

    if (const char* value = get_variable(Keys::HOMEBREW_READ_ONLY); !string_is_empty(value)) {
        _dldiReadOnly = string_is_equal(value, Values::ENABLED);
    } else {
        _dldiReadOnly = false;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_READ_ONLY, Values::DISABLED);
    }

    if (const char* value = get_variable(Keys::HOMEBREW_SYNC_TO_HOST); !string_is_empty(value)) {
        _dldiFolderSync = string_is_equal(value, Values::ENABLED);
    } else {
        _dldiFolderSync = true;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_SYNC_TO_HOST, Values::ENABLED);
    }

    const optional<string>& save_directory = retro::get_save_directory();
    if (const char* value = get_variable(Keys::HOMEBREW_SAVE_MODE); save_directory && !string_is_empty(value)) {

        auto set_config = [&save_directory](int size, const char* name) {
            char dldi_path[PATH_MAX];
            memset(dldi_path, 0, sizeof(dldi_path));
            fill_pathname_join_special(dldi_path, save_directory->c_str(), name, sizeof(dldi_path));

            _dldiFolderPath = dldi_path;

            strlcat(dldi_path, ".dldi", sizeof(dldi_path));

            _dldiImagePath = dldi_path;
            // If the SD card image exists, set the DLDISize to auto; else set it to the given card size
            _dldiImageSize = path_is_valid(_dldiImagePath.c_str()) ? 0 : size;
        };

        if (string_is_equal(value, Values::SHARED)) {
            _dldiSdCardMode = SdCardMode::Shared;
            set_config(4096, Values::SHARED4G);
        } else if (string_is_equal(value, Values::DEDICATED)) {
            _dldiSdCardMode = SdCardMode::Dedicated;
            char game_name[PATH_MAX];
            GetGameName(*nds_info, game_name, sizeof(game_name));
            set_config(4096, game_name);
        } else {
            _dldiSdCardMode = SdCardMode::None;
            _dldiImagePath = "";
            _dldiImageSize = 0;
            _dldiFolderPath = "";
        }
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_SAVE_MODE, Values::DISABLED);
        _dldiSdCardMode = SdCardMode::None;
        _dldiImagePath = "";
        _dldiImageSize = 0;
        _dldiFolderPath = "";
    }
}

/**
 * Reads the frontend's saved DSi save data options and applies them to the emulator.
 */
static void melonds::config::parse_dsi_sd_options(const optional<struct retro_game_info>& nds_info,
                                                  const optional<NDSHeader>& header) noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::save;
    using retro::get_variable;

    if (!nds_info || !header || !header->IsDSiWare()) {
        // If no game is loaded, or if a non-DSiWare game is loaded...
        _dsiSdCardMode = SdCardMode::None;
        return;
    }

    if (const char* value = get_variable(Keys::DSI_SD_READ_ONLY); !string_is_empty(value)) {
        _dsiSdReadOnly = string_is_equal(value, Values::ENABLED);
    } else {
        _dsiSdReadOnly = false;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::DSI_SD_READ_ONLY, Values::DISABLED);
    }

    if (const char* value = get_variable(Keys::DSI_SD_SYNC_TO_HOST); !string_is_empty(value)) {
        _dsiSdFolderSync = string_is_equal(value, Values::ENABLED);
    } else {
        _dsiSdFolderSync = true;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::DSI_SD_SYNC_TO_HOST, Values::ENABLED);
    }

    const optional<string>& save_directory = retro::get_save_directory();
    if (const char* value = get_variable(Keys::DSI_SD_SAVE_MODE); save_directory && !string_is_empty(value)) {

        auto set_config = [&save_directory](int size, const char* name) {
            char dsisd_path[PATH_MAX];
            memset(dsisd_path, 0, sizeof(dsisd_path));
            fill_pathname_join_special(dsisd_path, save_directory->c_str(), name, sizeof(dsisd_path));

            _dsiSdFolderPath = dsisd_path;

            strlcat(dsisd_path, ".dsisd", sizeof(dsisd_path));

            _dsiSdImagePath = dsisd_path;
            // If the SD card image exists, set the image size to auto; else set it to the given card size
            _dsiSdImageSize = path_is_valid(_dsiSdImagePath.c_str()) ? 0 : size;
        };

        if (string_is_equal(value, Values::SHARED)) {
            _dsiSdCardMode = SdCardMode::Shared;
            set_config(4096, Values::SHARED4GDSI);
        } else if (string_is_equal(value, Values::DEDICATED)) {
            _dsiSdCardMode = SdCardMode::Dedicated;
            char game_name[PATH_MAX];
            GetGameName(*nds_info, game_name, sizeof(game_name));
            set_config(4096, game_name);
        } else {
            _dsiSdCardMode = SdCardMode::None;
            _dsiSdImagePath = "";
            _dsiSdImageSize = 0;
            _dsiSdFolderPath = "";
        }
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::DSI_SD_SAVE_MODE, Values::DISABLED);
        _dsiSdCardMode = SdCardMode::None;
        _dsiSdImagePath = "";
        _dsiSdImageSize = 0;
        _dsiSdFolderPath = "";
    }
}

static void melonds::config::verify_nds_bios(bool ds_game_loaded) {
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

    // TODO: Set BIOS paths here
}

static void melonds::config::apply_audio_options() noexcept {
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
    using namespace config::save;

    if (header && header->IsHomebrew() && DldiEnable() && DldiFolderSync()) {
        // If we're loading a homebrew game and we want to sync its SD card image to the host...
        string path = DldiFolderPath();
        if (path_mkdir(path.c_str())) {
            // If we successfully created the save directory...

            retro::info("Created (or using existing) homebrew save directory \"%s\"", path.c_str());
        } else {
            throw emulator_exception("Failed to create homebrew save directory at " + path);
        }
    }

    if (system::ConsoleType() == ConsoleType::DSi && DsiSdEnable() && DsiSdFolderSync()) {
        // If we're running in DSi mode and we want to sync its SD card image to the host...
        string path = DsiSdFolderPath();
        if (path_mkdir(path.c_str())) {
            // If we successfully created the save directory...

            retro::info("Created (or using existing) DSi save directory \"%s\"", path.c_str());
        } else {
            throw emulator_exception("Failed to create DSi save directory at " + path);
        }
    }
}

static void melonds::config::apply_screen_options() noexcept {
    screen_layout_data.Layout(screen::ScreenLayout());
    screen_layout_data.HybridSmallScreenLayout(screen::SmallScreenLayout());
    screen_layout_data.ScreenGap(screen::ScreenGap());
    screen_layout_data.HybridRatio(screen::HybridRatio());
    screen_layout_data.Update(video::ConfiguredRenderer());
}

struct retro_core_option_v2_category option_cats_us[] = {
    {
        "system",
        "System",
        "Change system settings."
    },
    {
        "video",
        "Video",
        "Change video settings."
    },
    {
        "audio",
        "Audio",
        "Change audio settings."
    },
    {
        Config::Retro::Category::SCREEN,
        "Screen",
        "Change screen settings."
    },
#ifdef JIT_ENABLED
    {
        "cpu",
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
        Config::Retro::Keys::CONSOLE_MODE,
        "Console Type",
        nullptr,
        "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. "
        "Some features may not be available in DSi mode. "
        "DSi mode will be used if loading a DSiWare application.",
        nullptr,
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DS, "DS"},
            {Config::Retro::Values::DSI, "DSi (experimental)"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DS
    },
    {
        Config::Retro::Keys::BOOT_DIRECTLY,
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
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::ENABLED
    },
    {
        Config::Retro::Keys::OVERRIDE_FIRMWARE_SETTINGS,
        "Override Firmware Settings",
        nullptr,
        "Use language and username specified in the frontend, "
        "rather than those provided by the firmware itself. "
        "If disabled or the firmware is unavailable, these values will be provided by the frontend. "
        "If a name couldn't be found, \"melonDS\" will be used as the default.",
        nullptr,
        "system",
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
    {
        Config::Retro::Keys::LANGUAGE,
        "Language",
        nullptr,
        "The language mode of the emulated console. "
        "Not every game honors this setting. "
        "Automatic uses the frontend's language if supported by the DS, or English if not.",
        nullptr,
        "system",
        {
            {Config::Retro::Values::AUTO, "Automatic"},
            {Config::Retro::Values::ENGLISH, "English"},
            {Config::Retro::Values::JAPANESE, "Japanese"},
            {Config::Retro::Values::FRENCH, "French"},
            {Config::Retro::Values::GERMAN, "German"},
            {Config::Retro::Values::ITALIAN, "Italian"},
            {Config::Retro::Values::SPANISH, "Spanish"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DEFAULT
    },
    {
        Config::Retro::Keys::FAVORITE_COLOR,
        "Favorite Color",
        nullptr,
        "The theme (\"favorite color\") of the emulated console.",
        nullptr,
        "system",
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
        Config::Retro::Keys::USE_EXTERNAL_BIOS,
        "Use external BIOS if available",
        nullptr,
        "If enabled, melonDS will attempt to load a BIOS file from the system directory. "
        "If no valid BIOS is present, melonDS will fall back to its built-in FreeBIOS. "
        "Note that GBA connectivity requires a native BIOS. "
        "Takes effect at the next restart. "
        "If unsure, leave this enabled.",
        nullptr,
        "system",
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::ENABLED
    },

    // DSi
    {
        Config::Retro::Keys::DSI_SD_SAVE_MODE,
        "Virtual SD Card (DSi)",
        nullptr,
        "Select a virtual SD card that will be used by the emulated DSi.\n"
        "\n"
        "Disabled: The DSi won't see an SD card. Data won't be saved.\n"
        "Dedicated: The DSi will use an SD card specific to this game.\n"
        "Shared: The game shares an SD card with other games.\n"
        "\n"
        "The virtual SD card is 4GB, but the image file is dynamically sized "
        "(i.e. it will only use the disk space it needs). "
        "Ignored when not in DSi mode. "
        "Changing this setting does not transfer existing data. "
        "Changes take effect with next restart.",
        nullptr,
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DISABLED, "Disabled"},
            {Config::Retro::Values::SHARED, "Shared"},
            {Config::Retro::Values::DEDICATED, "Dedicated"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::SHARED4G
    },
    {
        Config::Retro::Keys::DSI_SD_READ_ONLY,
        "Read-Only Mode (DSi)",
        nullptr,
        "If enabled, the emulated DSi sees the virtual SD card as read-only. "
        "Changes take effect with next restart.",
        nullptr,
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
    {
        Config::Retro::Keys::DSI_SD_SYNC_TO_HOST,
        "Sync SD Card to Host (DSi)",
        nullptr,
        "If enabled, the virtual SD card's files will be synced to this core's save directory. "
        "Enable this if you want to add files to the virtual SD card from outside the core. "
        "Syncing happens when loading and unloading a game, "
        "so external changes won't have any effect while the core is running. "
        "Takes effect at the next boot. "
        "Adjusting this setting may overwrite existing save data.",
        nullptr,
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },

    // Video
#ifdef HAVE_THREADS
    {
        Config::Retro::Keys::THREADED_RENDERER,
        "Threaded Software Renderer",
        nullptr,
        "If enabled, the software renderer will run on a separate thread if possible. "
        "Otherwise, it will run on the main thread. "
        "Ignored if using the OpenGL renderer ."
        "Takes effect next time the core restarts. ",
        nullptr,
        Config::Retro::Category::VIDEO,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
#endif
#ifdef HAVE_OPENGL
    {
        Config::Retro::Keys::RENDER_MODE,
        "Render Mode",
        nullptr,
        "OpenGL mode uses OpenGL (or OpenGL ES) for rendering graphics. "
        "If that doesn't work, software rendering is used as a fallback. "
        "Changes take effect next time the core restarts. ",
        nullptr,
        Config::Retro::Category::VIDEO,
        {
            {Config::Retro::Values::SOFTWARE, "Software"},
            {Config::Retro::Values::OPENGL, "OpenGL"},
            {nullptr, nullptr},
        },
        "software"
    },
    {
        Config::Retro::Keys::OPENGL_RESOLUTION,
        "OpenGL Internal Resolution",
        nullptr,
        nullptr,
        nullptr,

        Config::Retro::Category::VIDEO,
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
        Config::Retro::Keys::OPENGL_BETTER_POLYGONS,
        "OpenGL Improved Polygon Splitting",
        nullptr,
        nullptr,
        nullptr,
        "video",
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
    {
        Config::Retro::Keys::OPENGL_FILTERING,
        "OpenGL Filtering",
        nullptr,
        nullptr,
        nullptr,
        Config::Retro::Category::VIDEO,
        {
            {"nearest", "Nearest"},
            {"linear", "Linear"},
            {nullptr, nullptr},
        },
        "nearest"
    },
#endif

    // Audio Settings
    {
        Config::Retro::Keys::MIC_INPUT,
        "Microphone Input Mode",
        nullptr,
        "Choose the sound that the emulated microphone will receive:\n"
        "\n"
        "Silence: No audio input.\n"
        "Blow: Loop a built-in blowing sound.\n"
        "Noise: Random white noise.\n"
        "Microphone: Use your real microphone if available, fall back to Silence if not.",
        nullptr,
        Config::Retro::Category::AUDIO,
        {
            {Config::Retro::Values::SILENCE, "Silence"},
            {Config::Retro::Values::BLOW, "Blow"},
            {Config::Retro::Values::NOISE, "Noise"},
            {Config::Retro::Values::MICROPHONE, "Microphone"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::MICROPHONE
    },
    {
        Config::Retro::Keys::MIC_INPUT_BUTTON,
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
        Config::Retro::Category::AUDIO,
        {
            {Config::Retro::Values::HOLD, "Hold"},
            {Config::Retro::Values::TOGGLE, "Toggle"},
            {Config::Retro::Values::ALWAYS, "Always"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::HOLD
    },
    {
        Config::Retro::Keys::AUDIO_BITDEPTH,
        "Audio Bit Depth",
        nullptr,
        "The audio playback bit depth. "
        "Automatic uses 10-bit audio for DS mode "
        "and 16-bit audio for DSi mode.\n"
        "\n"
        "Takes effect at next restart. "
        "If unsure, leave this set to Automatic.",
        nullptr,
        Config::Retro::Category::AUDIO,
        {
            {Config::Retro::Values::AUTO, "Automatic"},
            {Config::Retro::Values::_10BIT, "10-bit"},
            {Config::Retro::Values::_16BIT, "16-bit"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::AUTO
    },
    {
        Config::Retro::Keys::AUDIO_INTERPOLATION,
        "Audio Interpolation",
        nullptr,
        "Interpolates audio output for improved quality. "
        "Disable this to match the behavior of the original DS hardware.",
        nullptr,
        Config::Retro::Category::AUDIO,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::LINEAR, "Linear"},
            {Config::Retro::Values::COSINE, "Cosine"},
            {Config::Retro::Values::CUBIC, "Cubic"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },

    {
        Config::Retro::Keys::TOUCH_MODE,
        "Touch Mode",
        nullptr,
        "Choose mode for interactions with the touch screen.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {Config::Retro::Values::MOUSE, "Mouse"},
            {Config::Retro::Values::TOUCH, "Touch"},
            {Config::Retro::Values::JOYSTICK, "Joystick"},
            {Config::Retro::Values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::MOUSE
    },
    {
        Config::Retro::Keys::SWAPSCREEN_MODE,
        "Swap Screen Mode",
        nullptr,
        "Choose if the 'Swap screens' button should work on press or on hold.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {Config::Retro::Values::TOGGLE, "Toggle"},
            {Config::Retro::Values::HOLD, "Hold"},
            {nullptr, nullptr},
        },
        "Toggle"
    },
    {
        Config::Retro::Keys::SCREEN_LAYOUT,
        "Screen Layout",
        nullptr,
        "Choose how many screens should be displayed and how they should be displayed.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {Config::Retro::Values::TOP_BOTTOM, "Top/Bottom"},
            {Config::Retro::Values::BOTTOM_TOP, "Bottom/Top"},
            {Config::Retro::Values::LEFT_RIGHT, "Left/Right"},
            {Config::Retro::Values::RIGHT_LEFT, "Right/Left"},
            {Config::Retro::Values::TOP, "Top Only"},
            {Config::Retro::Values::BOTTOM, "Bottom Only"},
            {Config::Retro::Values::HYBRID_TOP, "Hybrid (Focus Top)"},
            {Config::Retro::Values::HYBRID_BOTTOM, "Hybrid (Focus Bottom)"},
            {Config::Retro::Values::ROTATE_LEFT, "Rotated Left"},
            {Config::Retro::Values::ROTATE_RIGHT, "Rotated Right"},
            {Config::Retro::Values::UPSIDE_DOWN, "Upside Down"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::TOP_BOTTOM
    },
    {
        Config::Retro::Keys::SCREEN_GAP,
        "Screen Gap",
        nullptr,
        "Choose how large the gap between the 2 screens should be.",
        nullptr,
        Config::Retro::Category::SCREEN,
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
        Config::Retro::Keys::HYBRID_SMALL_SCREEN,
        "Hybrid Small Screen Mode",
        nullptr,
        "Choose the position of the small screen when using a 'hybrid' mode, or if it should show both screens.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {Config::Retro::Values::BOTTOM, "Bottom"},
            {Config::Retro::Values::TOP, "Top"},
            {Config::Retro::Values::BOTH, "Both"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::BOTTOM
    },

    // Homebrew Save Data
    {
        Config::Retro::Keys::HOMEBREW_SAVE_MODE,
        "Virtual SD Card",
        nullptr,
        "Select a virtual SD card that will be used by homebrew games.\n"
        "\n"
        "Disabled: The game won't see an SD card. Data won't be saved.\n"
        "Dedicated: The game uses its own SD card.\n"
        "Shared: The game shares an SD card with other games.\n"
        "\n"
        "The virtual SD card is 4GB, but the image file is dynamically sized "
        "(i.e. it will only use the disk space it needs). "
        "Ignored for retail games. "
        "Changing this setting does not transfer existing data. "
        "Changes take effect with next restart.",
        nullptr,
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DISABLED, "Disabled"},
            {Config::Retro::Values::SHARED, "Shared"},
            {Config::Retro::Values::DEDICATED, "Dedicated"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::SHARED
    },
    {
        Config::Retro::Keys::HOMEBREW_READ_ONLY,
        "Read-Only Mode",
        nullptr,
        "If enabled, homebrew applications will see the virtual SD card as read-only. "
        "Changes take effect with next restart.",
        nullptr,
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
    {
        Config::Retro::Keys::HOMEBREW_SYNC_TO_HOST,
        "Sync SD Card to Host",
        nullptr,
        "If enabled, the virtual SD card's files will be synced to this core's save directory. "
        "Enable this if you want to add files to the virtual SD card from outside the core. "
        "Syncing happens when loading and unloading a game, "
        "so external changes won't have any effect while the core is running. "
        "Takes effect at the next boot. "
        "Adjusting this setting may overwrite existing save data.",
        nullptr,
        Config::Retro::Category::SYSTEM,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
#ifdef HAVE_OPENGL
    {
        Config::Retro::Keys::HYBRID_RATIO,
        "Hybrid Ratio (OpenGL Only)",
        nullptr,
        nullptr,
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {"2", "2x"},
            {"3", "3x"},
            {nullptr, nullptr},
        },
        "2"
    },
#endif
#ifdef JIT_ENABLED
    {
        Config::Retro::Keys::JIT_ENABLE,
        "JIT Enable (Restart)",
        nullptr,
        "Recompiles emulated machine code as it runs. "
        "Restart required to take effect. "
        "If unsure, leave enabled.",
        nullptr,
        "cpu",
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::ENABLED
    },
    {
        Config::Retro::Keys::JIT_BLOCK_SIZE,
        "JIT Block Size",
        nullptr,
        nullptr,
        nullptr,
        "cpu",
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
        Config::Retro::Keys::JIT_BRANCH_OPTIMISATIONS,
        "JIT Branch Optimisations",
        nullptr,
        nullptr,
        nullptr,
        "cpu",
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::ENABLED
    },
    {
        Config::Retro::Keys::JIT_LITERAL_OPTIMISATIONS,
        "JIT Literal Optimisations",
        nullptr,
        nullptr,
        nullptr,
        "cpu",
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::ENABLED
    },
#ifdef HAVE_JIT_FASTMEM
    {
        Config::Retro::Keys::JIT_FAST_MEMORY,
        "JIT Fast Memory",
        nullptr,
        nullptr,
        nullptr,
        "cpu",
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::ENABLED
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