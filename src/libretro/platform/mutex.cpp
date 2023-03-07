//
// Created by Jesse on 3/6/2023.
//

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