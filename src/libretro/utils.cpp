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

#include "utils.hpp"

#include <compat/strl.h>
#include <file/file_path.h>


void melonds::GetGameName(const struct retro_game_info& game_info, char* game_name, size_t game_name_size) noexcept {
    memset(game_name, 0, game_name_size);

    const char* basename = path_basename(game_info.path);
    strlcpy(game_name, basename ? basename : game_info.path, game_name_size);
    path_remove_extension(game_name);

    // TODO: Handle errors
}
