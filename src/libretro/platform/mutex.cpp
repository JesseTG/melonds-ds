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

#include "retro/threads.hpp"
#include "tracy.hpp"

using Platform::Mutex;

struct Platform::Mutex {
    TracyLockable(retro::slock, mutex);
};

Mutex* Platform::Mutex_Create()
{
    ZoneScopedN(TracyFunction);
    return new Mutex;
}

void Platform::Mutex_Free(Mutex* mutex)
{
    ZoneScopedN(TracyFunction);
    delete mutex;
}

void Platform::Mutex_Lock(Mutex* mutex)
{
    ZoneScopedN(TracyFunction);
    mutex->mutex.lock();
}

void Platform::Mutex_Unlock(Mutex* mutex)
{
    ZoneScopedN(TracyFunction);
    mutex->mutex.unlock();
}
