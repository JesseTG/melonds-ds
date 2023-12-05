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
#include <semaphore>

#include "tracy.hpp"

using std::counting_semaphore;

using namespace melonDS;
using Platform::Semaphore;

struct Platform::Semaphore {
    counting_semaphore<> semaphore;

    Semaphore() : semaphore(0) {}
};

Semaphore *Platform::Semaphore_Create()
{
    ZoneScopedN(TracyFunction);
    return new Semaphore;
}

void Platform::Semaphore_Reset(Semaphore *sema)
{
    ZoneScopedN(TracyFunction);
    while (sema->semaphore.try_acquire());
}

void Platform::Semaphore_Post(Semaphore *sema, int count)
{
    ZoneScopedN(TracyFunction);
    sema->semaphore.release(count);
}

void Platform::Semaphore_Wait(Semaphore *sema)
{
    ZoneScopedN(TracyFunction);
    sema->semaphore.acquire();
}

void Platform::Semaphore_Free(Semaphore *sema)
{
    ZoneScopedN(TracyFunction);
    delete sema;
}

