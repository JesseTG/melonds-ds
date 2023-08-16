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

#include <retro_assert.h>
#include "content.hpp"
#include "environment.hpp"

using std::optional;
using std::nullopt;
using std::string;
using std::make_optional;

namespace retro::content {
    // We maintain copies of some of the strings in retro_game_info
    // just in case the frontend frees them at the end of retro_load/retro_load_special.
    static optional<struct retro_game_info> _loaded_nds_info;
    static optional<struct retro_game_info_ext> _loaded_nds_info_ext;
    static optional<string> _loaded_nds_path;
    static optional<struct retro_game_info> _loaded_gba_info;
    static optional<struct retro_game_info_ext> _loaded_gba_info_ext;
    static optional<string> _loaded_gba_path;
    static optional<struct retro_game_info> _loaded_gba_save_info;
    static optional<string> _loaded_gba_save_path;
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

const optional<struct retro_game_info>& retro::content::get_loaded_gba_save_info() {
    return _loaded_gba_save_info;
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
    retro_assert(_loaded_nds_info == nullopt);
    retro_assert(_loaded_nds_info_ext == nullopt);
    retro_assert(_loaded_nds_path == nullopt);
    retro_assert(_loaded_gba_info == nullopt);
    retro_assert(_loaded_gba_info_ext == nullopt);
    retro_assert(_loaded_gba_path == nullopt);
    retro_assert(_loaded_gba_save_info == nullopt);
    retro_assert(_loaded_gba_save_path == nullopt);

    if (nds_info) {
        _loaded_nds_path = nds_info->path ? make_optional(nds_info->path) : nullopt;
        _loaded_nds_info = retro_game_info {
            .path = _loaded_nds_path ? _loaded_nds_path->c_str() : nullptr,
            .data = nds_info->data,
            .size = nds_info->size,
            .meta = nds_info->meta,
        };
    } else {
        _loaded_nds_info = nullopt;
        _loaded_nds_path = nullopt;
    }

    if (gba_info) {
        _loaded_gba_path = gba_info->path ? make_optional(gba_info->path) : nullopt;
        _loaded_gba_info = retro_game_info {
            .path = _loaded_gba_path ? _loaded_gba_path->c_str() : nullptr,
            .data = gba_info->data,
            .size = gba_info->size,
            .meta = gba_info->meta,
        };
    } else {
        _loaded_gba_info = nullopt;
        _loaded_gba_path = nullopt;
    }

    if (gba_save_info && gba_save_info->data) {
        // If a subsystem ROM is optional, then libretro might provide a zeroed-out retro_game_info
        _loaded_gba_save_path = gba_save_info->path ? make_optional(gba_save_info->path) : nullopt;
        _loaded_gba_save_info = retro_game_info {
            .path = _loaded_gba_save_path ? _loaded_gba_save_path->c_str() : nullptr,
            .data = gba_save_info->data,
            .size = gba_save_info->size,
            .meta = gba_save_info->meta,
        };
    } else {
        _loaded_gba_save_info = nullopt;
        _loaded_gba_save_path = nullopt;
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
    _loaded_nds_path = nullopt;
    _loaded_gba_path = nullopt;
    _loaded_gba_save_info = nullopt;
    _loaded_gba_save_path = nullopt;
}