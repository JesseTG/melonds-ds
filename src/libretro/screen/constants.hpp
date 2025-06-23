/**
    Copyright 2025 Jesse Talavera

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

#pragma once

#include <glm/vec2.hpp>

namespace MelonDsDs {

    /// The native width of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_WIDTH = 256;

    /// The native height of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_HEIGHT = 192;

    template<typename T>
    constexpr glm::tvec2<T> NDS_SCREEN_SIZE(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT);

    template<typename T>
    constexpr T NDS_SCREEN_AREA = NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT;

    // We require a pixel format of RETRO_PIXEL_FORMAT_XRGB8888, so we can assume 4 bytes here
    constexpr int PIXEL_SIZE = 4;

    template<typename T>
    constexpr T RETRO_MAX_POINTER_COORDINATE = 32767;

}