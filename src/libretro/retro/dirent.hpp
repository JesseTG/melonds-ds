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

#ifndef MELONDS_DS_DIRENT_HPP
#define MELONDS_DS_DIRENT_HPP

#include <cstdint>
#include <iterator>

#include <retro_dirent.h>
#include <vfs/vfs_implementation.h>

namespace retro {
    constexpr bool is_regular_file(int flags) noexcept {
        return flags & RETRO_VFS_STAT_IS_VALID && !(flags & (RETRO_VFS_STAT_IS_DIRECTORY | RETRO_VFS_STAT_IS_CHARACTER_SPECIAL));
    }

    struct dirent {
        char name[FILENAME_MAX];
        char path[PATH_MAX];
        int32_t size;
        int flags;

        dirent() {
            memset((void *) name, 0, sizeof(name));
            memset((void *) path, 0, sizeof(path));
            size = 0;
            flags = 0;
        }
    };

    struct file_tree {
        using value_type = dirent;

        file_tree(const std::string& path, bool hidden) noexcept;
        ~file_tree() noexcept;

        struct dirent_iterator {
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = dirent;
            using pointer = const value_type*;
            using reference = const value_type&;

            dirent_iterator(file_tree* ptr) noexcept;
            dirent_iterator& operator++() noexcept;
            dirent_iterator operator++(int) noexcept;

            reference operator*() const noexcept { return current; }
            pointer operator->() const noexcept { return &current; }

            friend bool operator== (const dirent_iterator& a, const dirent_iterator& b) { return a.m_ptr == b.m_ptr; }
            friend bool operator!= (const dirent_iterator& a, const dirent_iterator& b) { return a.m_ptr != b.m_ptr; }
        private:
            file_tree* m_ptr;
            dirent current;
        };

        dirent_iterator begin() noexcept;
        dirent_iterator end() const noexcept;
    private:
        RDIR* dir;
        std::string originalPath;
    };

    file_tree readdir(const std::string& path, bool hidden) noexcept;
}

#endif //MELONDS_DS_DIRENT_HPP
