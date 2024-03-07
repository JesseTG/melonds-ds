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

#include "test.hpp"

#include <string/stdstring.h>

#include "environment.hpp"

extern "C" int libretropy_add_integers(int a, int b) {
    return a + b;
}

extern "C" const char* libretropy_get_system_directory() {
    const char* path = nullptr;
    bool ok = retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &path);
    return ok ? path : nullptr;
}

extern "C" const char* libretropy_get_save_directory() {
    const char* path = nullptr;
    bool ok = retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &path);
    return ok ? path : nullptr;
}

retro_proc_address_t MelonDsDs::GetProcAddress(const char* sym) noexcept {
    if (string_is_equal(sym, "libretropy_add_integers"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_add_integers);

    if (string_is_equal(sym, "libretropy_get_system_directory"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_system_directory);

    if (string_is_equal(sym, "libretropy_get_save_directory"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_save_directory);

    return nullptr;
}

