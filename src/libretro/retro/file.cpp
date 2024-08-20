/*
    Copyright 2024 Jesse Talavera

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

#include "file.hpp"

#include <streams/file_stream.h>

void retro::rfile_deleter::operator()(RFILE* file) const noexcept {
    filestream_close(file);
}

retro::rfile_ptr retro::make_rfile(const char *path, unsigned mode, unsigned hints) noexcept {
    return rfile_ptr(filestream_open(path, mode, hints));
}

retro::rfile_ptr retro::make_rfile(std::string_view path, unsigned mode, unsigned hints) noexcept {
    return make_rfile(path.data(), mode, hints);
}

retro::rfile_ptr retro::make_rfile(const char *path, unsigned mode) noexcept {
    return make_rfile(path, mode, RETRO_VFS_FILE_ACCESS_HINT_NONE);
}

retro::rfile_ptr retro::make_rfile(std::string_view path, unsigned mode) noexcept {
    return make_rfile(path.data(), mode);
}
