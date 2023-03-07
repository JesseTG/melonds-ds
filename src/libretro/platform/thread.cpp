//
// Created by Jesse on 3/6/2023.
//

#include <libretro.h>
#include <rthreads/rthreads.h>
#include <Platform.h>

#include <utility>
#include <retro_timers.h>

using Platform::Thread;
struct ThreadData {
    std::function<void()> fn;
};

static void function_trampoline(void *param) {
    auto *data = (ThreadData *) param;
    data->fn();
    delete data;
}

Thread *Platform::Thread_Create(std::function<void()> func) {
#if HAVE_THREADS
    return (Thread *) sthread_create(function_trampoline, new ThreadData{std::move(func)});
#else
    return nullptr;
#endif
}

void Platform::Thread_Free(Thread *thread) {
#if HAVE_THREADS
    sthread_detach((sthread_t *) thread);
#endif
}

void Platform::Thread_Wait(Thread *thread) {
#if HAVE_THREADS
    sthread_join((sthread_t *) thread);
#endif
}

void Platform::Sleep(u64 usecs) {
    // TODO: May cause problems with recursion
    retro_sleep(usecs / 1000);
}