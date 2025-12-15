//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace noz {

    struct TaskHandle {
        u32 id;
        u32 generation;

        bool operator==(const TaskHandle& other) const { return id == other.id && generation == other.generation; }
        bool operator!=(const TaskHandle& other) const { return !(*this == other); }
        explicit operator bool() const { return generation != 0; }
    };

    constexpr TaskHandle TASK_HANDLE_INVALID = { 0, 0 };

    enum TaskState {
        TASK_STATE_FREE,
        TASK_STATE_PENDING,
        TASK_STATE_RUNNING,
        TASK_STATE_COMPLETE,
        TASK_STATE_CANCELED
    };

    using TaskRunFunc = void (*)(TaskHandle task, void* user_data);
    using TaskCompleteFunc = void (*)(TaskHandle task, void* user_data);

    extern TaskHandle CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func=nullptr, void* user_data=nullptr);
    extern bool IsTaskComplete(TaskHandle handle);
    extern bool IsTaskValid(TaskHandle handle);
    extern void CancelTask(TaskHandle handle);

} // namespace noz
