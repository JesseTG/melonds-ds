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

#include <cstring>
#include <frontend/qt_sdl/Config.h>
#include <GPU.h>
#include <string/stdstring.h>
#include "libretro.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "screenlayout.hpp"
#include "input.hpp"
#include "opengl.hpp"

namespace Config::Retro {
    bool MicButtonRequired = true;
}

namespace melonds::config {
    static unsigned _cursor_size = 2; // TODO: Make configurable
    static bool _show_opengl_options = true;
    static bool _show_hybrid_options = true;
    ScreenSwapMode screen_swap_mode = ScreenSwapMode::Toggle;
    static bool _randomize_mac = false;
    static GPU::RenderSettings _render_settings;
    static melonds::RendererType _renderer_type = melonds::RendererType::OpenGl;

#ifdef JIT_ENABLED
    static bool _show_jit_options = true;
#endif
}

unsigned melonds::cursor_size() {
    return config::_cursor_size;
}

GPU::RenderSettings &melonds::render_settings() {
    return config::_render_settings;
}

bool melonds::update_option_visibility() {
    using retro::environment;
    using namespace melonds::config;
    struct retro_core_option_display option_display{};
    struct retro_variable var{};
    bool updated = false;

#ifdef HAVE_OPENGL
    // Show/hide OpenGL core options
    bool show_opengl_options_prev = _show_opengl_options;

    _show_opengl_options = true;
    var.key = "melonds_opengl_renderer";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && string_is_equal(var.value, "disabled"))
        _show_opengl_options = false;

    if (_show_opengl_options != show_opengl_options_prev) {
        option_display.visible = _show_opengl_options;

        option_display.key = "melonds_opengl_resolution";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = "melonds_opengl_better_polygons";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = "melonds_opengl_filtering";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        updated = true;
    }
#endif

    // Show/hide Hybrid screen options
    bool show_hybrid_options_prev = _show_hybrid_options;

    _show_hybrid_options = true;
    var.key = "melonds_screen_layout";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value &&
        (strcmp(var.value, "Hybrid Top") && strcmp(var.value, "Hybrid Bottom")))
        _show_hybrid_options = false;

    if (_show_hybrid_options != show_hybrid_options_prev) {
        option_display.visible = _show_hybrid_options;

        option_display.key = "melonds_hybrid_small_screen";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

#ifdef HAVE_OPENGL
        option_display.key = "melonds_hybrid_ratio";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif

        updated = true;
    }

#ifdef JIT_ENABLED
    // Show/hide JIT core options
    bool jit_options_prev = _show_jit_options;

    _show_jit_options = true;
    var.key = "melonds_jit_enable";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && string_is_equal(var.value, "disabled"))
        _show_jit_options = false;

    if (_show_jit_options != jit_options_prev) {
        option_display.visible = _show_jit_options;

        option_display.key = "melonds_jit_block_size";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = "melonds_jit_branch_optimisations";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = "melonds_jit_literal_optimisations";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key = "melonds_jit_fast_memory";
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        updated = true;
    }
#endif

    return updated;
}

void melonds::check_variables(bool init) {
    using retro::environment;
    using melonds::screen_layout_data;

    struct retro_variable var = {nullptr};

#ifdef HAVE_OPENGL
    bool gl_settings_changed = false;
#endif

    var.key = "melonds_console_mode";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "DSi"))
            Config::ConsoleType = ConsoleType::DSi;
        else
            Config::ConsoleType = ConsoleType::DS;
    }

    var.key = "melonds_boot_directly";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "disabled"))
            Config::DirectBoot = false;
        else
            Config::DirectBoot = true;
    }

    // TODO: Use standard melonDS config settings
    ScreenLayout layout = ScreenLayout::TopBottom;
    var.key = "melonds_screen_layout";
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
    var.key = "melonds_screen_gap";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {

        screen_layout_data.screen_gap_unscaled = std::stoi(var.value);
    }

#ifdef HAVE_OPENGL
    // TODO: Use standard melonDS config settings
    var.key = "melonds_hybrid_ratio";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != nullptr) {
        screen_layout_data.hybrid_ratio = std::stoi(var.value);
    }
#else
    screen_layout_data.hybrid_ratio = 2;
#endif

    // TODO: Use standard melonDS config settings
    var.key = "melonds_hybrid_small_screen";
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

    var.key = "melonds_swapscreen_mode";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != NULL) {
        if (strcmp(var.value, "Toggle") == 0)
            melonds::config::screen_swap_mode = ScreenSwapMode::Toggle;
        else
            melonds::config::screen_swap_mode = ScreenSwapMode::Hold;
    }

    var.key = "melonds_randomize_mac_address";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        config::_randomize_mac = string_is_equal(var.value, "enabled");
    }

#ifdef HAVE_THREADS
    var.key = "melonds_threaded_renderer";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "enabled"))
            config::_render_settings.Soft_Threaded = true;
        else
            config::_render_settings.Soft_Threaded = false;
    }
#endif

    TouchMode new_touch_mode = TouchMode::Disabled;

    var.key = "melonds_touch_mode";
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
        var.key = "melonds_opengl_renderer";
        if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
            Config::ScreenUseGL = string_is_equal(var.value, "enabled");

            if (!init && melonds::opengl::using_opengl())
                current_renderer = Config::ScreenUseGL ? CurrentRenderer::OpenGLRenderer : CurrentRenderer::Software;
        }
    }

    // Running the software rendering thread at the same time as OpenGL is used will cause segfault on cleanup
    if (config::_renderer_type == RendererType::OpenGl) config::_render_settings.Soft_Threaded = false;

    var.key = "melonds_opengl_resolution";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        int first_char_val = (int) var.value[0];
        int scaleing = std::clamp(first_char_val - 48, 0, 8);

        if (Config::GL_ScaleFactor != scaleing)
            gl_settings_changed = true;

        Config::GL_ScaleFactor = scaleing;
    } else {
        Config::GL_ScaleFactor = 1;
    }

    var.key = "melonds_opengl_better_polygons";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        bool enabled = string_is_equal(var.value, "enabled");
        gl_settings_changed |= enabled != Config::GL_BetterPolygons;

        Config::GL_BetterPolygons = enabled;
    }

    var.key = "melonds_opengl_filtering";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::ScreenFilter = string_is_equal(var.value, "linear");
    }

    if ((config::_renderer_type == RendererType::OpenGl && gl_settings_changed) || layout != current_screen_layout())
        melonds::opengl::refresh_opengl = true;
#endif

#ifdef JIT_ENABLED
    var.key = "melonds_jit_enable";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_Enable = string_is_equal(var.value, "enabled");
    }

    var.key = "melonds_jit_block_size";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_MaxBlockSize = std::stoi(var.value);
    }

    var.key = "melonds_jit_branch_optimisations";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_BranchOptimisations = (string_is_equal(var.value, "enabled"));
    }

    var.key = "melonds_jit_literal_optimisations";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_LiteralOptimisations = string_is_equal(var.value, "enabled");
    }

    var.key = "melonds_jit_fast_memory";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::JIT_FastMemory = string_is_equal(var.value, "enabled");
    }
#endif

    var.key = "melonds_dsi_sdcard";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::DSiSDEnable = string_is_equal(var.value, "enabled");
    }

    var.key = "melonds_mic_input";
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

    var.key = "melonds_need_button_mic_input";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        Config::Retro::MicButtonRequired = string_is_equal(var.value, "With Button");

//        if (noise_button_required &&
//            micInterface.interface_version &&
//            micHandle != NULL &&
//            !input_state.holding_noise_btn) { // If the player wants to require the noise button for mic input and they aren't already holding it...
//            micInterface.set_mic_state(micHandle, false);
//        }
    }

    var.key = "melonds_audio_bitrate";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "10-bit"))
            Config::AudioBitrate = 1;
        else if (string_is_equal(var.value, "16-bit"))
            Config::AudioBitrate = 2;
        else
            Config::AudioBitrate = 0;
    }

    var.key = "melonds_audio_interpolation";
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

    var.key = "melonds_use_fw_settings";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (string_is_equal(var.value, "disabled"))
            Config::FirmwareOverrideSettings = true;
        else
            Config::FirmwareOverrideSettings = false;
    }

    var.key = "melonds_language";
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
                "melonds_console_mode",
                "Console Mode",
                nullptr,
                "Whether melonDS should emulate a Nintendo DS or a Nintendo DSi. Some features may not be available in DSi mode.",
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
                "melonds_boot_directly",
                "Boot Game Directly",
                nullptr,
                "Whether melonDS should directly boot the game or enter the DS menu beforehand. If disabled, compatible BIOS and firmware files must be provided in the system directory.",
                nullptr,
                "system",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "enabled"
        },
        {
                "melonds_use_fw_settings",
                "Use Firmware Settings",
                nullptr,
                "Use language and username specified in the DS firmware, rather than those provided by the frontend. If disabled or the firmware is unavailable, these values will be provided by the frontend. If a name couldn't be found, \"melonDS\" will be used as the default.",
                nullptr,
                "system",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "disabled"
        },
        {
                "melonds_language",
                "Language",
                nullptr,
                "Selected language will be used if 'Use Firmware Settings' is disabled or if firmware file was not found.",
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
                "melonds_randomize_mac_address",
                "Randomize MAC Address",
                nullptr,
                nullptr,
                nullptr,
                "system",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "disabled"
        },
        {
                "melonds_dsi_sdcard",
                "Enable DSi SD Card",
                nullptr,
                nullptr,
                nullptr,
                "system",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "disabled"
        },
#ifdef HAVE_THREADS
        {
                "melonds_threaded_renderer",
                "Threaded Software Renderer",
                nullptr,
                nullptr,
                nullptr,
                "video",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "disabled"
        },
#endif
#ifdef HAVE_OPENGL
        {
                "melonds_opengl_renderer",
                "OpenGL Renderer",
                nullptr,
                "Restart required.",
                nullptr,
                "video",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "disabled"
        },
        {
                "melonds_opengl_resolution",
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
                "melonds_opengl_better_polygons",
                "OpenGL Improved Polygon Splitting",
                nullptr,
                nullptr,
                nullptr,
                "video",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "disabled"
        },
        {
                "melonds_opengl_filtering",
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
                "melonds_mic_input",
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
                "melonds_need_button_mic_input",
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
                "melonds_audio_bitrate",
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
                "melonds_audio_interpolation",
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
                "melonds_touch_mode",
                "Touch Mode",
                nullptr,
                "Choose mode for interactions with the touch screen.",
                nullptr,
                "screen",
                {
                        {"Mouse", nullptr},
                        {"Touch", nullptr},
                        {"Joystick", nullptr},
                        {"disabled", nullptr},
                        {nullptr, nullptr},
                },
                "Mouse"
        },
        {
                "melonds_swapscreen_mode",
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
                "melonds_screen_layout",
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
                "melonds_screen_gap",
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
                "melonds_hybrid_small_screen",
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
                "melonds_hybrid_ratio",
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
                "melonds_jit_enable",
                "JIT Enable (Restart)",
                nullptr,
                nullptr,
                nullptr,
                "cpu",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "enabled"
        },
        {
                "melonds_jit_block_size",
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
                "melonds_jit_branch_optimisations",
                "JIT Branch Optimisations",
                nullptr,
                nullptr,
                nullptr,
                "cpu",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "enabled"
        },
        {
                "melonds_jit_literal_optimisations",
                "JIT Literal Optimisations",
                nullptr,
                nullptr,
                nullptr,
                "cpu",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "enabled"
        },
        {
                "melonds_jit_fast_memory",
                "JIT Fast Memory",
                nullptr,
                nullptr,
                nullptr,
                "cpu",
                {
                        {"disabled", nullptr},
                        {"enabled", nullptr},
                        {nullptr, nullptr},
                },
                "enabled"
        },
#endif
        {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, {{0}}, nullptr},
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