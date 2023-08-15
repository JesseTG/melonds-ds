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

#include "dirent.hpp"

#include <cstring>
#include <string>

#include <file/file_path.h>

retro::file_tree retro::readdir(const std::string &path, bool hidden) noexcept {
    return file_tree(path, hidden);
}

retro::file_tree::file_tree(const std::string &path, bool hidden) noexcept: originalPath(path) {
    dir = retro_opendir_include_hidden(path.c_str(), hidden);
}

retro::file_tree::~file_tree() noexcept {
    if (dir) {
        retro_closedir(dir);
    }
}

retro::file_tree::dirent_iterator retro::file_tree::begin() noexcept {
    return dir ? dirent_iterator(this) : dirent_iterator(nullptr);
}

retro::file_tree::dirent_iterator retro::file_tree::end() const noexcept {
    return dirent_iterator(nullptr);
}

retro::file_tree::dirent_iterator::dirent_iterator(file_tree *ptr) noexcept: m_ptr(ptr) {
    if (m_ptr) {
        ++(*this); // Find the first file
    } else {
        memset((void *) current.name, 0, sizeof(current.name));
        memset((void *) current.path, 0, sizeof(current.path));
        current.size = 0;
        current.flags = 0;
    }
}

retro::file_tree::dirent_iterator retro::file_tree::dirent_iterator::operator++(int) noexcept {
    dirent_iterator tmp = *this;
    ++(*this);
    return tmp;
}

// Go to the next file
retro::file_tree::dirent_iterator &retro::file_tree::dirent_iterator::operator++() noexcept {
    if (!m_ptr) {
        // If this iterator is at its end...
        return *this;
    }

    bool done = false;
    do {
        bool hasNext = retro_readdir(m_ptr->dir);
        if (!hasNext) {
            m_ptr = nullptr;
            memset((void *) current.path, 0, sizeof(current.path));
            memset((void *) current.name, 0, sizeof(current.name));
            current.flags = 0;
            current.size = 0;
            break;
        }

        const char *fileName = retro_dirent_get_name(m_ptr->dir);
        char filePath[PATH_MAX];
        size_t filePathLength = fill_pathname_join_special(filePath, m_ptr->originalPath.c_str(), fileName,
                                                           sizeof(filePath));
        int32_t size = 0;
        int flags = retro_vfs_stat_impl(filePath, &size);
        if (is_regular_file(flags)) {
            // If we've found the next file to return to whoever's using this iterator...
            strncpy(current.name, fileName, sizeof(current.name));
            strncpy(current.path, filePath, sizeof(current.path));
            current.flags = flags;
            current.size = size;
            done = true;
        }
    } while (!done);

    return *this;
}