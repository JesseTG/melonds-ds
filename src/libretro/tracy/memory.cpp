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

#include <tracy/Tracy.hpp>

// Defining these functions in the global scope
// overrides operator new and operator delete
// for all linked translation units.

void* operator new(std::size_t count)
{
    if (count == 0)
        ++count; // avoid std::malloc(0) which may return nullptr on success

    if (void *ptr = std::malloc(count)) {
        TracySecureAlloc(ptr, count);
        return ptr;
    }

    throw std::bad_alloc{}; // required by [new.delete.single]/3
}

void operator delete(void* ptr) noexcept
{
    TracySecureFree(ptr);
    std::free(ptr);
}
