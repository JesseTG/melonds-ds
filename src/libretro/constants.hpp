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

#pragma once

#include <cstdint>

#include "std/chrono.hpp"

namespace MelonDsDs {

    constexpr double FPS = 33513982.0 / 560190.0; // In frames per second
    constexpr double SAMPLE_RATE =  33513982.0 / 1024.0; // In Hz
    constexpr std::chrono::microseconds US_PER_FRAME {static_cast<int64_t>(1000000.0 / FPS)};
}