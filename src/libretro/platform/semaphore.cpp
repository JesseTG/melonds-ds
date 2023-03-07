//
// Created by Jesse on 3/6/2023.
//
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

