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

#include "task_queue.hpp"
#include <new>
#include <stdexcept>
#include <string.h>

#include <compat/strl.h>
#include <retro_assert.h>

#include "tracy.hpp"

using std::string_view;

struct TaskFunctions {
    retro::task::TaskHandler handler = nullptr;
    retro::task::TaskCallback callback = nullptr;
    retro::task::TaskHandler cleanup = nullptr;
};

void retro::task::init(bool threaded, retro_task_queue_msg_t msg_push) noexcept {
    ZoneScopedN("task_queue_init");
    task_queue_init(threaded, msg_push);
}


std::optional<uint32_t> retro::task::push(TaskSpec&& task) noexcept {
    if (task.Valid()) {
        ZoneScopedN("task_queue_push");
        task_queue_push(task._task);
        uint32_t ident = task._task->ident;
        task._task = nullptr;
        return ident;
    }

    return std::nullopt;
}

std::optional<retro::task::TaskHandle> retro::task::find(uint32_t ident) noexcept {
    return find([ident](TaskHandle& task) noexcept {
        return task.Identifier() == ident;
    });
}

std::optional<retro::task::TaskHandle> retro::task::find(std::string_view title) noexcept {
    return find([title](TaskHandle& task) noexcept {
        return task.Title() == title;
    });
}

std::optional<retro::task::TaskHandle> retro::task::find(const UnaryTaskFinder& finder) noexcept {
    if (finder == nullptr) {
        return std::nullopt;
    }

    struct FinderDataState {
        const UnaryTaskFinder& predicate;
        retro_task_t* result;

        bool operator()(TaskHandle& task) noexcept {
            bool ok = predicate(task);
            if (ok) {
                result = task._task;
            }
            return ok;
        }
    };

    FinderDataState finderState {
        .predicate = finder,
        .result = nullptr,
    };

    task_finder_data_t finder_data {
        .func = [](retro_task_t* task, void* data) noexcept {
            ZoneScopedN(TracyFunction);
            FinderDataState& state = *reinterpret_cast<FinderDataState*>(data);
            TaskHandle task_handle(task);
            return state(task_handle);
        },
        .userdata = (void*) &finderState,
    };

    ZoneScopedN("task_queue_find");
    if (task_queue_find(&finder_data)) {
        return TaskHandle(finderState.result);
    }

    return std::nullopt;
}

void retro::task::wait() noexcept {
    ZoneScopedN("task_queue_wait");
    task_queue_wait(nullptr, nullptr); // wait for all tasks to finish
}

void retro::task::deinit() noexcept {
    ZoneScopedN("task_queue_deinit");
    task_queue_deinit();
}

void retro::task::reset() noexcept {
    ZoneScopedN("task_queue_reset");
    task_queue_reset(); // cancel all outstanding tasks
}

void retro::task::check() noexcept {
    ZoneScopedN("task_queue_check");
    task_queue_check();
}

void retro::task::TaskSpec::TaskHandlerWrapper(retro_task_t* task) noexcept {
    ZoneScopedN(TracyFunction);
    retro_assert(task != nullptr);
    TaskFunctions* functions = static_cast<TaskFunctions*>(task->user_data);

    if (functions->handler && !(task_get_flags(task) & RETRO_TASK_FLG_FINISHED)) {
        retro::task::TaskHandle handle(task);
        retro_assert(!handle.IsFinished());
        if (handle.IsCancelled()) {
            handle.Finish();
        } else {
            functions->handler(handle);
        }
    }
}

void retro::task::TaskSpec::TaskCallbackWrapper(
    retro_task_t *task,
    void *task_data,
    void *user_data,
    const char *error
) noexcept {
    ZoneScopedN(TracyFunction);
    TaskFunctions* functions = static_cast<TaskFunctions*>(user_data);

    retro_assert(task != nullptr);
    retro_assert(functions != nullptr);
    retro_assert(functions->callback != nullptr); // if it were nullptr, we wouldn't have gotten this far

    std::string_view error_view = error ? std::string_view(error) : std::string_view();

    if (functions->callback) {
        retro::task::TaskHandle handle(task);
        functions->callback(handle, task_data, error_view);
    }
}

void retro::task::TaskSpec::TaskCleanupWrapper(retro_task_t* task) noexcept {
    ZoneScopedN(TracyFunction);
    retro_assert(task != nullptr);
    TaskFunctions* functions = static_cast<TaskFunctions*>(task->user_data);
    retro_assert(functions != nullptr);

    if (functions->cleanup) {
        retro::task::TaskHandle handle(task);
        functions->cleanup(handle);
    }

    delete functions;
    task->user_data = nullptr;
}

retro::task::TaskSpec::TaskSpec(const TaskHandler& handler, const TaskCallback& callback, const TaskHandler& cleanup, retro_time_t when, const std::string& title) {
    if (!handler) {
        throw std::invalid_argument("TaskSpec::TaskSpec: handler must be non-null");
    }

    _task = task_init();

    if (!_task) {
        throw std::bad_alloc();
    }

    _task->flags = RETRO_TASK_FLG_MUTE;
    _task->when = when;
    _task->handler = TaskHandlerWrapper;
    _task->callback = callback ? &TaskCallbackWrapper : nullptr;
    _task->cleanup = TaskCleanupWrapper;
    _task->user_data = new TaskFunctions {
        .handler = handler,
        .callback = callback,
        .cleanup = cleanup,
    };
    _task->title = title.empty() ? nullptr : strdup(title.c_str()); // the task queue will free this string later
}

retro::task::TaskSpec::~TaskSpec() noexcept {
    FreeTask();
}

retro::task::TaskSpec::TaskSpec(TaskSpec&& other) noexcept {
    _task = other._task;
    other._task = nullptr;
}

retro::task::TaskSpec& retro::task::TaskSpec::operator=(TaskSpec&& other) noexcept {
    if (this != &other) {
        FreeTask();
        _task = other._task;
        other._task = nullptr;
    }

    return *this;
}

void retro::task::TaskSpec::FreeTask() noexcept {
    if (!_task)
        return;

    if (_task->cleanup)
        _task->cleanup(_task);

    if (_task->error)
        free(_task->error);

    if (_task->title)
        free(_task->title);

    free(_task);
    _task = nullptr;
}

retro::task::TaskHandle::TaskHandle(retro_task_t* task) noexcept : _task(task) {
}

void retro::task::TaskHandle::Finish() noexcept {
    ZoneScopedN("task_set_finished");
    task_set_flags(_task, RETRO_TASK_FLG_FINISHED, true);
}

void retro::task::TaskHandle::Cancel() noexcept {
    ZoneScopedN("task_set_cancelled");
    task_set_flags(_task, RETRO_TASK_FLG_CANCELLED, true);
}

bool retro::task::TaskHandle::IsCancelled() const noexcept {
    ZoneScopedN("task_get_cancelled");
    return task_get_flags(_task) & RETRO_TASK_FLG_CANCELLED;
}

bool retro::task::TaskHandle::IsFinished() const noexcept {
    ZoneScopedN("task_get_finished");
    return task_get_flags(_task) & RETRO_TASK_FLG_FINISHED;
}

void retro::task::TaskHandle::SetError(const string_view& error) noexcept {
    char* error_cstr = error.empty() ? nullptr : strldup(error.data(), error.size());
    task_set_error(_task, error_cstr);
}

string_view retro::task::TaskHandle::GetError() const noexcept {
    const char* error = task_get_error(_task);
    return error ? string_view(error) : string_view();
}