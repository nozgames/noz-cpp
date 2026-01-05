//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include <functional>

namespace noz {

    struct Task {
        int id = 0;
        int generation = 0;

        constexpr Task() = default;
        constexpr Task(int id, int generation) : id(id), generation(generation) {}
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
        TASK_STATE_WAIT_DEPENDENT,
        TASK_STATE_CANCELED
    };

    constexpr void* TASK_NO_RESULT = (void*)"";  // Return this when you don't need a result

    using TaskRunFunc = std::function<void*(Task task)>;
    using TaskCompleteFunc = std::function<void(Task task, void* result)>;
    using TaskDestroyFunc = std::function<void(void* result)>;

    struct TaskConfig {
        TaskRunFunc run = nullptr;
        TaskCompleteFunc complete = nullptr;
        TaskDestroyFunc destroy = nullptr;
        Task parent = TASK_NULL;
        const Task* dependencies = nullptr;
        int dependency_count = 0;
        const char* name = nullptr;
    };

    // Frame tasks are for per-frame parallel work that completes within a single frame.
    // Results are valid until the end of the frame (cleaned up at start of next frame).
    struct FrameTaskConfig {
        TaskRunFunc run = nullptr;
        TaskDestroyFunc destroy = nullptr;
        void* user_data = nullptr;
        const Task* dependencies = nullptr;
        int dependency_count = 0;
        const char* name = nullptr;
    };

    extern Task CreateTask(const TaskConfig& config = {});
    extern Task CreateFrameTask(const FrameTaskConfig& config = {});
    extern void WaitForFrameTasks();
    extern bool HasPendingFrameTasks();
    extern void WaitForAllTasks();       // Blocks until all tasks (regular and frame) are complete
    extern bool HasPendingTasks();       // Returns true if any tasks are pending or running
    extern void Complete(Task task, void* result=TASK_NO_RESULT);
    extern void* GetResult(Task task);
    extern void* ReleaseResult(Task task);  // Takes ownership, caller responsible for cleanup
    extern int GetDependencyCount(Task task);
    extern Task GetDependencyAt(Task task, int index);
    extern Task GetParent(Task task);
    extern void SetParent(Task task, Task parent);
    extern bool IsComplete(Task task);
    extern bool IsValid(Task task);
    extern bool IsCancelled(Task task);
    extern void Cancel(Task task);
    extern void AddDependency(Task task, Task dependency);

} // namespace noz
