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

#include <charconv>
#include <cstring>
#include <initializer_list>
#include <system_error>
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

using std::from_chars;
using std::from_chars_result;
using std::initializer_list;
using std::string;
using std::nullopt;
using std::optional;

constexpr unsigned DS_NAME_LIMIT = 10;
constexpr unsigned AUTO_SDCARD_SIZE = 0;
constexpr unsigned DEFAULT_SDCARD_SIZE = 4096;
const char* const DEFAULT_HOMEBREW_SDCARD_IMAGE_NAME = "dldi_sd_card.bin";
const char* const DEFAULT_HOMEBREW_SDCARD_DIR_NAME = "dldi_sd_card";
const char* const DEFAULT_DSI_SDCARD_IMAGE_NAME = "dsi_sd_card.bin";
const char* const DEFAULT_DSI_SDCARD_DIR_NAME = "dsi_sd_card";

const initializer_list<unsigned> SCREEN_GAP_LENGTHS = {0, 1, 2, 8, 16, 24, 32, 48, 64, 72, 88, 90, 128};
const initializer_list<unsigned> CURSOR_TIMEOUTS = {1, 2, 3, 5, 10, 15, 20, 30, 60};

namespace Config {
    namespace Retro {
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
            static const char* const NUMBER_OF_SCREEN_LAYOUTS = "melonds_number_of_screen_layouts";
            static const char* const SCREEN_LAYOUT1 = "melonds_screen_layout1";
            static const char* const SCREEN_LAYOUT2 = "melonds_screen_layout2";
            static const char* const SCREEN_LAYOUT3 = "melonds_screen_layout3";
            static const char* const SCREEN_LAYOUT4 = "melonds_screen_layout4";
            static const char* const SCREEN_LAYOUT5 = "melonds_screen_layout5";
            static const char* const SCREEN_LAYOUT6 = "melonds_screen_layout6";
            static const char* const SCREEN_LAYOUT7 = "melonds_screen_layout7";
            static const char* const SCREEN_LAYOUT8 = "melonds_screen_layout8";
            static const std::array<const char* const, melonds::config::screen::MAX_SCREEN_LAYOUTS> SCREEN_LAYOUTS = {
                SCREEN_LAYOUT1,
                SCREEN_LAYOUT2,
                SCREEN_LAYOUT3,
                SCREEN_LAYOUT4,
                SCREEN_LAYOUT5,
                SCREEN_LAYOUT6,
                SCREEN_LAYOUT7,
                SCREEN_LAYOUT8
            };

            static const char* const SHOW_CURSOR = "melonds_show_cursor";
            static const char* const CURSOR_TIMEOUT = "melonds_cursor_timeout";
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
            static const char* const ONE = "one";
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
            static const char* const TIMEOUT = "timeout";
            static const char* const TOGGLE = "toggle";
            static const char* const TOP_BOTTOM = "top-bottom";
            static const char* const TOP = "top";
            static const char* const TOUCH = "touch";
            static const char* const TOUCHING = "touching";
            static const char* const UPSIDE_DOWN = "rotate-180";
        }
    }
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

    static optional<Renderer> ParseRenderer(const char* value) noexcept;
    static optional<CursorMode> ParseCursorMode(const char* value) noexcept;

    static void parse_jit_options() noexcept;
    static void parse_system_options() noexcept;
    static void parse_firmware_options() noexcept;
    static void parse_audio_options() noexcept;

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

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
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

static optional<melonds::CursorMode> melonds::config::ParseCursorMode(const char* value) noexcept {
    if (string_is_equal(value, Config::Retro::Values::DISABLED)) return melonds::CursorMode::Never;
    if (string_is_equal(value, Config::Retro::Values::TOUCHING)) return melonds::CursorMode::Touching;
    if (string_is_equal(value, Config::Retro::Values::TIMEOUT)) return melonds::CursorMode::Timeout;
    if (string_is_equal(value, Config::Retro::Values::ALWAYS)) return melonds::CursorMode::Always;
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

template<typename T>
static optional<T> ParseIntegerInRange(const char* value, T min, T max) noexcept {
    if (min > max) return nullopt;
    if (!value) return nullopt;

    T parsed_number = 0;
    from_chars_result result = from_chars(value, value + strlen(value), parsed_number);

    if (result.ec != std::errc()) return nullopt;
    if (parsed_number < min || parsed_number > max) return nullopt;

    return parsed_number;
}

template<typename T>
static optional<T> ParseIntegerInList(const char* value, const initializer_list<T>& list) noexcept {
    if (!value) return nullopt;

    T parsed_number = 0;
    from_chars_result result = from_chars(value, value + strlen(value), parsed_number);

    if (result.ec != std::errc()) return nullopt;
    for (T t : list) {
        if (parsed_number == t) return parsed_number;
    }

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

static optional<melonds::HybridSideScreenDisplay> ParseHybridSideScreenDisplay(const char* value) noexcept {
    using melonds::ScreenLayout;
    using namespace Config::Retro;
    if (string_is_equal(value, Values::ONE)) return melonds::HybridSideScreenDisplay::One;
    if (string_is_equal(value, Values::BOTH)) return melonds::HybridSideScreenDisplay::Both;

    return nullopt;
}

void melonds::InitConfig(const optional<struct retro_game_info>& nds_info, const optional<NDSHeader>& header,
                         ScreenLayoutData& screenLayout, InputState& inputState) {
    config::parse_system_options();
    config::parse_jit_options();
    config::parse_homebrew_save_options(nds_info, header);
    config::parse_dsi_sd_options();
    config::parse_firmware_options();
    config::parse_audio_options();
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
    using namespace Config::Retro;
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

    bool oldShowCursorTimeout = ShowCursorTimeout;
    optional<melonds::CursorMode> cursorMode = ParseCursorMode(get_variable(Keys::SHOW_CURSOR));

    ShowCursorTimeout = !cursorMode || *cursorMode == melonds::CursorMode::Timeout;
    if (ShowCursorTimeout != oldShowCursorTimeout) {
        set_option_visible(Keys::CURSOR_TIMEOUT, ShowCursorTimeout);

        updated = true;
    }

    unsigned oldNumberOfShownScreenLayouts = NumberOfShownScreenLayouts;
    optional<unsigned> numberOfScreenLayouts = ParseIntegerInRange(get_variable(Keys::NUMBER_OF_SCREEN_LAYOUTS), 1u, screen::MAX_SCREEN_LAYOUTS);

    NumberOfShownScreenLayouts = numberOfScreenLayouts ? *numberOfScreenLayouts : screen::MAX_SCREEN_LAYOUTS;
    if (NumberOfShownScreenLayouts != oldNumberOfShownScreenLayouts) {
        for (unsigned i = 0; i < screen::MAX_SCREEN_LAYOUTS; ++i) {
            set_option_visible(Keys::SCREEN_LAYOUTS[i], i < NumberOfShownScreenLayouts);
        }

        updated = true;
    }

    // Show/hide Hybrid screen options
    bool oldShowHybridOptions = ShowHybridOptions;
    bool oldShowVerticalLayoutOptions = ShowVerticalLayoutOptions;
    bool anyHybridLayouts = false;
    bool anyVerticalLayouts = false;
    for (unsigned i = 0; i < NumberOfShownScreenLayouts; i++) {
        optional<melonds::ScreenLayout> parsedLayout = ParseScreenLayout(get_variable(Keys::SCREEN_LAYOUTS[i]));

        anyHybridLayouts |= !parsedLayout || IsHybridLayout(*parsedLayout);
        anyVerticalLayouts |= !parsedLayout || LayoutSupportsScreenGap(*parsedLayout);
    }
    ShowHybridOptions = anyHybridLayouts;
    ShowVerticalLayoutOptions = anyVerticalLayouts;

    if (ShowHybridOptions != oldShowHybridOptions) {
        set_option_visible(Keys::HYBRID_SMALL_SCREEN, ShowHybridOptions);
        set_option_visible(Keys::HYBRID_RATIO, ShowHybridOptions);

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

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
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

static void melonds::config::parse_screen_options() noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::screen;
    using retro::get_variable;

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(Keys::SCREEN_GAP), SCREEN_GAP_LENGTHS)) {
        _screenGap = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", Keys::SCREEN_GAP, 0);
        _screenGap = 0;
    }

    if (optional<unsigned> value = ParseIntegerInList<unsigned>(get_variable(Keys::CURSOR_TIMEOUT), CURSOR_TIMEOUTS)) {
        _cursorTimeout = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", Keys::CURSOR_TIMEOUT, 3);
        _cursorTimeout = 3;
    }

    if (optional<melonds::CursorMode> value = ParseCursorMode(get_variable(Keys::SHOW_CURSOR))) {
        _cursorMode = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::SHOW_CURSOR, Values::ALWAYS);
        _cursorMode = CursorMode::Always;
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(Keys::HYBRID_RATIO), 2, 3)) {
        _hybridRatio = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", Keys::HYBRID_RATIO, 2);
        _hybridRatio = 2;
    }

    if (optional<HybridSideScreenDisplay> value = ParseHybridSideScreenDisplay(get_variable(Keys::HYBRID_SMALL_SCREEN))) {
        _smallScreenLayout = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HYBRID_SMALL_SCREEN, Values::BOTH);
        _smallScreenLayout = HybridSideScreenDisplay::Both;
    }

    if (optional<unsigned> value = ParseIntegerInRange(get_variable(Keys::NUMBER_OF_SCREEN_LAYOUTS), 1u, MAX_SCREEN_LAYOUTS)) {
        _numberOfScreenLayouts = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %d", Keys::NUMBER_OF_SCREEN_LAYOUTS, 2);
        _numberOfScreenLayouts = 2;
    }

    for (unsigned i = 0; i < MAX_SCREEN_LAYOUTS; i++) {
        if (optional<melonds::ScreenLayout> value = ParseScreenLayout(get_variable(Keys::SCREEN_LAYOUTS[i]))) {
            _screenLayouts[i] = *value;
        } else {
            retro::warn("Failed to get value for %s; defaulting to %s", Keys::SCREEN_LAYOUTS[i], Values::TOP_BOTTOM);
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
    using namespace Config::Retro;
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

    if (optional<bool> value = ParseBoolean(get_variable(Keys::HOMEBREW_SAVE_MODE))) {
        _dldiEnable = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_SAVE_MODE, Values::ENABLED);
        _dldiEnable = true;
    }
}

/**
 * Reads the frontend's saved DSi save data options and applies them to the emulator.
 */
static void melonds::config::parse_dsi_sd_options() noexcept {
    using namespace Config::Retro;
    using namespace melonds::config::save;
    using retro::get_variable;

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

    if (optional<bool> value = ParseBoolean(get_variable(Keys::DSI_SD_SAVE_MODE))) {
        _dsiSdEnable = *value;
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::DSI_SD_SAVE_MODE, Values::ENABLED);
        _dsiSdEnable = true;
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
        "If enabled, a virtual SD card will be made available to the emulated DSi. "
        "The card image must be within the frontend's system directory and be named dsi_sd_card.bin. "
        "If no image exists, a 4GB virtual SD card will be created. "
        "Ignored when in DS mode. "
        "Changes take effect at next boot.",
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
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    {
        Config::Retro::Keys::RENDER_MODE,
        "Render Mode",
        nullptr,
        "OpenGL mode uses OpenGL for rendering graphics. "
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
        "Internal Resolution",
        nullptr,
        "The degree to which the emulated 3D engine's graphics are scaled up. "
        "Dimensions are given per screen. "
        "OpenGL renderer only.",
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
        "Improved Polygon Splitting",
        nullptr,
        "Enable this if your game's 3D models are not rendering correctly. "
        "OpenGL renderer only.",
        nullptr,
        Config::Retro::Category::VIDEO,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
    {
        Config::Retro::Keys::OPENGL_FILTERING,
        "Screen Filtering",
        nullptr,
        "Affects how the emulated screens are scaled to fit the real screen. "
        "Performance impact is minimal. "
        "OpenGL renderer only.\n"
        "\n"
        "Nearest: No filtering. Graphics look blocky.\n"
        "Linear: Smooth scaling.\n",
        nullptr,
        Config::Retro::Category::VIDEO,
        {
            {Config::Retro::Values::NEAREST, "Nearest"},
            {Config::Retro::Values::LINEAR, "Linear"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::NEAREST
    },
#endif
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
        Config::Retro::Keys::SHOW_CURSOR,
        "Cursor Mode",
        nullptr,
        "Determines when a cursor should appear on the bottom screen. "
        "Never is recommended for touch screens; "
        "the other settings are best suited for mouse or joystick input.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {Config::Retro::Values::DISABLED, "Never"},
            {Config::Retro::Values::TOUCHING, "While Touching"},
            {Config::Retro::Values::TIMEOUT, "Until Timeout"},
            {Config::Retro::Values::ALWAYS, "Always"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::ALWAYS
    },
    {
        Config::Retro::Keys::CURSOR_TIMEOUT,
        "Cursor Timeout",
        nullptr,
        "If Cursor Mode is set to \"Until Timeout\", "
        "then the cursor will be hidden if the pointer hasn't moved for a certain time.",
        nullptr,
        Config::Retro::Category::SCREEN,
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
        Config::Retro::Keys::HYBRID_RATIO,
        "Hybrid Ratio",
        nullptr,
        "The size of the larger screen relative to the smaller ones when using a hybrid layout.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {"2", "2:1"},
            {"3", "3:1"},
            {nullptr, nullptr},
        },
        "2"
    },
    {
        Config::Retro::Keys::HYBRID_SMALL_SCREEN,
        "Hybrid Small Screen Mode",
        nullptr,
        "Choose which screens will be shown when using a hybrid layout.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {Config::Retro::Values::ONE, "Show Opposite Screen"},
            {Config::Retro::Values::BOTH, "Show Both Screens"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::BOTH
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
        Config::Retro::Keys::NUMBER_OF_SCREEN_LAYOUTS,
        "# of Screen Layouts",
        nullptr,
        "The number of screen layouts to cycle through with the Next Layout button.",
        nullptr,
        Config::Retro::Category::SCREEN,
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
        Config::Retro::Keys::SCREEN_LAYOUT1,
        "Screen Layout #1",
        nullptr,
        nullptr,
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
        Config::Retro::Keys::SCREEN_LAYOUT2,
        "Screen Layout #2",
        nullptr,
        nullptr,
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
        Config::Retro::Values::LEFT_RIGHT
    },
    {
        Config::Retro::Keys::SCREEN_LAYOUT3,
        "Screen Layout #3",
        nullptr,
        nullptr,
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
        Config::Retro::Values::TOP
    },
    {
        Config::Retro::Keys::SCREEN_LAYOUT4,
        "Screen Layout #4",
        nullptr,
        nullptr,
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
        Config::Retro::Values::BOTTOM
    },
    {
        Config::Retro::Keys::SCREEN_LAYOUT5,
        "Screen Layout #5",
        nullptr,
        nullptr,
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
        Config::Retro::Values::HYBRID_TOP
    },
    {
        Config::Retro::Keys::SCREEN_LAYOUT6,
        "Screen Layout #6",
        nullptr,
        nullptr,
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
        Config::Retro::Values::HYBRID_BOTTOM
    },
    {
        Config::Retro::Keys::SCREEN_LAYOUT7,
        "Screen Layout #7",
        nullptr,
        nullptr,
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
        Config::Retro::Values::BOTTOM_TOP
    },
    {
        Config::Retro::Keys::SCREEN_LAYOUT8,
        "Screen Layout #8",
        nullptr,
        nullptr,
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
        Config::Retro::Values::RIGHT_LEFT
    },

    // Homebrew Save Data
    {
        Config::Retro::Keys::HOMEBREW_SAVE_MODE,
        "Virtual SD Card",
        nullptr,
        "If enabled, a virtual SD card will be made available to homebrew DS games. "
        "The card image must be within the frontend's system directory and be named dldi_sd_card.bin. "
        "If no image exists, a 4GB virtual SD card will be created. "
        "Ignored for retail games. "
        "Changes take effect at next boot.",
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