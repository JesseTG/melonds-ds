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
#include "config/definitions.hpp"
#include "config/constants.hpp"

#include <libretro.h>
#include <retro_miscellaneous.h>

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
struct retro_core_option_v2_definition melonds::FixedOptionDefinitions[] = {
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
        {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, {{nullptr, nullptr}}, nullptr},
};

const size_t melonds::FixedOptionDefinitionsLength = ARRAY_SIZE(FixedOptionDefinitions) - 1;


struct retro_core_options_v2 melonds::options_us = {
        option_cats_us,
        FixedOptionDefinitions
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