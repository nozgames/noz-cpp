//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <functional>
#include <initializer_list>

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

    using TaskRunFunc = std::function<void*(TaskHandle task)>;
    using TaskCompleteFunc = std::function<void(TaskHandle task, void* result)>;
    using TaskDestroyFunc = std::function<void(void* result)>;

    extern TaskHandle CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func = nullptr,
        TaskDestroyFunc destroy_func = nullptr);

    extern TaskHandle CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        TaskHandle depends_on,
        TaskDestroyFunc destroy_func = nullptr);

    extern TaskHandle CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        std::initializer_list<TaskHandle> depends_on,
        TaskDestroyFunc destroy_func = nullptr);

    extern TaskHandle CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        const TaskHandle* depends_on,
        int count,
        TaskDestroyFunc destroy_func = nullptr);

    extern TaskHandle CreateVirtualTask(TaskDestroyFunc destroy_func = nullptr);
    extern void CompleteTask(TaskHandle handle, void* result);
    extern void* GetTaskResult(TaskHandle handle);
    extern void* ReleaseTaskResult(TaskHandle handle);  // Takes ownership, caller responsible for cleanup
    extern int GetDependencyCount(TaskHandle handle);
    extern TaskHandle GetDependencyAt(TaskHandle handle, int index);
    extern bool IsTaskComplete(TaskHandle handle);
    extern bool IsTaskValid(TaskHandle handle);
    extern bool IsTaskCanceled(TaskHandle handle);
    extern void CancelTask(TaskHandle handle);

} // namespace noz
