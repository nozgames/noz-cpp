//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <functional>
#include <initializer_list>
#include <span>

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

    constexpr void* TASK_NO_RESULT = (void*)"";  // Return this when you don't need a result

    using TaskRunFunc = std::function<void*(TaskHandle task, std::span<void*> dep_results)>;
    using TaskCompleteFunc = std::function<void(TaskHandle task, void* result)>;

    // Create task with no dependencies
    extern TaskHandle CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func = nullptr);

    // Create task with single dependency
    extern TaskHandle CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func, TaskHandle depends_on);

    // Create task with multiple dependencies
    extern TaskHandle CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func, std::initializer_list<TaskHandle> depends_on);
    extern TaskHandle CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func, const TaskHandle* depends_on, int count);

    extern TaskHandle CreateVirtualTask();  // No worker - complete manually with CompleteTask()
    extern void CompleteTask(TaskHandle handle, void* result);  // Complete a virtual task
    extern void* GetTaskResult(TaskHandle handle);  // Get result from completed dependency
    extern bool IsTaskComplete(TaskHandle handle);
    extern bool IsTaskValid(TaskHandle handle);
    extern bool IsTaskCanceled(TaskHandle handle);  // Call from run_func to check if should abort
    extern void CancelTask(TaskHandle handle);

} // namespace noz
