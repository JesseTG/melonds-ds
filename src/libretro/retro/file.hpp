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

#ifndef MELONDSDS_RETRO_FILE_HPP
#define MELONDSDS_RETRO_FILE_HPP

#include <memory>
#include <string_view>

struct RFILE;

namespace retro {
    // Inspired by https://biowpn.github.io/bioweapon/2024/03/05/raii-all-the-things.html
    struct rfile_deleter {
        void operator()(RFILE* file) const noexcept;
    };

    using rfile_ptr = std::unique_ptr<RFILE, rfile_deleter>;

    rfile_ptr make_rfile(const char *path, unsigned mode, unsigned hints) noexcept;
    rfile_ptr make_rfile(std::string_view path, unsigned mode, unsigned hints) noexcept;
    rfile_ptr make_rfile(const char *path, unsigned mode) noexcept;
    rfile_ptr make_rfile(std::string_view path, unsigned mode) noexcept;
}


#endif // MELONDSDS_RETRO_FILE_HPP
