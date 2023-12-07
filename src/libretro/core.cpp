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

#include "core.hpp"

#include <libretro.h>
#include <retro_assert.h>

#include <NDS.h>

MelonDsDs::CoreState MelonDsDs::Core(false);

MelonDsDs::CoreState::CoreState(bool init) noexcept : initialized(init) {
}

MelonDsDs::CoreState::~CoreState() noexcept {
    Console = nullptr;
    initialized = false;
}

retro_system_av_info MelonDsDs::CoreState::GetSystemAvInfo() const noexcept {
#ifndef NDEBUG
    if (!_messageScreen) {
        retro_assert(Console != nullptr);
    }
#endif

    return {
        .timing {
            .fps = 32.0f * 1024.0f * 1024.0f / 560190.0f,
            .sample_rate = 32.0f * 1024.0f,
        },
        .geometry = _screenLayout.Geometry(Console->GPU.GetRenderer3D()),
    };
}