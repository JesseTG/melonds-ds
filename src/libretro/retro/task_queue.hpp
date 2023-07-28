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

#ifndef MELONDS_DS_TASK_QUEUE_HPP
#define MELONDS_DS_TASK_QUEUE_HPP

#include <functional>
#include <string>

#include <queues/task_queue.h>

namespace retro::task {
    constexpr retro_time_t ASAP = 0;
    class TaskSpec;
    class TaskHandle;

    using TaskHandler = std::function<void(TaskHandle&)>;

    void init(bool threaded, retro_task_queue_msg_t msg_push) noexcept;

    void push(TaskSpec&& task) noexcept;
    void check() noexcept;
    void deinit() noexcept;

    class TaskSpec {
    public:
        TaskSpec(const TaskHandler& handler, const TaskHandler& cleanup = nullptr, retro_time_t when = ASAP, const std::string& title = "");
        ~TaskSpec() noexcept;
        TaskSpec(TaskSpec&& other) noexcept;
        TaskSpec(const TaskSpec& other) = delete;
        TaskSpec& operator=(TaskSpec&& other) noexcept;
        TaskSpec& operator=(const TaskSpec& other) = delete;
        [[nodiscard]] bool Valid() const noexcept { return _task != nullptr; }

        [[nodiscard]] retro_time_t When() const noexcept { return _task->when; }
        void When(retro_time_t when) noexcept { _task->when = when; }
    private:
        void FreeTask() noexcept;
        static void TaskHandlerWrapper(retro_task_t* task) noexcept;
        static void TaskCleanupWrapper(retro_task_t* task) noexcept;
        friend void push(TaskSpec&& task) noexcept;
        retro_task_t* _task;
    };

    class TaskHandle {
    public:
        // Default destructor
        ~TaskHandle() = default;
        TaskHandle(const TaskHandle& other) = delete;
        TaskHandle& operator=(const TaskHandle& other) = delete;
        TaskHandle(TaskHandle&& other) = delete;
        TaskHandle& operator=(TaskHandle&& other) = delete;
        [[nodiscard]] bool Valid() const noexcept { return _task != nullptr; }
        void Finish() noexcept;
        [[nodiscard]] bool IsCancelled() const noexcept;
        // TODO: Write a setter for the error message
    private:
        friend class TaskSpec;
        TaskHandle(retro_task_t* task) noexcept;
        retro_task_t* _task;
    };


}

#endif //MELONDS_DS_TASK_QUEUE_HPP
