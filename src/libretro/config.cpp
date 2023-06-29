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
#include <frontend/qt_sdl/Config.h>
#include <GPU.h>
#include <string/stdstring.h>
#include <file/file_path.h>

#include "content.hpp"
#include "libretro.hpp"
#include "environment.hpp"
#include "screenlayout.hpp"
#include "input.hpp"
#include "opengl.hpp"
#include "microphone.hpp"

using std::string;
using std::optional;

namespace Config {
    bool ScreenSwap;
    bool ScreenFilter;

    bool Threaded3D;

    int GL_ScaleFactor;
    bool GL_BetterPolygons;

    int ConsoleType;
    [[maybe_unused]] bool DirectBoot;

#ifdef JIT_ENABLED
    [[maybe_unused]] bool JIT_Enable;
    [[maybe_unused]] int JIT_MaxBlockSize;
    [[maybe_unused]] bool JIT_BranchOptimisations;
    [[maybe_unused]] bool JIT_LiteralOptimisations;
    [[maybe_unused]] bool JIT_FastMemory;
#endif

    [[maybe_unused]] bool ExternalBIOSEnable = true;

    // These path values are intentionally fixed
    [[maybe_unused]] std::string BIOS9Path = "bios9.bin";
    [[maybe_unused]] std::string BIOS7Path = "bios7.bin";
    [[maybe_unused]] std::string FirmwarePath = "firmware.bin";

    [[maybe_unused]] std::string DSiBIOS9Path = "dsi_bios9.bin";
    [[maybe_unused]] std::string DSiBIOS7Path = "dsi_bios7.bin";
    [[maybe_unused]] std::string DSiFirmwarePath = "dsi_firmware.bin";
    [[maybe_unused]] std::string DSiNANDPath = "dsi_nand.bin";

    [[maybe_unused]] bool DLDIEnable = true;
    [[maybe_unused]] std::string DLDISDPath;
    [[maybe_unused]] int DLDISize = 0;
    [[maybe_unused]] bool DLDIReadOnly = false;
    [[maybe_unused]] bool DLDIFolderSync;
    [[maybe_unused]] std::string DLDIFolderPath;

    [[maybe_unused]] bool DSiSDEnable;
    [[maybe_unused]] std::string DSiSDPath = "dsi_sd_card.bin";
    [[maybe_unused]] int DSiSDSize;
    [[maybe_unused]] bool DSiSDReadOnly;
    [[maybe_unused]] bool DSiSDFolderSync;
    [[maybe_unused]] std::string DSiSDFolderPath;

    [[maybe_unused]] bool FirmwareOverrideSettings;
    [[maybe_unused]] std::string FirmwareUsername;
    [[maybe_unused]] int FirmwareLanguage;
    [[maybe_unused]] int FirmwareBirthdayMonth;
    [[maybe_unused]] int FirmwareBirthdayDay;
    [[maybe_unused]] int FirmwareFavouriteColour;
    [[maybe_unused]] std::string FirmwareMessage;
    [[maybe_unused]] std::string FirmwareMAC;

    int AudioInterp;
    [[maybe_unused]] int AudioBitrate;
    int MicInputType;

    namespace Retro {
        melonds::MicButtonMode MicButtonMode = melonds::MicButtonMode::Hold;
        bool RandomizeMac = false;
        melonds::ScreenSwapMode ScreenSwapMode;
        melonds::Renderer CurrentRenderer;
        melonds::Renderer ConfiguredRenderer;
        float CursorSize = 2.0;
        int FlushDelay = 120; // TODO: Make configurable

        namespace Category {
            static const char* const VIDEO = "video";
            static const char* const AUDIO = "audio";
            static const char* const SYSTEM = "system";
            static const char* const SAVE = "save";
            static const char* const DSI = "dsi";
            static const char* const SCREEN = "screen";
        }

        namespace Keys {
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
            static const char* const USE_FIRMWARE_SETTINGS = "melonds_use_fw_settings";
            static const char* const LANGUAGE = "melonds_language";
            static const char* const HOMEBREW_SAVE_MODE = "melonds_homebrew_sdcard";
            static const char* const HOMEBREW_READ_ONLY = "melonds_homebrew_readonly";
            static const char* const HOMEBREW_DEDICATED_CARD_SIZE = "melonds_homebrew_dedicated_sdcard_size";
            static const char* const HOMEBREW_SYNC_TO_HOST = "melonds_homebrew_sync_sdcard_to_host";
            static const char* const DSI_SD_SAVE_MODE = "melonds_dsi_sdcard";
            static const char* const DSI_SD_READ_ONLY = "melonds_dsi_sdcard_readonly";
            static const char* const DSI_SD_DEDICATED_CARD_SIZE = "melonds_dsi_sdcard_dedicated_sdcard_size";
            static const char* const DSI_SD_SYNC_TO_HOST = "melonds_dsi_sdcard_sync_sdcard_to_host";
        }

        namespace Values {
            static const char* const _10BIT = "10bit";
            static const char* const _16BIT = "16bit";
            static const char* const ALWAYS = "always";
            static const char* const AUTO = "auto";
            static const char* const BLOW = "blow";
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
            static const char* const ITALIAN = "it";
            static const char* const JAPANESE = "ja";
            static const char* const LINEAR = "linear";
            static const char* const MICROPHONE = "microphone";
            static const char* const NOISE = "noise";
            static const char* const OPENGL = "opengl";
            static const char* const SHARED256M = "shared0256m";
            static const char* const SHARED512M = "shared0512m";
            static const char* const SHARED1G = "shared1024m";
            static const char* const SHARED2G = "shared2048m";
            static const char* const SHARED4G = "shared4096m";
            static const char* const SILENCE = "silence";
            static const char* const SOFTWARE = "software";
            static const char* const SPANISH = "es";
            static const char* const TOGGLE = "toggle";
        }
    }
}

namespace melonds::config {
    static bool _show_opengl_options = true;
    static bool _show_hybrid_options = true;

#ifdef JIT_ENABLED
    static bool _show_jit_options = true;
#endif

    static void check_system_options(bool initializing) noexcept;
    static void check_audio_options(bool initializing) noexcept;
    static void check_homebrew_save_options(bool initializing) noexcept;
    static void check_dsi_sd_options(bool initializing) noexcept;

    static void apply_audio_options(bool initializing) noexcept;
}

GPU::RenderSettings Config::Retro::RenderSettings() {
    GPU::RenderSettings settings{
        .Soft_Threaded = Config::Threaded3D,
        .GL_ScaleFactor = Config::GL_ScaleFactor,
        .GL_BetterPolygons = Config::GL_BetterPolygons,
    };

    return settings;
}

bool melonds::update_option_visibility() {
    using namespace Config::Retro;
    using retro::environment;
    using namespace melonds::config;
    struct retro_core_option_display option_display{};
    struct retro_variable var{};
    bool updated = false;

#ifdef HAVE_OPENGL
    // Show/hide OpenGL core options
    bool show_opengl_options_prev = _show_opengl_options;

    var.key = Keys::RENDER_MODE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        _show_opengl_options = string_is_equal(var.value, Values::OPENGL);
    }

    if (_show_opengl_options != show_opengl_options_prev) {
        option_display.visible = _show_opengl_options;

        option_display.key = Keys::OPENGL_RESOLUTION;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = Keys::OPENGL_BETTER_POLYGONS;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = Keys::OPENGL_FILTERING;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        updated = true;
    }
#endif

    // Show/hide Hybrid screen options
    bool show_hybrid_options_prev = _show_hybrid_options;

    _show_hybrid_options = true;
    var.key = Keys::SCREEN_LAYOUT;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value &&
        (strcmp(var.value, "Hybrid Top") && strcmp(var.value, "Hybrid Bottom")))
        _show_hybrid_options = false;

    if (_show_hybrid_options != show_hybrid_options_prev) {
        option_display.visible = _show_hybrid_options;

        option_display.key = Keys::HYBRID_SMALL_SCREEN;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

#ifdef HAVE_OPENGL
        option_display.key = Keys::HYBRID_RATIO;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif

        updated = true;
    }

#ifdef JIT_ENABLED
    // Show/hide JIT core options
    bool jit_options_prev = _show_jit_options;

    _show_jit_options = true;
    var.key = Keys::JIT_ENABLE;
    // TODO: Use RETRO_ENVIRONMENT_GET_JIT_CAPABLE
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && string_is_equal(var.value, Values::DISABLED))
        _show_jit_options = false;

    if (_show_jit_options != jit_options_prev) {
        option_display.visible = _show_jit_options;

        option_display.key = Keys::JIT_BLOCK_SIZE;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = Keys::JIT_BRANCH_OPTIMISATIONS;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = Keys::JIT_LITERAL_OPTIMISATIONS;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = Keys::JIT_FAST_MEMORY;
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        updated = true;
    }
#endif

    return updated;
}

// TODO: Consider splitting the code that updates the emulator state into a separate function
// TODO: Organize this function better; split it into a few smaller functions by category
void melonds::update_variables(bool init) noexcept {
    using namespace Config::Retro;
    using retro::environment;
    using melonds::screen_layout_data;

    struct retro_variable var = {nullptr};

#ifdef HAVE_OPENGL
    bool gl_settings_changed = false;
#endif

    // TODO: Use standard melonDS config settings
    ScreenLayout layout = ScreenLayout::TopBottom;
    var.key = Keys::SCREEN_LAYOUT;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "Top/Bottom"))
            layout = ScreenLayout::TopBottom;
        else if (string_is_equal(var.value, "Bottom/Top"))
            layout = ScreenLayout::BottomTop;
        else if (string_is_equal(var.value, "Left/Right"))
            layout = ScreenLayout::LeftRight;
        else if (string_is_equal(var.value, "Right/Left"))
            layout = ScreenLayout::RightLeft;
        else if (string_is_equal(var.value, "Top Only"))
            layout = ScreenLayout::TopOnly;
        else if (string_is_equal(var.value, "Bottom Only"))
            layout = ScreenLayout::BottomOnly;
        else if (string_is_equal(var.value, "Hybrid Top"))
            layout = ScreenLayout::HybridTop;
        else if (string_is_equal(var.value, "Hybrid Bottom"))
            layout = ScreenLayout::HybridBottom;
    }

    // TODO: Use standard melonDS config settings
    var.key = Keys::SCREEN_GAP;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {

        screen_layout_data.screen_gap_unscaled = std::stoi(var.value);
    }

#ifdef HAVE_OPENGL
    // TODO: Use standard melonDS config settings
    var.key = Keys::HYBRID_RATIO;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != nullptr) {
        screen_layout_data.hybrid_ratio = std::stoi(var.value);
    }
#else
    screen_layout_data.hybrid_ratio = 2;
#endif

    // TODO: Use standard melonDS config settings
    var.key = Keys::HYBRID_SMALL_SCREEN;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != nullptr) {
        SmallScreenLayout old_hybrid_screen_value = screen_layout_data.hybrid_small_screen; // Copy the hybrid screen value
        if (string_is_equal(var.value, "Top"))
            screen_layout_data.hybrid_small_screen = SmallScreenLayout::SmallScreenTop;
        else if (string_is_equal(var.value, "Bottom"))
            screen_layout_data.hybrid_small_screen = SmallScreenLayout::SmallScreenBottom;
        else
            screen_layout_data.hybrid_small_screen = SmallScreenLayout::SmallScreenDuplicate;

#ifdef HAVE_OPENGL
        if (old_hybrid_screen_value != screen_layout_data.hybrid_small_screen) {
            gl_settings_changed = true;
        }
#endif
    }

    var.key = Keys::SWAPSCREEN_MODE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != NULL) {
        if (string_is_equal(var.value, "Toggle"))
            Config::Retro::ScreenSwapMode = ScreenSwapMode::Toggle;
        else
            Config::Retro::ScreenSwapMode = ScreenSwapMode::Hold;
    }

    var.key = Keys::RANDOMIZE_MAC_ADDRESS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::Retro::RandomizeMac = string_is_equal(var.value, Values::ENABLED);
    }

#ifdef HAVE_THREADS
    var.key = Keys::THREADED_RENDERER;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::Threaded3D = string_is_equal(var.value, Values::ENABLED);
    }
#endif

    TouchMode new_touch_mode = TouchMode::Disabled;

    var.key = Keys::TOUCH_MODE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "Mouse"))
            new_touch_mode = TouchMode::Mouse;
        else if (string_is_equal(var.value, "Touch"))
            new_touch_mode = TouchMode::Touch;
        else if (string_is_equal(var.value, "Joystick"))
            new_touch_mode = TouchMode::Joystick;
    }

#ifdef HAVE_OPENGL
    if (input_state.current_touch_mode != new_touch_mode) // Hide the cursor
        gl_settings_changed = true;

    // TODO: Fix the OpenGL software only render impl so you can switch at runtime
    if (init) {
        // If we're initializing the game...
        var.key = Keys::RENDER_MODE;
        if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
            if (string_is_equal(var.value, Values::OPENGL)) {
                Config::Retro::ConfiguredRenderer = Renderer::OpenGl;
            } else {
                Config::Retro::ConfiguredRenderer = Renderer::Software;
            }
        }
    }

    var.key = Keys::OPENGL_RESOLUTION;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        int first_char_val = (int) var.value[0];
        int scaleing = std::clamp(first_char_val - 48, 0, 8);

        if (Config::GL_ScaleFactor != scaleing)
            gl_settings_changed = true;

        Config::GL_ScaleFactor = scaleing;
    } else {
        Config::GL_ScaleFactor = 1;
    }

    var.key = Keys::OPENGL_BETTER_POLYGONS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        bool enabled = string_is_equal(var.value, Values::ENABLED);
        gl_settings_changed |= enabled != Config::GL_BetterPolygons;

        Config::GL_BetterPolygons = enabled;
    }

    var.key = Keys::OPENGL_FILTERING;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::ScreenFilter = string_is_equal(var.value, "linear");
    }

    if ((melonds::opengl::UsingOpenGl() && gl_settings_changed) || layout != current_screen_layout())
        // If we're using OpenGL and the settings changed, or the screen layout changed...
        melonds::opengl::RequestOpenGlRefresh();
#endif

#ifdef JIT_ENABLED
    var.key = Keys::JIT_ENABLE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_Enable = string_is_equal(var.value, Values::ENABLED);
    }

    var.key = Keys::JIT_BLOCK_SIZE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_MaxBlockSize = std::stoi(var.value);
    }

    var.key = Keys::JIT_BRANCH_OPTIMISATIONS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_BranchOptimisations = (string_is_equal(var.value, Values::ENABLED));
    }

    var.key = Keys::JIT_LITERAL_OPTIMISATIONS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_LiteralOptimisations = string_is_equal(var.value, Values::ENABLED);
    }

    var.key = Keys::JIT_FAST_MEMORY;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_FastMemory = string_is_equal(var.value, Values::ENABLED);
    }
#endif

    config::check_system_options(init);
    config::check_homebrew_save_options(init);
    config::check_dsi_sd_options(init);
    config::check_audio_options(init);

    input_state.current_touch_mode = new_touch_mode;

    update_screenlayout(layout, &screen_layout_data, Config::Retro::ConfiguredRenderer == Renderer::OpenGl,
                        Config::ScreenSwap);

    update_option_visibility();
}

void melonds::apply_variables(bool init) noexcept {
    config::apply_audio_options(init);
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

static void melonds::config::check_system_options(bool initializing) noexcept {
    using retro::environment;
    using namespace Config::Retro;
    struct retro_variable var = {nullptr};

    if (!initializing)
        return;
    // All of these options take effect when a game starts, so there's no need to update them mid-game

    var.key = Keys::CONSOLE_MODE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::DSI))
            Config::ConsoleType = ConsoleType::DSi;
        else
            Config::ConsoleType = ConsoleType::DS;
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::DS);
        Config::ConsoleType = ConsoleType::DS;
    }

    var.key = Keys::BOOT_DIRECTLY;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::DirectBoot = string_is_equal(var.value, Values::ENABLED);
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::DS);
        Config::DirectBoot = true;
    }

    var.key = Keys::USE_FIRMWARE_SETTINGS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::DISABLED))
            Config::FirmwareOverrideSettings = true;
        else
            Config::FirmwareOverrideSettings = false;
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::DISABLED);
        Config::FirmwareOverrideSettings = false;
    }

    var.key = Keys::LANGUAGE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::AUTO))
            Config::FirmwareLanguage = static_cast<int>(get_firmware_language(retro::get_language()));
        else if (string_is_equal(var.value, Values::JAPANESE))
            Config::FirmwareLanguage = static_cast<int>(FirmwareLanguage::Japanese);
        else if (string_is_equal(var.value, Values::ENGLISH))
            Config::FirmwareLanguage = static_cast<int>(FirmwareLanguage::English);
        else if (string_is_equal(var.value, Values::FRENCH))
            Config::FirmwareLanguage = static_cast<int>(FirmwareLanguage::French);
        else if (string_is_equal(var.value, Values::GERMAN))
            Config::FirmwareLanguage = static_cast<int>(FirmwareLanguage::German);
        else if (string_is_equal(var.value, Values::ITALIAN))
            Config::FirmwareLanguage = static_cast<int>(FirmwareLanguage::Italian);
        else if (string_is_equal(var.value, Values::SPANISH))
            Config::FirmwareLanguage = static_cast<int>(FirmwareLanguage::Spanish);
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to English", var.key);
        Config::FirmwareLanguage = static_cast<int>(FirmwareLanguage::English);
    }

    var.key = Keys::USE_EXTERNAL_BIOS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::ExternalBIOSEnable = string_is_equal(var.value, Values::ENABLED);
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::ENABLED);
        Config::ExternalBIOSEnable = true;
    }
}

static void melonds::config::check_audio_options(bool initializing) noexcept {
    using namespace Config::Retro;
    using retro::environment;

    struct retro_variable var = {nullptr};
    var.key = Keys::MIC_INPUT_BUTTON;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::HOLD)) {
            Config::Retro::MicButtonMode = MicButtonMode::Hold;
//        } else if (string_is_equal(var.value, Values::TOGGLE)) {
//            Config::Retro::MicButtonMode = MicButtonMode::Toggle;
        } else if (string_is_equal(var.value, Values::ALWAYS)) {
            Config::Retro::MicButtonMode = MicButtonMode::Always;
        } else {
            Config::Retro::MicButtonMode = MicButtonMode::Hold;
        }
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::HOLD);
        Config::Retro::MicButtonMode = MicButtonMode::Hold;
    }

    // TODO: Support loading WAV files from the system directory (list them and add modify the config object)
    var.key = Keys::MIC_INPUT;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::MICROPHONE))
            Config::MicInputType = static_cast<int>(MicInputMode::HostMic);
        else if (string_is_equal(var.value, Values::BLOW))
            Config::MicInputType = static_cast<int>(MicInputMode::BlowNoise);
        else if (string_is_equal(var.value, Values::NOISE))
            Config::MicInputType = static_cast<int>(MicInputMode::WhiteNoise);
        else
            Config::MicInputType = static_cast<int>(MicInputMode::None);
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::SILENCE);
        Config::MicInputType = static_cast<int>(MicInputMode::None);
    }

    var.key = Keys::AUDIO_BITDEPTH;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::_10BIT))
            Config::AudioBitrate = static_cast<int>(BitDepth::_10Bit);
        else if (string_is_equal(var.value, Values::_16BIT))
            Config::AudioBitrate = static_cast<int>(BitDepth::_16Bit);
        else
            Config::AudioBitrate = static_cast<int>(BitDepth::Auto);
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::AUTO);
        Config::AudioBitrate = static_cast<int>(BitDepth::Auto);
    }

    var.key = Keys::AUDIO_INTERPOLATION;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::CUBIC))
            Config::AudioInterp = static_cast<int>(AudioInterpolation::Cubic);
        else if (string_is_equal(var.value, Values::COSINE))
            Config::AudioInterp = static_cast<int>(AudioInterpolation::Cosine);
        else if (string_is_equal(var.value, Values::LINEAR))
            Config::AudioInterp = static_cast<int>(AudioInterpolation::Linear);
        else
            Config::AudioInterp = static_cast<int>(AudioInterpolation::None);
    }
    else {
        retro::warn("Failed to get value for %s; defaulting to %s", var.key, Values::DISABLED);
        Config::AudioInterp = static_cast<int>(AudioInterpolation::None);
    }
}

/**
 * Reads the frontend's saved homebrew save data options and applies them to the emulator.
 * @param initializing Whether the emulator is initializing a game.
 * If false, the emulator will not update options that require a restart to take effect.
 */
static void melonds::config::check_homebrew_save_options(bool initializing) noexcept {
    using namespace Config::Retro;
    using retro::environment;
    using retro::get_variable;
    using retro::set_variable;

    // TODO: return if the loaded cart is not a homebrew ROM
    if (!initializing)
        return;
    // All of these options take effect when a game starts, so there's no need to update them mid-game

    const optional<struct retro_game_info>& game_info = retro::content::get_loaded_nds_info();

    if (!game_info)
        // If there's no game loaded, there's no need to update the save mode
        return;

    struct retro_variable var = {nullptr, nullptr};

    var.key = Keys::HOMEBREW_READ_ONLY;
    if (get_variable(&var) && var.value) {
        Config::DLDIReadOnly = string_is_equal(var.value, Values::ENABLED);
    } else {
        Config::DLDIReadOnly = false;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_READ_ONLY, Values::DISABLED);
    }

    var.key = Keys::HOMEBREW_SYNC_TO_HOST;
    if (get_variable(&var) && var.value) {
        Config::DLDIFolderSync = string_is_equal(var.value, Values::ENABLED);
    } else {
        Config::DLDIFolderSync = false;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_SYNC_TO_HOST, Values::DISABLED);
    }

    var.key = Keys::HOMEBREW_DEDICATED_CARD_SIZE;
    int dedicated_card_size = 0;
    if (get_variable(&var) && var.value) {
        try {
            dedicated_card_size = std::stoi(var.value);
            switch (dedicated_card_size) {
                case 0:
                case 256:
                case 512:
                case 1024:
                case 2048:
                case 4096:
                    break;
                default:
                    retro::warn("Invalid homebrew dedicated card size \"%s\"; defaulting to Auto", var.value);
                    dedicated_card_size = 0;
                    break;
            }
        }
        catch (...) {
            retro::warn("Invalid homebrew dedicated card size \"%s\"; defaulting to Auto", var.value);
            dedicated_card_size = 0;
        }
    } else {
        dedicated_card_size = 0;
        retro::warn("Failed to get value for %s; defaulting to Auto", Keys::HOMEBREW_DEDICATED_CARD_SIZE);
    }

    var.key = Keys::HOMEBREW_SAVE_MODE;

    const optional<string>& save_directory = retro::get_save_directory();
    if (save_directory && get_variable(&var) && var.value) {
        char game_name[256];
        memset(game_name, 0, sizeof(game_name));
        const char* ptr = path_basename(game_info->path);
        strlcpy(game_name, ptr ? ptr : game_info->path, sizeof(game_name));
        path_remove_extension(game_name);

        auto set_config = [&save_directory](int size, const char* name) {
            char dldi_path[1024];
            memset(dldi_path, 0, sizeof(dldi_path));
            fill_pathname_join_special(dldi_path, save_directory->c_str(), name, sizeof(dldi_path));

            Config::DLDIFolderPath = string(dldi_path);

            strlcat(dldi_path, ".dldi", sizeof(dldi_path));

            Config::DLDISDPath = string(dldi_path);
            Config::DLDIEnable = true;
            Config::DLDISize = size;
        };

        if (string_is_equal(var.value, Values::DISABLED)) {
            Config::DLDIEnable = false;
            Config::DLDISize = 0;
            Config::DLDIFolderPath = "";
            Config::DLDISDPath = "";
        } else if (string_is_equal(var.value, Values::DEDICATED)) {
            set_config(0, game_name);

            // If the SD card image exists, set the DLDISize to auto; else set it to the dedicated card size
            Config::DLDISize = path_is_valid(Config::DLDISDPath.c_str()) ? 0 : dedicated_card_size;
        } else if (string_is_equal(var.value, Values::SHARED256M)) {
            set_config(256, Values::SHARED256M);
        } else if (string_is_equal(var.value, Values::SHARED512M)) {
            set_config(512, Values::SHARED512M);
        } else if (string_is_equal(var.value, Values::SHARED1G)) {
            set_config(1024, Values::SHARED1G);
        } else if (string_is_equal(var.value, Values::SHARED2G)) {
            set_config(2048, Values::SHARED2G);
        } else if (string_is_equal(var.value, Values::SHARED4G)) {
            set_config(4096, Values::SHARED4G);
        } else {
            retro::warn("Invalid homebrew save mode \"%s\"; defaulting to %s", var.value, Values::DEDICATED);
            set_config(0, game_name);

            // If the SD card image exists, set the DLDISize to auto; else set it to the dedicated card size
            Config::DLDISize = path_is_valid(Config::DLDISDPath.c_str()) ? 0 : dedicated_card_size;
        }
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_SAVE_MODE, Values::DISABLED);
        Config::DLDIEnable = false;
        Config::DLDISize = 0;
        Config::DLDIFolderPath = "";
        Config::DLDISDPath = "";
    }

    retro::log(RETRO_LOG_DEBUG, "DLDIEnable: %s", Config::DLDIEnable ? "true" : "false");
    retro::log(RETRO_LOG_DEBUG, "DLDIReadOnly: %s", Config::DLDIReadOnly ? "true" : "false");
    retro::log(RETRO_LOG_DEBUG, "DLDIFolderSync: %s", Config::DLDIFolderSync ? "true" : "false");
    retro::log(RETRO_LOG_DEBUG, "DLDISize: %d", Config::DLDISize);
    retro::log(RETRO_LOG_DEBUG, "DLDISDPath: %s", Config::DLDISDPath.c_str());
    retro::log(RETRO_LOG_DEBUG, "DLDIFolderPath: %s", Config::DLDIFolderPath.c_str());
}

/**
 * Reads the frontend's saved DSi save data options and applies them to the emulator.
 * @param initializing Whether the emulator is initializing a game.
 * If false, the emulator will not update options that require a restart to take effect.
 */
static void melonds::config::check_dsi_sd_options(bool initializing) noexcept {
    using namespace Config::Retro;
    using retro::environment;
    using retro::get_variable;
    using retro::set_variable;

    if (!initializing)
        return;
    // All of these options take effect when a game starts, so there's no need to update them mid-game

    const optional<struct retro_game_info>& game_info = retro::content::get_loaded_nds_info();

    if (!game_info)
        // If there's no game loaded, there's no need to update the save mode
        return;

    struct retro_variable var = {nullptr, nullptr};

    var.key = Keys::DSI_SD_READ_ONLY;
    if (get_variable(&var) && var.value) {
        Config::DSiSDReadOnly = string_is_equal(var.value, Values::ENABLED);
    } else {
        Config::DSiSDReadOnly = false;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::DSI_SD_READ_ONLY, Values::DISABLED);
    }

    var.key = Keys::DSI_SD_SYNC_TO_HOST;
    if (get_variable(&var) && var.value) {
        Config::DSiSDFolderSync = string_is_equal(var.value, Values::ENABLED);
    } else {
        Config::DSiSDFolderSync = false;
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::DSI_SD_SYNC_TO_HOST, Values::DISABLED);
    }

    var.key = Keys::DSI_SD_DEDICATED_CARD_SIZE;
    int dedicated_card_size = 0;
    if (get_variable(&var) && var.value) {
        try {
            dedicated_card_size = std::stoi(var.value);
            switch (dedicated_card_size) {
                case 0:
                case 256:
                case 512:
                case 1024:
                case 2048:
                case 4096:
                    break;
                default:
                    retro::warn("Invalid DSi dedicated card size \"%s\"; defaulting to Auto", var.value);
                    dedicated_card_size = 0;
                    break;
            }
        }
        catch (...) {
            retro::warn("Invalid DSi dedicated card size \"%s\"; defaulting to Auto", var.value);
            dedicated_card_size = 0;
        }
    } else {
        dedicated_card_size = 0;
        retro::warn("Failed to get value for %s; defaulting to Auto", Keys::DSI_SD_DEDICATED_CARD_SIZE);
    }

    var.key = Keys::DSI_SD_SAVE_MODE;

    const optional<string>& save_directory = retro::get_save_directory();
    if (save_directory && get_variable(&var) && var.value) {
        char game_name[256];
        memset(game_name, 0, sizeof(game_name));
        const char* ptr = path_basename(game_info->path);
        strlcpy(game_name, ptr ? ptr : game_info->path, sizeof(game_name));
        path_remove_extension(game_name);

        auto set_config = [&save_directory](int size, const char* name) {
            char sd_path[1024];
            memset(sd_path, 0, sizeof(sd_path));
            fill_pathname_join_special(sd_path, save_directory->c_str(), name, sizeof(sd_path));

            Config::DSiSDFolderPath = string(sd_path);

            strlcat(sd_path, ".dsisd", sizeof(sd_path));

            Config::DSiSDPath = string(sd_path);
            Config::DSiSDEnable = true;
            Config::DSiSDSize = size;
        };

        if (string_is_equal(var.value, Values::DISABLED)) {
            Config::DSiSDEnable = false;
            Config::DSiSDSize = 0;
            Config::DSiSDFolderPath = "";
            Config::DSiSDPath = "";
        } else if (string_is_equal(var.value, Values::DEDICATED)) {
            set_config(0, game_name);

            // If the SD card image exists, set the DSiSDSize to auto; else set it to the dedicated card size
            Config::DSiSDSize = path_is_valid(Config::DSiSDPath.c_str()) ? 0 : dedicated_card_size;
        } else if (string_is_equal(var.value, Values::SHARED256M)) {
            set_config(256, Values::SHARED256M);
        } else if (string_is_equal(var.value, Values::SHARED512M)) {
            set_config(512, Values::SHARED512M);
        } else if (string_is_equal(var.value, Values::SHARED1G)) {
            set_config(1024, Values::SHARED1G);
        } else if (string_is_equal(var.value, Values::SHARED2G)) {
            set_config(2048, Values::SHARED2G);
        } else if (string_is_equal(var.value, Values::SHARED4G)) {
            set_config(4096, Values::SHARED4G);
        } else {
            retro::warn("Invalid homebrew save mode \"%s\"; defaulting to %s", var.value, Values::DEDICATED);
            set_config(0, game_name);

            // If the SD card image exists, set the DSiSDSize to auto; else set it to the dedicated card size
            Config::DSiSDSize = path_is_valid(Config::DSiSDPath.c_str()) ? 0 : dedicated_card_size;
        }
    } else {
        retro::warn("Failed to get value for %s; defaulting to %s", Keys::HOMEBREW_SAVE_MODE, Values::DISABLED);
        Config::DSiSDEnable = false;
        Config::DSiSDSize = 0;
        Config::DSiSDFolderPath = "";
        Config::DSiSDPath = "";
    }

    retro::log(RETRO_LOG_DEBUG, "DSiSDEnable: %s", Config::DSiSDEnable ? "true" : "false");
    retro::log(RETRO_LOG_DEBUG, "DSiSDReadOnly: %s", Config::DSiSDReadOnly ? "true" : "false");
    retro::log(RETRO_LOG_DEBUG, "DSiSDFolderSync: %s", Config::DSiSDFolderSync ? "true" : "false");
    retro::log(RETRO_LOG_DEBUG, "DSiSDSize: %d", Config::DSiSDSize);
    retro::log(RETRO_LOG_DEBUG, "DSiSDPath: %s", Config::DSiSDPath.c_str());
    retro::log(RETRO_LOG_DEBUG, "DSiSDFolderPath: %s", Config::DSiSDFolderPath.c_str());
}

static void melonds::config::apply_audio_options(bool initializing) noexcept {
    if (retro::microphone::is_interface_available()) {
        bool is_using_host_mic = static_cast<MicInputMode>(Config::MicInputType) == MicInputMode::HostMic;
        // Open the mic if the user wants it (and it isn't already open)
        // Close the mic if the user wants it (and it is open)
        bool ok = retro::microphone::set_open(is_using_host_mic);
        if (!ok) {
            // If we couldn't open or close the microphone...
            retro::warn("Failed to %s microphone", is_using_host_mic ? "open" : "close");
        }
    }
    else {
        bool is_using_host_mic = static_cast<MicInputMode>(Config::MicInputType) == MicInputMode::HostMic;

        if (is_using_host_mic) {
            retro::set_warn_message("This frontend doesn't support microphones.");
        }
    }
}

struct retro_core_option_v2_category option_cats_us[] = {
    {
        "system",
        "System",
        "Change system settings."
    },
    {
        Config::Retro::Category::DSI,
        "DSi",
        "Change system settings specific to the Nintendo DSi."
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
        Config::Retro::Category::SAVE,
        "Save Data",
        "Change save data settings."
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
        "Some features may not be available in DSi mode.",
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
        Config::Retro::Keys::USE_FIRMWARE_SETTINGS,
        "Use Firmware Settings",
        nullptr,
        "Use language and username specified in the DS firmware, "
        "rather than those provided by the frontend. "
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
        "Ignored if 'Use Firmware Settings' is enabled."
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
        Config::Retro::Keys::RANDOMIZE_MAC_ADDRESS,
        "Randomize MAC Address",
        nullptr,
        nullptr,
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
        "Determines how the emulated DSi saves data to the virtual SD card. "
        "If set to Disabled, the DSi won't see an SD card and data won't be saved. "
        "If set to Dedicated, the game uses its own virtual SD card. "
        "If set to Shared, the game uses one of five shared virtual SD cards (each of which is a different size). "
        "Card images are dynamically-sized, so they'll only take up as much space as they need. "
        "Changing this setting does not transfer existing data. "
        "Changes take effect with next restart.",
        nullptr,
        Config::Retro::Category::DSI,
        {
            {Config::Retro::Values::DISABLED, "Disabled"},
            {Config::Retro::Values::SHARED256M, "Shared (256 MiB)"},
            {Config::Retro::Values::SHARED512M, "Shared (512 MiB)"},
            {Config::Retro::Values::SHARED1G, "Shared (1 GiB)"},
            {Config::Retro::Values::SHARED2G, "Shared (2 GiB)"},
            {Config::Retro::Values::SHARED4G, "Shared (4 GiB)"},
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
        Config::Retro::Category::DSI,
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
        Config::Retro::Category::DSI,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
    {
        Config::Retro::Keys::DSI_SD_DEDICATED_CARD_SIZE,
        "Dedicated SD Card Size (DSi)",
        nullptr,
        "The size of new dedicated virtual SD cards. "
        "Will not alter the size of existing card images.",
        nullptr,
        Config::Retro::Category::DSI,
        {
            {"0", "Auto"},
            {"256", "256 MiB"},
            {"512", "512 MiB"},
            {"1024", "1 GiB"},
            {"2048", "2 GiB"},
            {"4096", "4 GiB"},
            {nullptr, nullptr},
        },
        "0",
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
            {"1x native (256x192)", nullptr},
            {"2x native (512x384)", nullptr},
            {"3x native (768x576)", nullptr},
            {"4x native (1024x768)", nullptr},
            {"5x native (1280x960)", nullptr},
            {"6x native (1536x1152)", nullptr},
            {"7x native (1792x1344)", nullptr},
            {"8x native (2048x1536)", nullptr},
            {nullptr, nullptr},
        },
        "1x native (256x192)"
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
            {"Mouse", nullptr},
            {"Touch", nullptr},
            {"Joystick", nullptr},
            {Config::Retro::Values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        "Mouse"
    },
    {
        Config::Retro::Keys::SWAPSCREEN_MODE,
        "Swap Screen Mode",
        nullptr,
        "Choose if the 'Swap screens' button should work on press or on hold.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {"Toggle", nullptr},
            {"Hold", nullptr},
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
            {"Top/Bottom", nullptr},
            {"Bottom/Top", nullptr},
            {"Left/Right", nullptr},
            {"Right/Left", nullptr},
            {"Top Only", nullptr},
            {"Bottom Only", nullptr},
            {"Hybrid Top", nullptr},
            {"Hybrid Bottom", nullptr},
            {nullptr, nullptr},
        },
        "Top/Bottom"
    },
    {
        Config::Retro::Keys::SCREEN_GAP,
        "Screen Gap",
        nullptr,
        "Choose how large the gap between the 2 screens should be.",
        nullptr,
        Config::Retro::Category::SCREEN,
        {
            {"0", nullptr},
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
            {"33", nullptr},
            {"34", nullptr},
            {"35", nullptr},
            {"36", nullptr},
            {"37", nullptr},
            {"38", nullptr},
            {"39", nullptr},
            {"40", nullptr},
            {"41", nullptr},
            {"42", nullptr},
            {"43", nullptr},
            {"44", nullptr},
            {"45", nullptr},
            {"46", nullptr},
            {"47", nullptr},
            {"48", nullptr},
            {"49", nullptr},
            {"50", nullptr},
            {"51", nullptr},
            {"52", nullptr},
            {"53", nullptr},
            {"54", nullptr},
            {"55", nullptr},
            {"56", nullptr},
            {"57", nullptr},
            {"58", nullptr},
            {"59", nullptr},
            {"60", nullptr},
            {"61", nullptr},
            {"62", nullptr},
            {"63", nullptr},
            {"64", nullptr},
            {"65", nullptr},
            {"66", nullptr},
            {"67", nullptr},
            {"68", nullptr},
            {"69", nullptr},
            {"70", nullptr},
            {"71", nullptr},
            {"72", nullptr},
            {"73", nullptr},
            {"74", nullptr},
            {"75", nullptr},
            {"76", nullptr},
            {"77", nullptr},
            {"78", nullptr},
            {"79", nullptr},
            {"80", nullptr},
            {"81", nullptr},
            {"82", nullptr},
            {"83", nullptr},
            {"84", nullptr},
            {"85", nullptr},
            {"86", nullptr},
            {"87", nullptr},
            {"88", nullptr},
            {"89", nullptr},
            {"90", nullptr},
            {"91", nullptr},
            {"92", nullptr},
            {"93", nullptr},
            {"94", nullptr},
            {"95", nullptr},
            {"96", nullptr},
            {"97", nullptr},
            {"98", nullptr},
            {"99", nullptr},
            {"100", nullptr},
            {"101", nullptr},
            {"102", nullptr},
            {"103", nullptr},
            {"104", nullptr},
            {"105", nullptr},
            {"106", nullptr},
            {"107", nullptr},
            {"108", nullptr},
            {"109", nullptr},
            {"110", nullptr},
            {"111", nullptr},
            {"112", nullptr},
            {"113", nullptr},
            {"114", nullptr},
            {"115", nullptr},
            {"116", nullptr},
            {"117", nullptr},
            {"118", nullptr},
            {"119", nullptr},
            {"120", nullptr},
            {"121", nullptr},
            {"122", nullptr},
            {"123", nullptr},
            {"124", nullptr},
            {"125", nullptr},
            {"126", nullptr},
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
            {"Bottom", nullptr},
            {"Top", nullptr},
            {"Duplicate", nullptr},
            {nullptr, nullptr},
        },
        "Bottom"
    },

    // Homebrew Save Data
    {
        Config::Retro::Keys::HOMEBREW_SAVE_MODE,
        "Virtual SD Card",
        nullptr,
        "Determines how homebrew saves data to the virtual SD card. "
        "If set to Disabled, the game won't see an SD card and data won't be saved. "
        "If set to Dedicated, the game uses its own virtual SD card. "
        "If set to Shared, the game uses one of five shared virtual SD cards (each of which is a different size). "
        "Card images are dynamically-sized, so they'll only take up as much space as they need. "
        "Changing this setting does not transfer existing data. "
        "Changes take effect with next restart.",
        nullptr,
        Config::Retro::Category::SAVE,
        {
            {Config::Retro::Values::DISABLED, "Disabled"},
            {Config::Retro::Values::SHARED256M, "Shared (256 MiB)"},
            {Config::Retro::Values::SHARED512M, "Shared (512 MiB)"},
            {Config::Retro::Values::SHARED1G, "Shared (1 GiB)"},
            {Config::Retro::Values::SHARED2G, "Shared (2 GiB)"},
            {Config::Retro::Values::SHARED4G, "Shared (4 GiB)"},
            {Config::Retro::Values::DEDICATED, "Dedicated"},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DEDICATED
    },
    {
        Config::Retro::Keys::HOMEBREW_READ_ONLY,
        "Read-Only Mode",
        nullptr,
        "If enabled, homebrew applications will see the virtual SD card as read-only. "
        "Changes take effect with next restart.",
        nullptr,
        Config::Retro::Category::SAVE,
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
        Config::Retro::Category::SAVE,
        {
            {Config::Retro::Values::DISABLED, nullptr},
            {Config::Retro::Values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        Config::Retro::Values::DISABLED
    },
    {
        Config::Retro::Keys::HOMEBREW_DEDICATED_CARD_SIZE,
        "Dedicated SD Card Size",
        nullptr,
        "The size of new dedicated virtual SD cards. "
        "Will not alter the size of existing card images.",
        nullptr,
        Config::Retro::Category::SAVE,
        {
            {"0", "Auto"},
            {"256", "256 MiB"},
            {"512", "512 MiB"},
            {"1024", "1 GiB"},
            {"2048", "2 GiB"},
            {"4096", "4 GiB"},
            {nullptr, nullptr},
        },
        "0",
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
            {"2", nullptr},
            {"3", nullptr},
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