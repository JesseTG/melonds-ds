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

#ifndef MELONDS_DS_GBA_HPP
#define MELONDS_DS_GBA_HPP

#include <memory>
#include <optional>

#include <libretro.h>
#include <streams/file_stream.h>
#include <Platform.h>

#include "retro/task_queue.hpp"

struct retro_game_info;

namespace melonds {
    class SaveManager;

    namespace gba {

        extern std::unique_ptr<SaveManager> GbaSaveManager;

        void FlushSram(const retro_game_info& gba_save_info) noexcept;

        retro::task::TaskSpec FlushTask() noexcept;
    }

}

#endif //MELONDS_DS_GBA_HPP
