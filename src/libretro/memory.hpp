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

#ifndef MELONDS_DS_MEMORY_HPP
#define MELONDS_DS_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <types.h>
#include <memory>
#include <optional>

namespace melonds {
    using std::size_t;

    constexpr size_t DEFAULT_SERIALIZE_TEST_SIZE = 16 * 1024 * 1024; // 16 MiB

    constexpr unsigned NINTENDO_DS_MEMORY_SAVE_RAM = 0x101;

    void clear_memory_config();


}
#endif //MELONDS_DS_MEMORY_HPP
