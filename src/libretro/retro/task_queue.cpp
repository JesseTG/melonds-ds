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
#include <string.h>
#include <retro_assert.h>

struct TaskFunctions {
    retro::task::TaskHandler handler = nullptr;
    retro::task::TaskHandler cleanup = nullptr;
};

void retro::task::init(bool threaded, retro_task_queue_msg_t msg_push) noexcept {
    task_queue_init(threaded, msg_push);
}


void retro::task::push(TaskSpec&& task) noexcept {
    task_queue_push(task._task);
    task._task = nullptr;
}

void retro::task::deinit() noexcept {
    task_queue_deinit();
}

void retro::task::check() noexcept {
    task_queue_check();
}

void retro::task::TaskSpec::TaskHandlerWrapper(retro_task_t* task) noexcept {
    TaskFunctions* functions = static_cast<TaskFunctions*>(task->user_data);

    if (functions->handler) {
        retro::task::TaskHandle handle(task);
        functions->handler(handle);
    }
}

void retro::task::TaskSpec::TaskCleanupWrapper(retro_task_t* task) noexcept {
    TaskFunctions* functions = static_cast<TaskFunctions*>(task->user_data);

    if (functions->cleanup) {
        retro::task::TaskHandle handle(task);
        functions->cleanup(handle);
    }

    delete functions;
}

retro::task::TaskSpec::TaskSpec(const TaskHandler& handler, const TaskHandler& cleanup, retro_time_t when, const std::string& title) {
    _task = task_init();

    if (!_task) {
        throw std::bad_alloc();
    }

    _task->mute = true;
    _task->when = when;
    _task->handler = TaskHandlerWrapper;
    _task->cleanup = TaskCleanupWrapper;
    _task->user_data = new TaskFunctions {
        .handler = handler,
        .cleanup = cleanup,
    };
    _task->title = strdup(title.c_str()); // the task queue will free this string later
}

retro::task::TaskSpec::~TaskSpec() noexcept {
    FreeTask();
}

retro::task::TaskSpec::TaskSpec(TaskSpec&& other) noexcept {
    _task = other._task;
    other._task = nullptr;
}

retro::task::TaskSpec& retro::task::TaskSpec::operator=(TaskSpec&& other) noexcept {
    FreeTask();
    _task = other._task;
    other._task = nullptr;

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
    task_set_finished(_task, true);
}

bool retro::task::TaskHandle::IsCancelled() const noexcept {
    return task_get_cancelled(_task);
}