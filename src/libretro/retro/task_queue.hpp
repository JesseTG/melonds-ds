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
#include <string_view>

#include <queues/task_queue.h>

namespace retro::task {
    constexpr retro_time_t ASAP = 0;
    class TaskSpec;
    class TaskHandle;

    using TaskHandler = std::function<void(TaskHandle&)>;
    using TaskCallback = std::function<void(TaskHandle&, void*, std::string_view)>;

    using UnaryTaskFinder = std::function<bool(TaskHandle&)>;

    void init(bool threaded, retro_task_queue_msg_t msg_push) noexcept;

    /// Ignores invalid tasks.
    void push(TaskSpec&& task) noexcept;
    bool find(const UnaryTaskFinder& finder) noexcept;

    void check() noexcept;
    void reset() noexcept;
    void deinit() noexcept;
    void wait() noexcept;

    class TaskSpec {
    public:
        // Exists to create a trivial task that does nothing.
        TaskSpec() noexcept = default;
        TaskSpec(
            const TaskHandler& handler,
            const TaskCallback& callback = nullptr,
            const TaskHandler& cleanup = nullptr,
            retro_time_t when = ASAP,
            const std::string& title = ""
        );
        ~TaskSpec() noexcept;
        TaskSpec(TaskSpec&& other) noexcept;
        TaskSpec(const TaskSpec& other) = delete;
        TaskSpec& operator=(TaskSpec&& other) noexcept;
        TaskSpec& operator=(const TaskSpec& other) = delete;
        [[nodiscard]] bool Valid() const noexcept { return _task != nullptr; }

        [[nodiscard]] retro_time_t When() const noexcept { return _task ? _task->when : 0; }
        void When(retro_time_t when) noexcept { if (_task) _task->when = when; }

        [[nodiscard]] uint32_t Identifier() const noexcept { return _task ? _task->ident : 0; }
        void Identifier(uint32_t ident) noexcept { if (_task) _task->ident = ident; }

        operator bool() const noexcept { return _task != nullptr; }
    private:
        void FreeTask() noexcept;
        static void TaskHandlerWrapper(retro_task_t* task) noexcept;
        static void TaskCallbackWrapper(retro_task_t *task, void *task_data, void *user_data, const char *error) noexcept;
        static void TaskCleanupWrapper(retro_task_t* task) noexcept;
        friend void push(TaskSpec&& task) noexcept;
        retro_task_t* _task;
    };

    static bool operator==(const TaskSpec& lhs, std::nullptr_t) noexcept { return !lhs; }
    static bool operator==(std::nullptr_t, const TaskSpec& rhs) noexcept { return !rhs; }
    static bool operator!=(const TaskSpec& lhs, std::nullptr_t) noexcept { return (bool)lhs; }
    static bool operator!=(std::nullptr_t, const TaskSpec& rhs) noexcept { return (bool)rhs; }

    class TaskHandle {
    public:
        // Default destructor (the task system will clean up the underlying task)
        ~TaskHandle() = default;
        TaskHandle(const TaskHandle& other) = delete;
        TaskHandle& operator=(const TaskHandle& other) = delete;
        TaskHandle(TaskHandle&& other) = delete;
        TaskHandle& operator=(TaskHandle&& other) = delete;
        [[nodiscard]] bool Valid() const noexcept { return _task != nullptr; }
        void Finish() noexcept;
        void Cancel() noexcept;
        [[nodiscard]] bool IsCancelled() const noexcept;
        [[nodiscard]] bool IsFinished() const noexcept;
        void SetError(const std::string_view& error) noexcept;
        [[nodiscard]] std::string_view GetError() const noexcept;
        [[nodiscard]] uint32_t Identifier() const noexcept { return _task->ident; }
    private:
        friend class TaskSpec;
        friend bool find(const UnaryTaskFinder& finder) noexcept;
        TaskHandle(retro_task_t* task) noexcept;
        retro_task_t* _task;
    };
}

#endif //MELONDS_DS_TASK_QUEUE_HPP
