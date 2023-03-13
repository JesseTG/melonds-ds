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
#include <rthreads/rthreads.h>

using Platform::Mutex;

Mutex* Platform::Mutex_Create()
{
#ifdef HAVE_THREADS
    return (Mutex*)slock_new();
#endif
    return nullptr;
}

void Platform::Mutex_Free(Mutex* mutex)
{
#ifdef HAVE_THREADS
    slock_free((slock_t*)mutex);
#endif
}

void Platform::Mutex_Lock(Mutex* mutex)
{
#ifdef HAVE_THREADS
    slock_lock((slock_t*)mutex);
#endif
}

void Platform::Mutex_Unlock(Mutex* mutex)
{
#ifdef HAVE_THREADS
    slock_unlock((slock_t*)mutex);
#endif
}

bool Platform::Mutex_TryLock(Mutex* mutex)
{
#ifdef HAVE_THREADS
    slock_try_lock((slock_t*)mutex);
#endif
    return true;
}