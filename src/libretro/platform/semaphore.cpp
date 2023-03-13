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
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <Platform.h>
#include "../../rthreads/rsemaphore.h"

using Platform::Semaphore;

Semaphore *Platform::Semaphore_Create()
{
#ifdef HAVE_THREADS
    ssem_t *sem = ssem_new(0);
    if (sem)
        return (Semaphore*)sem;
#endif
    return nullptr;
}

void Platform::Semaphore_Reset(Semaphore *sema)
{
#ifdef HAVE_THREADS
    while (ssem_get((ssem_t*)sema) > 0) {
        ssem_trywait((ssem_t*)sema);
    }
#endif
}

void Platform::Semaphore_Post(Semaphore *sema, int count)
{
#ifdef HAVE_THREADS
    for (int i = 0; i < count; i++)
    {
        ssem_signal((ssem_t*)sema);
    }
#endif
}

void Platform::Semaphore_Wait(Semaphore *sema)
{
#ifdef HAVE_THREADS
    ssem_wait((ssem_t*)sema);
#endif
}

void Platform::Semaphore_Free(Semaphore *sema)
{

#ifdef HAVE_THREADS
    auto *sem = (ssem_t*)sema;
    if (sem)
        ssem_free(sem);
#endif
}

