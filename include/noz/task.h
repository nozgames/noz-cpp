//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <functional>
#include <initializer_list>

namespace noz {

    struct Task {
        u32 id = 0;
        u32 generation = 0;

        constexpr Task() = default;
        constexpr Task(u32 id, u32 generation) : id(id), generation(generation) {}
        constexpr Task(std::nullptr_t) {}

        bool operator==(const Task& other) const { return id == other.id && generation == other.generation; }
        bool operator!=(const Task& other) const { return !(*this == other); }
        bool operator==(std::nullptr_t) const { return generation == 0; }
        bool operator!=(std::nullptr_t) const { return generation != 0; }
        explicit operator bool() const { return generation != 0; }
    };

    constexpr Task TASK_NULL = {};

    enum TaskState {
        TASK_STATE_FREE,
        TASK_STATE_PENDING,
        TASK_STATE_RUNNING,
        TASK_STATE_COMPLETE,
        TASK_STATE_CANCELED
    };

    constexpr void* TASK_NO_RESULT = (void*)"";  // Return this when you don't need a result

    using TaskRunFunc = std::function<void*(Task task)>;
    using TaskCompleteFunc = std::function<void(Task task, void* result)>;
    using TaskDestroyFunc = std::function<void(void* result)>;

    extern Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func = nullptr,
        TaskDestroyFunc destroy_func = nullptr);

    extern Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        Task depends_on,
        TaskDestroyFunc destroy_func = nullptr);

    extern Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        std::initializer_list<Task> depends_on,
        TaskDestroyFunc destroy_func = nullptr);

    extern Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        const Task* depends_on,
        int count,
        TaskDestroyFunc destroy_func = nullptr);

    extern Task CreateTask(TaskDestroyFunc destroy_func = nullptr);
    extern void Complete(Task task, void* result);
    extern void* GetResult(Task task);
    extern void* ReleaseResult(Task task);  // Takes ownership, caller responsible for cleanup
    extern int GetDependencyCount(Task task);
    extern Task GetDependencyAt(Task task, int index);
    extern Task GetParent(Task task);
    extern void SetParent(Task task, Task parent);
    extern bool IsComplete(Task task);
    extern bool IsTaskValid(Task task);
    extern bool IsCancelled(Task task);
    extern void Cancel(Task task);

} // namespace noz
