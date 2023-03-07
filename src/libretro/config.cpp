//
// Created by Jesse on 3/6/2023.
//

#include <cstring>
#include "libretro.hpp"
#include "environment.hpp"
#include "config.hpp"

namespace melonds::config {
    static bool opengl_options = true;
    static bool hybrid_options = true;
#ifdef JIT_ENABLED
    static bool jit_options = true;
#endif
}

bool melonds::update_option_visibility()
{
    using retro::environment;
    using namespace melonds::config;
    struct retro_core_option_display option_display{};
    struct retro_variable var{};
    bool updated = false;

#ifdef HAVE_OPENGL
    // Show/hide OpenGL core options
    bool opengl_options_prev = opengl_options;

    opengl_options = true;
    var.key = "melonds_opengl_renderer";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !strcmp(var.value, "disabled"))
        opengl_options = false;

    if (opengl_options != opengl_options_prev)
    {
        option_display.visible = opengl_options;

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
    bool hybrid_options_prev = hybrid_options;

    hybrid_options = true;
    var.key = "melonds_screen_layout";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && (strcmp(var.value, "Hybrid Top") && strcmp(var.value, "Hybrid Bottom")))
        hybrid_options = false;

    if (hybrid_options != hybrid_options_prev)
    {
        option_display.visible = hybrid_options;

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
    bool jit_options_prev = jit_options;

    jit_options = true;
    var.key = "melonds_jit_enable";
    if (environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !strcmp(var.value, "disabled"))
        jit_options = false;

    if (jit_options != jit_options_prev)
    {
        option_display.visible = jit_options;

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
                "Whether melonDS should directly boot the game or enter the DS menu beforehand. If disabled, compatible BIOS and firmware files are required.",
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