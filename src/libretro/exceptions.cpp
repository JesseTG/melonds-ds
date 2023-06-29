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

#include "exceptions.hpp"

#include <sstream>

static std::string construct_missing_bios_message(const std::vector<std::string>& bios_files) {
    std::stringstream error;

    error << "Missing these BIOS files: ";
    for (size_t i = 0; i < bios_files.size(); ++i) {
        const std::string& file = bios_files[i];
        error << file;
        if (i < bios_files.size() - 1) {
            // If this isn't the last missing BIOS file...
            error << ", ";
        }
    }

    return error.str();
}

melonds::missing_bios_exception::missing_bios_exception(const std::vector<std::string>& bios_files)
    : emulator_exception(construct_missing_bios_message(bios_files)) {
}