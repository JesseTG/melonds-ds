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

#ifndef MELONDS_DS_UTILS_HPP
#define MELONDS_DS_UTILS_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <libretro.h>

#if defined(_WIN32)
#define PLATFORM_DIR_SEPERATOR '\\'
#else
#define PLATFORM_DIR_SEPERATOR  '/'
#endif

namespace MelonDsDs {
    void GetGameName(const struct retro_game_info& game_info, char* game_name, size_t game_name_size) noexcept;

    template<class T>
    using optional_ref = std::optional<std::reference_wrapper<T>>;
}

#endif //MELONDS_DS_UTILS_HPP
