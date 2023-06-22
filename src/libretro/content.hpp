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

#ifndef MELONDS_DS_CONTENT_HPP
#define MELONDS_DS_CONTENT_HPP

#include <optional>
#include <string>

#include <libretro.h>

namespace retro::content {
    const std::optional<struct retro_game_info>& get_loaded_nds_info();
    const std::optional<std::string>& get_loaded_nds_path();
    const std::optional<struct retro_game_info_ext>& get_loaded_nds_info_ext();
    const std::optional<std::string>& get_loaded_gba_path();
    const std::optional<struct retro_game_info>& get_loaded_gba_info();
    const std::optional<struct retro_game_info_ext>& get_loaded_gba_info_ext();
    const std::optional<struct retro_game_info>& get_loaded_gba_save_info();
    const std::optional<std::string>& get_loaded_gba_save_path();

    void set_loaded_content_info(
        const struct retro_game_info* nds_info,
        const struct retro_game_info* gba_info
    ) noexcept;

    void set_loaded_content_info(
        const struct retro_game_info* nds_info,
        const struct retro_game_info* gba_info,
        const struct retro_game_info* gba_save_info
    ) noexcept;

    void clear() noexcept;
}

#endif //MELONDS_DS_CONTENT_HPP
