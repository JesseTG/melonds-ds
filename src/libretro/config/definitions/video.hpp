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

#ifndef MELONDS_DS_VIDEO_HPP
#define MELONDS_DS_VIDEO_HPP

#include <initializer_list>
#include <libretro.h>

#include "../constants.hpp"

namespace MelonDsDs::config::definitions {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    constexpr retro_core_option_v2_definition RenderMode {
        config::video::RENDER_MODE,
        "Render Mode",
        nullptr,
        "Software mode is faster and more accurate, "
        "while OpenGL mode supports scaling up "
        "the resolution of 3D graphics in most cases.\n"
        "\n"
        "OpenGL mode may be buggy on some graphics hardware. "
        "If it doesn't work, software rendering is used as a fallback. "
        "Changes take effect immediately "
        "but may require the frontend's video driver to be restarted.",
        nullptr,
        config::video::CATEGORY,
        {
            {MelonDsDs::config::values::SOFTWARE, "Software"},
            {MelonDsDs::config::values::OPENGL, "OpenGL"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::SOFTWARE
    };

    constexpr retro_core_option_v2_definition OpenGlScaleFactor {
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
    };

    constexpr retro_core_option_v2_definition OpenGlBetterPolygons {
        config::video::OPENGL_BETTER_POLYGONS,
        "Improved Polygon Splitting",
        nullptr,
        "Enable this if your game's 3D models are not rendering correctly. "
        "OpenGL renderer only.",
        nullptr,
        config::video::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };

    constexpr retro_core_option_v2_definition OpenGlFiltering {
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
            {MelonDsDs::config::values::NEAREST, "Nearest"},
            {MelonDsDs::config::values::LINEAR, "Linear"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::NEAREST
    };
#endif
#if defined(HAVE_THREADS) && defined(HAVE_THREADED_RENDERER)
    constexpr retro_core_option_v2_definition ThreadedSoftwareRenderer {
        config::video::THREADED_RENDERER,
        "Threaded Software Renderer",
        nullptr,
        "If enabled, the software renderer will run on a separate thread. "
        "Changes take effect immediately. "
        "If unsure, leave this enabled.",
        nullptr,
        config::video::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::ENABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };
#endif

    constexpr std::initializer_list<retro_core_option_v2_definition> VideoOptionDefinitions {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
        RenderMode,
        OpenGlScaleFactor,
        OpenGlBetterPolygons,
        OpenGlFiltering,
#endif
#if defined(HAVE_THREADS) && defined(HAVE_THREADED_RENDERER)
        ThreadedSoftwareRenderer,
#endif
    };
}


#endif //MELONDS_DS_VIDEO_HPP
