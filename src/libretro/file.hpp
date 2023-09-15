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

#ifndef MELONDS_DS_FILE_HPP
#define MELONDS_DS_FILE_HPP

#include <Platform.h>
#include <streams/file_stream.h>

#include "retro/task_queue.hpp"

struct Platform::FileHandle {
    RFILE *file;
    unsigned hints;
};

namespace melonds::file {
    void init();
    void reset() noexcept;
    void deinit();

    [[deprecated("Each kind of file will get its own flush task")]]
    retro::task::TaskSpec FlushTask() noexcept;
}

#endif //MELONDS_DS_FILE_HPP
