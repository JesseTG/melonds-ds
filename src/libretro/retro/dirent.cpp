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
#include <compat/strl.h>

#include "tracy.hpp"

retro::dirent_tree retro::readdir(const std::string &path, bool hidden) noexcept {
    ZoneScopedN(TracyFunction);
    ZoneText(path.c_str(), path.size());
    return dirent_tree(path, hidden);
}

retro::dirent_tree::dirent_tree(const std::string &path, bool hidden) noexcept: originalPath(path) {
    ZoneScopedN(TracyFunction);
    ZoneText(path.c_str(), path.size());
    dir = retro_opendir_include_hidden(path.c_str(), hidden);
}

retro::dirent_tree::~dirent_tree() noexcept {
    ZoneScopedN(TracyFunction);
    if (dir) {
        retro_closedir(dir);
    }
}

retro::dirent_tree::dirent_iterator retro::dirent_tree::begin() noexcept {
    return dir ? dirent_iterator(this) : dirent_iterator(nullptr);
}

retro::dirent_tree::dirent_iterator retro::dirent_tree::end() const noexcept {
    return dirent_iterator(nullptr);
}

retro::dirent_tree::dirent_iterator::dirent_iterator(dirent_tree *ptr) noexcept: m_ptr(ptr) {
    ZoneScopedN(TracyFunction);
    if (m_ptr) {
        ++(*this); // Find the first file
    } else {
        memset((void *) current.path, 0, sizeof(current.path));
        current.size = 0;
        current.flags = 0;
    }
}

retro::dirent_tree::dirent_iterator retro::dirent_tree::dirent_iterator::operator++(int) noexcept {
    dirent_iterator tmp = *this;
    ++(*this);
    return tmp;
}

// Go to the next file
retro::dirent_tree::dirent_iterator &retro::dirent_tree::dirent_iterator::operator++() noexcept {
    ZoneScopedN(TracyFunction);
    if (!m_ptr) {
        // If this iterator is at its end...
        return *this;
    }

    bool done = false;
    do {
        ZoneScopedN("retro::dirent_tree::dirent_iterator::operator++::do");
        bool hasNext;
        {
            ZoneScopedN("retro_readdir");
            hasNext = retro_readdir(m_ptr->dir);
        }
        if (!hasNext) {
            m_ptr = nullptr;
            memset((void *) current.path, 0, sizeof(current.path));
            current.flags = 0;
            current.size = 0;
            break;
        }

        const char *fileName;
        {
            ZoneScopedN("retro_dirent_get_name");
            fileName = retro_dirent_get_name(m_ptr->dir);
        }
        char filePath[PATH_MAX];
        {
            ZoneScopedN("fill_pathname_join_special");
            size_t filePathLength = fill_pathname_join_special(filePath, m_ptr->originalPath.c_str(), fileName, sizeof(filePath));
            if (filePathLength >= sizeof(filePath)) {
                // If the path is too long...
                // TODO: Log that this path is being skipped
                continue;
            }
        }

        int flags;
        {
            ZoneScopedN("path_stat");
            flags = path_stat(filePath);
        }
        if (is_regular_file(flags)) {
            // If we've found the next file to return to whoever's using this iterator...
            strlcpy(current.path, filePath, sizeof(current.path));
            current.flags = flags;
            {
                ZoneScopedN("path_get_size");
                current.size = path_get_size(filePath);
            }
            done = true;
        }
    } while (!done);

    return *this;
}