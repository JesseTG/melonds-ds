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

#ifndef MELONDSDS_RETRO_GAMEINFO_HPP
#define MELONDSDS_RETRO_GAMEINFO_HPP

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>

struct retro_game_info;

namespace retro {

    class GameInfo {
    public:
        GameInfo(const retro_game_info& info) noexcept;

        std::string_view GetPath() const noexcept { return _path; }
        std::span<const std::byte> GetData() const noexcept {
            return std::span(_data.get(), _size);
        }
        std::string_view GetMeta() const noexcept { return _meta; }
    private:
        std::string _path;
        std::unique_ptr<std::byte[]> _data;
        size_t _size;
        std::string _meta;
    };

} // retro

#endif // MELONDSDS_RETRO_GAMEINFO_HPP
