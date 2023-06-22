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

#include "content.hpp"
#include "environment.hpp"

using std::optional;
using std::nullopt;

namespace retro::content {

    static optional<struct retro_game_info> _loaded_nds_info;
    static optional<struct retro_game_info_ext> _loaded_nds_info_ext;
    static optional<struct retro_game_info> _loaded_gba_info;
    static optional<struct retro_game_info_ext> _loaded_gba_info_ext;
}

const optional<struct retro_game_info>& retro::content::get_loaded_nds_info() {
    return _loaded_nds_info;
}

const optional<struct retro_game_info_ext>& retro::content::get_loaded_nds_info_ext() {
    return _loaded_nds_info_ext;
}

const optional<struct retro_game_info>& retro::content::get_loaded_gba_info() {
    return _loaded_gba_info;
}

const optional<struct retro_game_info_ext>& retro::content::get_loaded_gba_info_ext() {
    return _loaded_gba_info_ext;
}

void retro::content::set_loaded_content_info(
    const struct retro_game_info *nds_info,
    const struct retro_game_info *gba_info
) noexcept {
    return set_loaded_content_info(nds_info, gba_info, nullptr);
}

void retro::content::set_loaded_content_info(
    const struct retro_game_info *nds_info,
    const struct retro_game_info *gba_info,
    const struct retro_game_info *gba_save_info
) noexcept {
    if (nds_info) {
        _loaded_nds_info = *nds_info;
    }

    if (gba_info) {
        _loaded_gba_info = *gba_info;
    }

    const struct retro_game_info_ext *info_array;
    if (environment(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &info_array) && info_array) {
        // If the frontend supports extended game info, and has any to give...
        _loaded_nds_info_ext = info_array[0];

        if (gba_info) {
            _loaded_gba_info_ext = info_array[1];
        }
    }
}

void retro::content::clear() noexcept {
    _loaded_nds_info = nullopt;
    _loaded_nds_info_ext = nullopt;
    _loaded_gba_info = nullopt;
    _loaded_gba_info_ext = nullopt;
}