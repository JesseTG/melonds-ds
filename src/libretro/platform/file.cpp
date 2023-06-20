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

#include <Platform.h>

#include <utility>
#include "../environment.hpp"
#include "../utils.hpp"

FILE *Platform::OpenFile(const std::string& path, const std::string& mode, bool mustexist) {
    FILE *ret;

    if (mustexist) {
        ret = fopen(path.c_str(), "rb");
        if (ret) fclose(ret);
        ret = fopen(path.c_str(), mode.c_str());
    } else
        ret = fopen(path.c_str(), mode.c_str());

    return ret;
}

FILE *Platform::OpenLocalFile(const std::string& path, const std::string& mode) {
    std::string directory = retro::get_system_directory().value_or("");
    std::string fullpath = directory + PLATFORM_DIR_SEPERATOR + path;
    FILE* f = OpenFile(fullpath, mode, true);
    return f;
}

FILE *Platform::OpenDataFile(const std::string& path) {
    return OpenLocalFile(path, "rb");
}