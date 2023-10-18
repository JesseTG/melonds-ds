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

#include "threads.hpp"

#include <new>
#include <stdexcept>

retro::slock::slock() {
    mutex = slock_new();
    if (!mutex) {
        throw std::bad_alloc();
    }
}

retro::slock::~slock() noexcept {
    if (mutex) {
        slock_free(mutex);
    }
    mutex = nullptr;
}

void retro::slock::lock() noexcept {
    slock_lock(mutex);
}

void retro::slock::unlock() noexcept {
    slock_unlock(mutex);
}

bool retro::slock::try_lock() noexcept {
    return slock_try_lock(mutex);
}