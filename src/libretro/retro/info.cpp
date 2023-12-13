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

#include "info.hpp"

#include <cstring>
#include <libretro.h>

retro::GameInfo::GameInfo(const retro_game_info& info) noexcept :
    _path(info.path ? info.path : ""),
    _data(info.data && info.size ? std::make_unique<std::byte[]>(info.size) : nullptr),
    _size(info.size),
    _meta(info.meta ? info.meta : "")
{
    if (_data) {
        memcpy(_data.get(), info.data, info.size);
    }
}

