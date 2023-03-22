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
#include "libretro.hpp"
#include "environment.hpp"
#include "screenlayout.hpp"
#include "input.hpp"
#include "opengl.hpp"

namespace Config {
    bool ScreenSwap;
    bool ScreenFilter;

    bool ScreenUseGL;

    int _3DRenderer;
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

    // TODO: Make configurable
    [[maybe_unused]] bool ExternalBIOSEnable = true;

    [[maybe_unused]] std::string BIOS9Path;
    [[maybe_unused]] std::string BIOS7Path;
    [[maybe_unused]] std::string FirmwarePath;

    [[maybe_unused]] std::string DSiBIOS9Path;
    [[maybe_unused]] std::string DSiBIOS7Path;
    [[maybe_unused]] std::string DSiFirmwarePath;
    [[maybe_unused]] std::string DSiNANDPath;

    [[maybe_unused]] bool DLDIEnable;
    [[maybe_unused]] std::string DLDISDPath;
    [[maybe_unused]] int DLDISize;
    [[maybe_unused]] bool DLDIReadOnly;
    [[maybe_unused]] bool DLDIFolderSync;
    [[maybe_unused]] std::string DLDIFolderPath;

    [[maybe_unused]] bool DSiSDEnable;
    [[maybe_unused]] std::string DSiSDPath;
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

    std::string SaveFilePath;

    namespace Retro {
        bool MicButtonRequired = true;
        bool RandomizeMac = false;
        melonds::ScreenSwapMode ScreenSwapMode;
        melonds::CurrentRenderer CurrentRenderer;
        float CursorSize = 2.0;

        namespace Keys {
            static const char *const OPENGL_RENDERER = "melonds_opengl_renderer";
            static const char *const OPENGL_RESOLUTION = "melonds_opengl_resolution";
            static const char *const THREADED_RENDERER = "melonds_threaded_renderer";
            static const char *const OPENGL_BETTER_POLYGONS = "melonds_opengl_better_polygons";
            static const char *const OPENGL_FILTERING = "melonds_opengl_filtering";
            static const char *const SCREEN_LAYOUT = "melonds_screen_layout";
            static const char *const HYBRID_SMALL_SCREEN = "melonds_hybrid_small_screen";
            static const char *const HYBRID_RATIO = "melonds_hybrid_ratio";
            static const char *const JIT_ENABLE = "melonds_jit_enable";
            static const char *const JIT_BLOCK_SIZE = "melonds_jit_block_size";
            static const char *const JIT_BRANCH_OPTIMISATIONS = "melonds_jit_branch_optimisations";
            static const char *const JIT_LITERAL_OPTIMISATIONS = "melonds_jit_literal_optimisations";
            static const char *const JIT_FAST_MEMORY = "melonds_jit_fast_memory";
            static const char *const CONSOLE_MODE = "melonds_console_mode";
            static const char *const BOOT_DIRECTLY = "melonds_boot_directly";
            static const char *const SCREEN_GAP = "melonds_screen_gap";
            static const char *const SWAPSCREEN_MODE = "melonds_swapscreen_mode";
            static const char *const RANDOMIZE_MAC_ADDRESS = "melonds_randomize_mac_address";
            static const char *const TOUCH_MODE = "melonds_touch_mode";
            static const char *const MIC_INPUT_BUTTON = "melonds_need_button_mic_input";
            static const char *const DSI_SDCARD = "melonds_dsi_sdcard";
            static const char *const MIC_INPUT = "melonds_mic_input";
            static const char *const AUDIO_BITRATE = "melonds_audio_bitrate";
            static const char *const AUDIO_INTERPOLATION = "melonds_audio_interpolation";
            static const char *const USE_FIRMWARE_SETTINGS = "melonds_use_fw_settings";
            static const char *const LANGUAGE = "melonds_language";
        }

        namespace Values {
            static const char *const DISABLED = "disabled";
            static const char *const ENABLED = "enabled";
        }
    }
}

namespace melonds::config {
    static bool _show_opengl_options = true;
    static bool _show_hybrid_options = true;
    static GPU::RenderSettings _render_settings;

#ifdef JIT_ENABLED
    static bool _show_jit_options = true;
#endif
}

GPU::RenderSettings Config::Retro::RenderSettings() {
    return GPU::RenderSettings{
            .Soft_Threaded = !Config::Threaded3D,
            .GL_ScaleFactor = Config::GL_ScaleFactor,
            .GL_BetterPolygons = Config::GL_BetterPolygons,
    };
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

    _show_opengl_options = true;
    var.key = Keys::OPENGL_RENDERER;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && string_is_equal(var.value, Values::DISABLED))
        _show_opengl_options = false;

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

void melonds::check_variables(bool init) {
    using namespace Config::Retro;
    using retro::environment;
    using melonds::screen_layout_data;

    struct retro_variable var = {nullptr};

#ifdef HAVE_OPENGL
    bool gl_settings_changed = false;
#endif

    var.key = Keys::CONSOLE_MODE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "DSi"))
            Config::ConsoleType = ConsoleType::DSi;
        else
            Config::ConsoleType = ConsoleType::DS;
    }

    var.key = Keys::BOOT_DIRECTLY;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::DirectBoot = string_is_equal(var.value, Values::ENABLED);
    }

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
        var.key = Keys::OPENGL_RENDERER;
        if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
            Config::ScreenUseGL = string_is_equal(var.value, Values::ENABLED);

            if (Config::Retro::UsingOpenGl)
                Config::Retro::CurrentRenderer = Config::ScreenUseGL ? CurrentRenderer::OpenGl
                                                                     : CurrentRenderer::Software;
        }
    }

    if (Config::ScreenUseGL) {
        // Running the software rendering thread at the same time as OpenGL is used will cause segfault on cleanup
        config::_render_settings.Soft_Threaded = false;
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
    config::_render_settings.GL_ScaleFactor = Config::GL_ScaleFactor;

    var.key = Keys::OPENGL_BETTER_POLYGONS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        bool enabled = string_is_equal(var.value, Values::ENABLED);
        gl_settings_changed |= enabled != Config::GL_BetterPolygons;

        Config::GL_BetterPolygons = enabled;
    }
    config::_render_settings.GL_BetterPolygons = Config::GL_BetterPolygons;

    var.key = Keys::OPENGL_FILTERING;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::ScreenFilter = string_is_equal(var.value, "linear");
    }

    if ((Config::Retro::UsingOpenGl && gl_settings_changed) || layout != current_screen_layout())
        melonds::opengl::refresh_opengl = true;
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

    var.key = Keys::DSI_SDCARD;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::DSiSDEnable = string_is_equal(var.value, Values::ENABLED);
    }

    var.key = Keys::MIC_INPUT;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "Microphone Input"))
            Config::MicInputType = static_cast<int>(MicInputMode::HostMic);
        else if (string_is_equal(var.value, "Blow Noise"))
            Config::MicInputType = static_cast<int>(MicInputMode::BlowNoise);
        else if (string_is_equal(var.value, "White Noise"))
            Config::MicInputType = static_cast<int>(MicInputMode::WhiteNoise);
        else
            Config::MicInputType = static_cast<int>(MicInputMode::None);

        if (static_cast<MicInputMode>(Config::MicInputType) !=
            MicInputMode::HostMic /*&& micInterface.interface_version && micHandle*/) {
            // If the player wants to stop using the real mic as the DS mic's input...
            // micInterface.set_mic_state(micHandle, false);
        }
    }

    var.key = Keys::MIC_INPUT_BUTTON;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::Retro::MicButtonRequired = string_is_equal(var.value, "With Button");

//        if (noise_button_required &&
//            micInterface.interface_version &&
//            micHandle != NULL &&
//            !input_state.holding_noise_btn) { // If the player wants to require the noise button for mic input and they aren't already holding it...
//            micInterface.set_mic_state(micHandle, false);
//        }
    }

    var.key = Keys::AUDIO_BITRATE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "10-bit"))
            Config::AudioBitrate = 1;
        else if (string_is_equal(var.value, "16-bit"))
            Config::AudioBitrate = 2;
        else
            Config::AudioBitrate = 0;
    }

    var.key = Keys::AUDIO_INTERPOLATION;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "Cubic"))
            Config::AudioInterp = 3;
        else if (string_is_equal(var.value, "Cosine"))
            Config::AudioInterp = 2;
        else if (string_is_equal(var.value, "Linear"))
            Config::AudioInterp = 1;
        else
            Config::AudioInterp = 0;
    }

    var.key = Keys::USE_FIRMWARE_SETTINGS;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, Values::DISABLED))
            Config::FirmwareOverrideSettings = true;
        else
            Config::FirmwareOverrideSettings = false;
    }

    var.key = Keys::LANGUAGE;
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "Japanese"))
            Config::FirmwareLanguage = 0;
        else if (string_is_equal(var.value, "English"))
            Config::FirmwareLanguage = 1;
        else if (string_is_equal(var.value, "French"))
            Config::FirmwareLanguage = 2;
        else if (string_is_equal(var.value, "German"))
            Config::FirmwareLanguage = 3;
        else if (string_is_equal(var.value, "Italian"))
            Config::FirmwareLanguage = 4;
        else if (string_is_equal(var.value, "Spanish"))
            Config::FirmwareLanguage = 5;
    }

    input_state.current_touch_mode = new_touch_mode;

    update_screenlayout(layout, &screen_layout_data, Config::ScreenUseGL, Config::ScreenSwap);

    update_option_visibility();
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
                "screen",
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

/// These items intentionally share config keys with the original melonDS core,
/// in order to simplify migration.
struct retro_core_option_v2_definition melonds::option_defs_us[] = {
        {
                Config::Retro::Keys::CONSOLE_MODE,
                "Console Mode",
                nullptr,
                "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. "
                "Some features may not be available in DSi mode.",
                nullptr,
                "system",
                {
                        {"DS", nullptr},
                        {"DSi", nullptr},
                        {nullptr, nullptr},
                },
                "DS"
        },
        {
                Config::Retro::Keys::BOOT_DIRECTLY,
                "Boot Game Directly",
                nullptr,
                "Whether melonDS should directly boot the game or enter the DS menu beforehand. "
                "If disabled, compatible BIOS and firmware files must be provided in the system directory. "
                "Ignored if the core is loaded without a game, "
                "or if suitable BIOS/firmware files weren't found.",
                nullptr,
                "system",
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
                "The language mode of the emulated DS. "
                "Ignored if 'Use Firmware Settings' is enabled or if no valid firmware file was found. "
                "Not every game honors this setting. "
                "'Default' uses the frontend's language if supported by the DS, or English if not.",
                nullptr,
                "system",
                {
                        {"Japanese", nullptr},
                        {"English", nullptr},
                        {"French", nullptr},
                        {"German", nullptr},
                        {"Italian", nullptr},
                        {"Spanish", nullptr},
                        {nullptr, nullptr},
                },
                "English"
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
                Config::Retro::Keys::DSI_SDCARD,
                "Enable DSi SD Card",
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
#ifdef HAVE_THREADS
        {
                Config::Retro::Keys::THREADED_RENDERER,
                "Threaded Software Renderer",
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
#endif
#ifdef HAVE_OPENGL
        {
                Config::Retro::Keys::OPENGL_RENDERER,
                "OpenGL Renderer",
                nullptr,
                "Restart required.",
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
                Config::Retro::Keys::OPENGL_RESOLUTION,
                "OpenGL Internal Resolution",
                nullptr,
                nullptr,
                nullptr,
                "video",
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
                "video",
                {
                        {"nearest", nullptr},
                        {"linear", nullptr},
                        {nullptr, nullptr},
                },
                "nearest"
        },
#endif
        {
                Config::Retro::Keys::MIC_INPUT,
                "Microphone Input",
                nullptr,
                "Choose the type of noise that will be used as microphone input.",
                nullptr,
                "audio",
                {
                        {"Disabled", nullptr},
                        {"Blow Noise", nullptr},
                        {"White Noise", nullptr},
                        {"Microphone Input", nullptr},
                        {nullptr, nullptr},
                },
                "Microphone Input"
        },
        {
                Config::Retro::Keys::MIC_INPUT_BUTTON,
                "Listen for Mic Input",
                nullptr,
                "Set the microphone to be active when the mic button is held, or at all times.",
                nullptr,
                "audio",
                {
                        {"With Button", nullptr},
                        {"Always", nullptr},
                        {nullptr, nullptr},
                },
                "With Button"
        },
        {
                Config::Retro::Keys::AUDIO_BITRATE,
                "Audio Bitrate",
                nullptr,
                nullptr,
                nullptr,
                "audio",
                {
                        {"Automatic", nullptr},
                        {"10-bit", nullptr},
                        {"16-bit", nullptr},
                        {nullptr, nullptr},
                },
                "Automatic"
        },
        {
                Config::Retro::Keys::AUDIO_INTERPOLATION,
                "Audio Interpolation",
                nullptr,
                nullptr,
                nullptr,
                "audio",
                {
                        {"None", nullptr},
                        {"Linear", nullptr},
                        {"Cosine", nullptr},
                        {"Cubic", nullptr},
                        {nullptr, nullptr},
                },
                "None"
        },
        {
                Config::Retro::Keys::TOUCH_MODE,
                "Touch Mode",
                nullptr,
                "Choose mode for interactions with the touch screen.",
                nullptr,
                "screen",
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
                "screen",
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
                "screen",
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
                "screen",
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
                "screen",
                {
                        {"Bottom", nullptr},
                        {"Top", nullptr},
                        {"Duplicate", nullptr},
                        {nullptr, nullptr},
                },
                "Bottom"
        },
#ifdef HAVE_OPENGL
        {
                Config::Retro::Keys::HYBRID_RATIO,
                "Hybrid Ratio (OpenGL Only)",
                nullptr,
                nullptr,
                nullptr,
                "screen",
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
struct retro_core_options_v2 *melonds::options_intl[] = {
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