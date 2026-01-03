//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// #define TASK_DEBUG
// #define TASK_DEBUG_VERBOSE

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <noz/task.h>
#include <thread>

namespace noz {
struct TaskImpl {
    std::atomic<TaskState> state{TASK_STATE_FREE};
    TaskRunFunc run_func;
    TaskCompleteFunc complete_func;
    TaskDestroyFunc destroy_func;
    void* result;
    Task parent;
    i32 dependency_head;
    i32 dependency_next;
    i32 dependency_count;
    i32 dependent;
    u64 start_frame;
    int generation;
    bool is_virtual;
    bool is_frame_task;
    u64 frame_id;

#if defined(TASK_DEBUG)
    String128 name;
    f64 debug_start_time;
    f64 debug_queue_time;
    f64 debug_end_time;
#endif
};

struct TaskWorker {
    std::thread thread;
    std::atomic<bool> running{false};
    std::atomic<TaskImpl*> current_task{nullptr};
    std::condition_variable cv;
    std::mutex cv_mutex;
};

struct TaskSystem {
    TaskImpl* tasks;
    i32 max_tasks;
    i32 max_frame_tasks;
    TaskWorker* workers;
    i32 worker_count;
    std::mutex mutex;
    std::atomic<bool> running{false};
    std::atomic<bool> tasks_completed{false};
    std::atomic<i32> pending_frame_tasks{0};
    u64 current_frame;
    u32 next_generation;
};

void UpdateTasks();
void InitTasks(const ApplicationTraits& traits);
void ShutdownTasks();

static TaskSystem g_tasks = {};
} // namespace noz

static bool AreAllDependenciesReady(noz::TaskImpl& impl);

using namespace noz;

static TaskImpl* GetTask(Task handle) {
    if (handle.generation == 0 || handle.id >= g_tasks.max_tasks)
        return nullptr;

    TaskImpl& task = g_tasks.tasks[handle.id];
    if (task.generation != handle.generation)
        return nullptr;

    return &task;
}

inline Task GetHandle(TaskImpl* task) {
    return {static_cast<i32>(task - g_tasks.tasks), task->generation};
}

inline int GetTaskIndex(TaskImpl* impl) {
    return static_cast<int>(impl - g_tasks.tasks);
}

static Task CreateTaskInternal(
    TaskRunFunc run_func,
    TaskCompleteFunc complete_func,
    TaskDestroyFunc destroy_func,
    const Task* deps,
    i32 dep_count,
    const char* name
) {
    (void) name;

    std::lock_guard lock(g_tasks.mutex);

    // Skip frame task reserved slots (start from max_frame_tasks)
    for (i32 task_index = g_tasks.max_frame_tasks; task_index < g_tasks.max_tasks; task_index++) {
        TaskImpl& impl = g_tasks.tasks[task_index];
        TaskState expected = TASK_STATE_FREE;
        if (!impl.state.compare_exchange_strong(expected, TASK_STATE_PENDING))
            continue;

        impl.generation = ++g_tasks.next_generation;
        impl.run_func = std::move(run_func);
        impl.complete_func = std::move(complete_func);
        impl.destroy_func = std::move(destroy_func);
        impl.parent = nullptr;
        impl.result = nullptr;
        impl.start_frame = g_tasks.current_frame;
        impl.is_virtual = false;
        impl.is_frame_task = false;
        impl.frame_id = 0;
        impl.dependent = -1;

#if defined(TASK_DEBUG)
        impl.debug_start_time = GetRealTime();
        Set(impl.name, name);
#endif
#if defined(TASK_DEBUG_VERBOSE)
        LogInfo("[TASK] QUEUED   : %3d: %s", task_index, impl.name);
#endif

        impl.dependency_head = -1;
        impl.dependency_next = -1;
        impl.dependency_count = 0;

        if (dep_count > 0) {
#ifndef NDEBUG
            for (i32 j = 0; j < dep_count; j++)
                assert(deps[j].id < g_tasks.max_tasks && "Invalid dependency handle");
#endif

            for (i32 dep_index=0; dep_index < dep_count; dep_index++) {
                if (deps[dep_index].generation == 0) continue;
                int dep_id = deps[dep_index].id;
                TaskImpl& dep = g_tasks.tasks[dep_id];
                assert(dep.dependent == -1 && "Task is already a dependency of another task");
                assert(dep.dependency_next == -1);

                dep.dependent = task_index;
                dep.dependency_next = impl.dependency_head;
                impl.dependency_head = dep_id;
                impl.dependency_count++;

#if defined(TASK_DEBUG_VERBOSE)
                if (dep_index == 0)
                    LogInfo("       DEPENDS  : %3d: %s", dep_id, dep.name);
                else
                    LogInfo("                : %3d: %s", dep_id, dep.name);
#endif
            }

#ifndef NDEBUG
            // todo: validate the dependency chain
            int validate_dep_id = impl.dependency_head;
            int validate_dep_count = 0;
            while (validate_dep_id != -1) {
                TaskImpl& dep = g_tasks.tasks[validate_dep_id];
                assert(dep.dependent == task_index);
                validate_dep_id = dep.dependency_next;
                validate_dep_count++;
            }
            assert(validate_dep_count == impl.dependency_count);
#endif
        }

        return {task_index, impl.generation};
    }

    return nullptr;
}

static Task CreateVirtualTask(TaskDestroyFunc destroy_func, const char* name) {
    (void) name;

    std::lock_guard lock(g_tasks.mutex);

    // Skip frame task reserved slots (start from max_frame_tasks)
    for (i32 task_index = g_tasks.max_frame_tasks; task_index < g_tasks.max_tasks; task_index++) {
        TaskImpl& task = g_tasks.tasks[task_index];
        TaskState expected = TASK_STATE_FREE;
        if (!task.state.compare_exchange_strong(expected, TASK_STATE_RUNNING))
            continue;

        task.generation = ++g_tasks.next_generation;
        task.run_func = nullptr;
        task.complete_func = nullptr;
        task.destroy_func = std::move(destroy_func);
        task.dependency_head = -1;
        task.dependency_next = -1;
        task.parent = nullptr;
        task.result = nullptr;
        task.start_frame = g_tasks.current_frame;
        task.is_virtual = true;
        task.is_frame_task = false;
        task.frame_id = 0;

#if defined(TASK_DEBUG)
        Set(task.name, name);
        task.debug_queue_time = GetRealTime();
        task.debug_start_time = task.debug_queue_time;
        task.debug_end_time = task.debug_queue_time;
#endif

        return {task_index, task.generation};
    }

    return nullptr;
}

Task noz::CreateTask(const TaskConfig& config) {
    Task result;

#if defined(TASK_DEBUG)
    assert(config.name);
#endif

    // Do not create a task if the parent task is already cancelled
    if (IsValid(config.parent) && IsCancelled(config.parent))
        return TASK_NULL;

    if (config.run)
        result = CreateTaskInternal(
            config.run,
            config.complete,
            config.destroy,
            config.dependencies,
            config.dependency_count,
            config.name
        );
    else
        result = CreateVirtualTask(
            config.destroy,
            config.name);

    if (result && config.parent)
        SetParent(result, config.parent);

    return result;
}

Task noz::CreateFrameTask(const FrameTaskConfig& config) {
#if defined(TASK_DEBUG)
    assert(config.name && "Frame tasks require a name in debug mode");
#endif

    std::lock_guard lock(g_tasks.mutex);

    // Allocate from reserved frame task slots only (indices 0 to max_frame_tasks-1)
    for (i32 task_index = 0; task_index < g_tasks.max_frame_tasks; task_index++) {
        TaskImpl& impl = g_tasks.tasks[task_index];
        TaskState expected = TASK_STATE_FREE;
        if (!impl.state.compare_exchange_strong(expected, TASK_STATE_PENDING))
            continue;

        impl.generation = ++g_tasks.next_generation;
        impl.run_func = config.run;
        impl.complete_func = nullptr;  // Frame tasks have no completion callback
        impl.destroy_func = config.destroy;
        impl.parent = nullptr;
        impl.result = nullptr;
        impl.start_frame = g_tasks.current_frame;
        impl.is_virtual = false;
        impl.is_frame_task = true;
        impl.frame_id = g_tasks.current_frame;
        impl.dependent = -1;
        impl.dependency_head = -1;
        impl.dependency_next = -1;
        impl.dependency_count = 0;

#if defined(TASK_DEBUG)
        impl.debug_queue_time = GetRealTime();
        Set(impl.name, config.name);
#endif

        // Handle dependencies
        if (config.dependency_count > 0 && config.dependencies) {
            for (i32 dep_index = 0; dep_index < config.dependency_count; dep_index++) {
                if (config.dependencies[dep_index].generation == 0) continue;
                int dep_id = config.dependencies[dep_index].id;
                TaskImpl& dep = g_tasks.tasks[dep_id];
                assert(dep.dependent == -1 && "Task is already a dependency of another task");
                dep.dependent = task_index;
                dep.dependency_next = impl.dependency_head;
                impl.dependency_head = dep_id;
                impl.dependency_count++;
            }
        }

        g_tasks.pending_frame_tasks.fetch_add(1);

        return {task_index, impl.generation};
    }

    // No free frame task slots - this is a programming error
    LogError("[TASK] No free frame task slots! Increase max_frame_tasks (current: %d)", g_tasks.max_frame_tasks);
    return TASK_NULL;
}

bool noz::HasPendingFrameTasks() {
    return g_tasks.pending_frame_tasks.load() > 0;
}

void noz::WaitForFrameTasks() {
    if (g_tasks.pending_frame_tasks.load() == 0)
        return;

#if defined(TASK_DEBUG)
    f64 wait_start = GetRealTime();
#endif

    u64 target_frame = g_tasks.current_frame;

    while (true) {
        // Check if all frame tasks for this frame are done
        bool all_done = true;
        for (i32 i = 0; i < g_tasks.max_frame_tasks; i++) {
            TaskImpl& impl = g_tasks.tasks[i];
            if (!impl.is_frame_task || impl.frame_id != target_frame)
                continue;

            TaskState state = impl.state.load();
            if (state == TASK_STATE_PENDING || state == TASK_STATE_RUNNING) {
                all_done = false;
                break;
            }
        }

        if (all_done)
            break;

        // Try to help execute pending frame tasks on main thread
        TaskImpl* pending = nullptr;
        {
            std::lock_guard lock(g_tasks.mutex);
            for (i32 i = 0; i < g_tasks.max_frame_tasks; i++) {
                TaskImpl& impl = g_tasks.tasks[i];
                if (impl.is_frame_task && impl.frame_id == target_frame &&
                    impl.state.load() == TASK_STATE_PENDING &&
                    AreAllDependenciesReady(impl)) {
                    TaskState expected = TASK_STATE_PENDING;
                    if (impl.state.compare_exchange_strong(expected, TASK_STATE_RUNNING)) {
                        pending = &impl;
                        break;
                    }
                }
            }
        }

        if (pending) {
            // Execute on main thread
#if defined(TASK_DEBUG)
            pending->debug_start_time = GetRealTime();
#endif
            if (pending->run_func) {
                try {
                    pending->result = pending->run_func(GetHandle(pending));
                } catch (std::exception& e) {
                    LogInfo("[TASK] exception: %s", e.what());
                } catch (...) {
                    LogInfo("[TASK] exception: ???");
                }
            }
#if defined(TASK_DEBUG)
            pending->debug_end_time = GetRealTime();
#endif
            pending->state.store(TASK_STATE_COMPLETE);
            g_tasks.pending_frame_tasks.fetch_sub(1);
            g_tasks.tasks_completed.store(true);
        } else {
            ThreadYield();
        }
    }

#if defined(TASK_DEBUG)
    f64 wait_time = GetRealTime() - wait_start;
    if (wait_time > 0.010) {  // Warn if > 10ms
        LogWarning("[TASK] WaitForFrameTasks took %dms", GetMilliseconds(wait_time));
    }
#endif
}

bool noz::HasPendingTasks() {
    for (i32 i = 0; i < g_tasks.max_tasks; i++) {
        TaskState state = g_tasks.tasks[i].state.load();
        if (state == TASK_STATE_PENDING || state == TASK_STATE_RUNNING)
            return true;
    }
    return false;
}

void noz::WaitForAllTasks() {
    while (HasPendingTasks()) {
        UpdateTasks();
        ThreadYield();
    }
}

void noz::Complete(Task task, void* result) {
    if (task.generation == 0)
        return;

    std::lock_guard lock(g_tasks.mutex);
    TaskImpl* impl = GetTask(task);
    if (!impl || !impl->is_virtual)
        return;

    impl->result = result;
    impl->state.store(TASK_STATE_COMPLETE);
    g_tasks.tasks_completed.store(true);

#if defined(TASK_DEBUG)
    impl->debug_start_time = impl->debug_end_time = GetRealTime();
#endif

#if defined(TASK_DEBUG)
    LogInfo(
        "[TASK] COMPLETE : %3d: %s (total_time=%dms  run_time=%dms  queue_time=%dms)",
        GetTaskIndex(impl),
        impl->name,
        GetMilliseconds(GetRealTime() - impl->debug_queue_time),
        GetMilliseconds(impl->debug_end_time - impl->debug_start_time),
        GetMilliseconds(impl->debug_start_time - impl->debug_queue_time)
    );
#endif
}

void* noz::GetResult(Task task) {
    if (task.generation == 0)
        return nullptr;

    if (task.id >= g_tasks.max_tasks)
        return nullptr;

    TaskImpl& impl = g_tasks.tasks[task.id];
    if (impl.generation != task.generation)
        return nullptr;

    return impl.result;
}

void* noz::ReleaseResult(Task task) {
    if (task.generation == 0)
        return nullptr;

    std::lock_guard lock(g_tasks.mutex);
    TaskImpl* impl = GetTask(task);
    if (!impl)
        return nullptr;

    void* result = impl->result;
    impl->result = nullptr;
    impl->destroy_func = nullptr;
    return result;
}

int noz::GetDependencyCount(Task task) {
    TaskImpl* impl = GetTask(task);
    if (!impl)
        return 0;

    return impl->dependency_count;
}

Task noz::GetDependencyAt(Task task, int index) {
    TaskImpl* impl = GetTask(task);
    if (!impl)
        return nullptr;

    i32 dep_index = impl->dependency_head;
    for (i32 i = impl->dependency_count - index - 1; i > 0 && dep_index >= 0; i--)
        dep_index = g_tasks.tasks[dep_index].dependency_next;

    if (dep_index < 0)
        return nullptr;

    return GetHandle(&g_tasks.tasks[dep_index]);
}

Task noz::GetParent(Task task) {
    if (task.generation == 0)
        return nullptr;

    TaskImpl* impl = GetTask(task);
    if (!impl)
        return nullptr;

    return impl->parent;
}

void noz::SetParent(Task task, Task parent) {
    if (task.generation == 0)
        return;

    std::lock_guard lock(g_tasks.mutex);
    TaskImpl* impl = GetTask(task);
    if (impl)
        impl->parent = parent;
}

bool noz::IsValid(Task task) {
    if (task.generation == 0)
        return false;

    std::lock_guard lock(g_tasks.mutex);
    return GetTask(task) != nullptr;
}

bool noz::IsComplete(Task task) {
    if (task.generation == 0)
        return true;

    std::lock_guard lock(g_tasks.mutex);
    TaskImpl* impl = GetTask(task);
    if (!impl)
        return true;

    TaskState state = impl->state.load();
    return state == TASK_STATE_COMPLETE || state == TASK_STATE_CANCELED || state == TASK_STATE_WAIT_DEPENDENT;
}

bool noz::IsCancelled(Task task) {
    if (task.generation == 0)
        return true;

    if (task.id >= g_tasks.max_tasks)
        return true;

    // No lock - this is called frequently from worker thread
    TaskImpl& impl = g_tasks.tasks[task.id];
    if (impl.generation != task.generation)
        return true;

    if (impl.state.load() == TASK_STATE_CANCELED)
        return true;

    // Check parent chain - if parent is cancelled, we're cancelled too
    if (impl.parent)
        return IsCancelled(impl.parent);

    return false;
}

void noz::Cancel(Task task) {
    if (!IsValid(task))
        return;

    std::lock_guard lock(g_tasks.mutex);
    TaskImpl* impl = GetTask(task);
    if (!impl)
        return;

    TaskState expected = TASK_STATE_PENDING;
    if (impl->state.compare_exchange_strong(expected, TASK_STATE_CANCELED))
        return;

    expected = TASK_STATE_RUNNING;
    impl->state.compare_exchange_strong(expected, TASK_STATE_CANCELED);
}

static bool AreAllDependenciesReady(TaskImpl& impl) {
    i32 dependency_index = impl.dependency_head;
    while (dependency_index >= 0) {
        TaskImpl& dep = g_tasks.tasks[dependency_index];
        assert(dep.dependent == GetTaskIndex(&impl));
        TaskState dep_state = dep.state.load();
        if (dep_state != TASK_STATE_COMPLETE && dep_state != TASK_STATE_WAIT_DEPENDENT) return false;
        dependency_index = dep.dependency_next;
    }
    return true;
}

static TaskImpl* FindOldestPendingTask() {
    // Priority 1: Frame tasks from current frame (always scheduled first)
    for (i32 i = 0; i < g_tasks.max_frame_tasks; i++) {
        TaskImpl& impl = g_tasks.tasks[i];
        if (impl.state.load() == TASK_STATE_PENDING &&
            impl.is_frame_task &&
            impl.frame_id == g_tasks.current_frame) {
            if (AreAllDependenciesReady(impl))
                return &impl;
        }
    }

    // Priority 2: Regular tasks (oldest first, skip frame task reserved slots)
    TaskImpl* oldest = nullptr;
    u64 oldest_frame = UINT64_MAX;

    for (i32 i = g_tasks.max_frame_tasks; i < g_tasks.max_tasks; i++) {
        TaskImpl& impl = g_tasks.tasks[i];
        if (impl.state.load() == TASK_STATE_PENDING) {
            if (!AreAllDependenciesReady(impl)) continue;

            if (impl.start_frame < oldest_frame) {
                oldest_frame = impl.start_frame;
                oldest = &impl;
            }
        }
    }

    return oldest;
}

static TaskWorker* FindIdleWorker() {
    for (i32 i = 0; i < g_tasks.worker_count; i++) {
        TaskWorker& worker = g_tasks.workers[i];
        if (worker.current_task.load() == nullptr)
            return &worker;
    }
    return nullptr;
}

static bool HasActiveDependents(TaskImpl* impl) {
    if (impl->dependent == -1) return false;
    TaskImpl* dependent = &g_tasks.tasks[impl->dependent];
    return dependent->state.load() != TASK_STATE_FREE;
}

static void ClearDependencyChain(TaskImpl* impl) {
    i32 dep_idx = impl->dependency_head;
    while (dep_idx >= 0) {
        TaskImpl& dep = g_tasks.tasks[dep_idx];
        assert(dep.dependent == GetTaskIndex(impl));
        i32 next = dep.dependency_next;
        dep.dependency_next = -1;
        dep.dependent = -1;
        dep_idx = next;
    }

    impl->dependency_head = -1;
    impl->dependency_next = -1;
    impl->dependency_count = 0;
}

// Cancel all children of a task (called when parent is cancelled)
static void CancelChildren(Task handle) {
    for (i32 i = 0; i < g_tasks.max_tasks; i++) {
        TaskImpl& task = g_tasks.tasks[i];
        if (task.parent.id != handle.id || task.parent.generation != handle.generation)
            continue;

        TaskState state = task.state.load();
        if (state == TASK_STATE_PENDING) {
            task.state.store(TASK_STATE_CANCELED);
        } else if (state == TASK_STATE_RUNNING) {
            task.state.store(TASK_STATE_CANCELED);
        }
        task.parent = nullptr;
    }
}

static void ClearChildrenParent(TaskImpl* impl) {
    int task_index = GetTaskIndex(impl);
    for (i32 i = 0; i < g_tasks.max_tasks; i++) {
        TaskImpl& task = g_tasks.tasks[i];
        if (task.parent.id == task_index && task.parent.generation == impl->generation)
            task.parent = nullptr;
    }
}

#if defined(TASK_DEBUG_VERBOSE)
static const char* GetStateName(TaskImpl& impl) {
    switch (impl.state) {
        case TASK_STATE_FREE: return "FREE";
        case TASK_STATE_PENDING: return "PENDING";
        case TASK_STATE_RUNNING: return impl.run_func ? "RUNNING" : "VIRTUAL";
        case TASK_STATE_COMPLETE: return "COMPLETE";
        case TASK_STATE_CANCELED: return "CANCELED";
        case TASK_STATE_WAIT_DEPENDENT: return "DEPENDENT";
        default: return "UNKNOWN";
    }
}
#endif

static void DestroyTask(TaskImpl* impl) {
    ClearChildrenParent(impl);

    // Call destructor before freeing
    if (impl->destroy_func) {
#if defined(TASK_DEBUG_VERBOSE)
        LogInfo("[TASK] DESTROY: %s: %3d: %s", GetStateName(*impl), GetTaskIndex(impl), impl->name);
#endif
        try {
            impl->destroy_func(impl->result);
        } catch (std::exception& e) {
            LogInfo("[TASK] exception: %s", e.what());
        } catch (...) {
            LogInfo("[TASK] exception: ???");
        }
    }

    ClearDependencyChain(impl);

    impl->run_func = nullptr;
    impl->complete_func = nullptr;
    impl->destroy_func = nullptr;
    impl->result = nullptr;
    impl->dependency_head = -1;
    impl->dependency_next = -1;
    impl->dependent = -1;
    impl->generation = 0;
    impl->state.store(TASK_STATE_FREE);
}

void noz::UpdateTasks() {
    // Clean up previous frame's completed frame tasks before incrementing frame
    for (i32 i = 0; i < g_tasks.max_frame_tasks; i++) {
        TaskImpl& impl = g_tasks.tasks[i];
        if (impl.is_frame_task &&
            impl.frame_id < g_tasks.current_frame &&
            impl.state.load() == TASK_STATE_COMPLETE) {
            DestroyTask(&impl);
        }
    }

    g_tasks.current_frame++;

#if defined(TASK_DEBUG_VERBOSE)
    static u64 debug_print_frame = 0;
    debug_print_frame++;
    bool should_print = (debug_print_frame % 300) == 0;
#endif

    if (g_tasks.tasks_completed.exchange(false)) {
        for (i32 task_index=0; task_index < g_tasks.max_tasks; task_index++) {
            TaskImpl& impl = g_tasks.tasks[task_index];
            TaskState state = impl.state.load();

            if (state == TASK_STATE_COMPLETE) {
                Task handle = GetHandle(&impl);

#if defined(TASK_DEBUG_VERBOSE)
                f64 elapsed_time = GetRealTime() - impl.debug_start_time;
                LogInfo("[TASK] DONE     : %3d: %s (%dms)",
                    task_index,
                    impl.name,
                    static_cast<int>(elapsed_time * 1000.0f)
                );
#endif

                if (impl.complete_func) {
                    try {
                        impl.complete_func(handle, impl.result);
                    } catch (std::exception& e) {
                        LogInfo("[TASK] exception: %s", e.what());
                    } catch (...) {
                        LogInfo("[TASK] exception: ???");
                    }
                }

#if defined(TASK_DEBUG)
                LogInfo(
                    "[TASK] COMPLETE : %3d: %s (total_time=%dms  run_time=%dms  queue_time=%dms)",
                    GetTaskIndex(&impl),
                    impl.name,
                    GetMilliseconds(GetRealTime() - impl.debug_queue_time),
                    GetMilliseconds(impl.debug_end_time - impl.debug_start_time),
                    GetMilliseconds(impl.debug_start_time - impl.debug_queue_time)
                );
#endif

                if (HasActiveDependents(&impl)) {
                    impl.state.store(TASK_STATE_WAIT_DEPENDENT);
                    continue;
                }

                DestroyTask(&impl);

            } else if (state == TASK_STATE_CANCELED) {
                Task handle = GetHandle(&impl);

                if (HasActiveDependents(&impl))
                    continue;

                // Propagate cancellation to children before freeing
                CancelChildren(handle);

                DestroyTask(&impl);
            }
        }
    }

    // Always check WAIT_DEPENDENT tasks - they need cleanup even when no new tasks complete
    // This fixes a bug where WAIT_DEPENDENT tasks could get stuck if their dependent was
    // destroyed but no other tasks completed in subsequent frames
    for (i32 task_index=0; task_index < g_tasks.max_tasks; task_index++) {
        TaskImpl& impl = g_tasks.tasks[task_index];
        if (impl.state.load() == TASK_STATE_WAIT_DEPENDENT) {
            if (!HasActiveDependents(&impl)) {
                DestroyTask(&impl);
            }
        }
    }

    // Assign pending tasks to idle workers
    std::lock_guard lock(g_tasks.mutex);

    while (true) {
        TaskWorker* worker = FindIdleWorker();
        if (!worker)
            break;

        TaskImpl* task = FindOldestPendingTask();
        if (!task)
            break;

        task->state.store(TASK_STATE_RUNNING);
        worker->current_task.store(task);
        worker->cv.notify_one();
    }

#if defined(TASK_DEBUG_VERBOSE)
    if (should_print) {
        int pending_count = 0;
        int running_count = 0;
        int wait_dependent = 0;
        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            if (task.state == TASK_STATE_PENDING)
                pending_count++;
            else if (task.state == TASK_STATE_RUNNING)
                running_count++;
            else if (task.state == TASK_STATE_WAIT_DEPENDENT)
                wait_dependent++;
        }

        LogInfo("[TASK] UPDATE   : running=%d  pending=%d", running_count, pending_count);
        for (i32 task_index=0; task_index < g_tasks.max_tasks; task_index++) {
            TaskImpl& impl = g_tasks.tasks[task_index];
            if (impl.state == TASK_STATE_FREE)
                continue;

            LogInfo("    %s: %3d : %s", GetStateName(impl), task_index, impl.name);

            if (GetTask(impl.parent)) {
                TaskImpl& parent = g_tasks.tasks[impl.parent.id];
                LogInfo("        PARENT: %s: %3d: %s", GetStateName(parent), impl.parent.id, parent.name);
            }

            int dep_index = impl.dependency_head;
            while (dep_index != -1) {
                TaskImpl& dep = g_tasks.tasks[dep_index];
                if (impl.dependency_head == dep_index)
                    LogInfo("        DEPEND: %s: %3d: %s", GetStateName(dep), dep_index, dep.name);
                else
                    LogInfo("              : %s: %3d: %s", GetStateName(dep), dep_index, dep.name);

                dep_index = dep.dependency_next;
            }
        }
    }
#endif
}

static void WorkerProc(TaskWorker* worker, int worker_index) {
    char name[32];
    snprintf(name, sizeof(name), "task_worker_%d", worker_index);
    SetThreadName(name);

    worker->running = true;

    while (worker->running) {
        TaskImpl* impl = nullptr;

        {
            std::unique_lock lock(worker->cv_mutex);
            worker->cv.wait(lock, [worker] { return worker->current_task.load() != nullptr || !worker->running; });

            if (!worker->running)
                break;

            impl = worker->current_task.load();
        }

        if (impl) {
#if defined(TASK_DEBUG_VERBOSE)
            LogInfo("[TASK] WORKER_WORK: %s: %3d : %s", name, GetTaskIndex(impl), impl->name);
#endif

            TaskState expected = TASK_STATE_RUNNING;
            if (impl->state.load() == expected && impl->run_func) {
#if defined (TASK_DEBUG)
                impl->debug_start_time = GetRealTime();
#endif
#if defined(TASK_DEBUG_VERBOSE)
                LogInfo("[TASK] RUN_BEGIN: %3d : 0x%llx: %s", GetTaskIndex(impl), GetThreadId(), impl->name);
#endif
                try {
                    impl->result = impl->run_func(GetHandle(impl));
                } catch (std::exception& e) {
                    LogInfo("[TASK] exception: %s", e.what());
                } catch (...) {
                    LogInfo("[TASK] exception: ???");
                }

#if defined(TASK_DEBUG)
                impl->debug_end_time = GetRealTime();
#endif
#if defined(TASK_DEBUG_VERBOSE)
                LogInfo(
                    "[TASK] RUN_END  : %3d : 0x%llx: %s (%dms)", GetTaskIndex(impl), GetThreadId(), impl->name,
                    static_cast<int>((impl->debug_end_time - impl->debug_start_time) * 1000.0f)
                );
#endif
            } else {
#if defined(TASK_DEBUG)
                impl->debug_start_time = GetRealTime();
                impl->debug_end_time = impl->debug_start_time;
#endif
            }

            expected = TASK_STATE_RUNNING;
            if (impl->state.compare_exchange_strong(expected, TASK_STATE_COMPLETE)) {
                g_tasks.tasks_completed.store(true);
                // Decrement frame task counter if this was a frame task
                if (impl->is_frame_task) {
                    g_tasks.pending_frame_tasks.fetch_sub(1);
                }
            }

#if defined(TASK_DEBUG_VERBOSE)
            LogInfo("[TASK] WORKER_DONE: %s: %3d : %s", name, GetTaskIndex(impl), impl->name);
#endif
        } else {
#if defined(TASK_DEBUG_VERBOSE)
            LogInfo("[TASK] WORKER_WORK: nullptr");
#endif
        }

        worker->current_task.store(nullptr);
    }
}

void noz::InitTasks(const ApplicationTraits& traits) {
    i32 max_tasks = traits.max_tasks;
    i32 max_frame_tasks = traits.max_frame_tasks > 0 ? traits.max_frame_tasks : 64;
    i32 worker_count = traits.max_task_worker_count;

    // Ensure max_frame_tasks doesn't exceed max_tasks
    if (max_frame_tasks > max_tasks)
        max_frame_tasks = max_tasks / 4;

    g_tasks.max_tasks = max_tasks;
    g_tasks.max_frame_tasks = max_frame_tasks;
    g_tasks.tasks = new TaskImpl[max_tasks];
    g_tasks.worker_count = worker_count;
    g_tasks.workers = new TaskWorker[worker_count];
    g_tasks.running = true;
    g_tasks.current_frame = 0;
    g_tasks.next_generation = 0;
    g_tasks.pending_frame_tasks = 0;

    for (i32 i = 0; i < max_tasks; i++) {
        TaskImpl& impl = g_tasks.tasks[i];
        impl.state = TASK_STATE_FREE;
        impl.generation = 0;
        impl.destroy_func = nullptr;
        impl.dependency_count = 0;
        impl.dependency_head = -1;
        impl.dependency_next = -1;
        impl.dependent = -1;
        impl.is_frame_task = false;
        impl.frame_id = 0;
    }

    for (i32 i = 0; i < worker_count; i++) {
        g_tasks.workers[i].running = false;
        g_tasks.workers[i].current_task = nullptr;
        g_tasks.workers[i].thread = std::thread(WorkerProc, &g_tasks.workers[i], i);
    }
}

void noz::ShutdownTasks() {
    g_tasks.running = false;

    for (i32 i = 0; i < g_tasks.worker_count; i++) {
        TaskWorker& worker = g_tasks.workers[i];
        worker.running = false;
        worker.cv.notify_one();
    }

    for (i32 i = 0; i < g_tasks.worker_count; i++) {
        if (g_tasks.workers[i].thread.joinable())
            g_tasks.workers[i].thread.join();
    }

    delete[] g_tasks.workers;
    g_tasks.workers = nullptr;

    delete[] g_tasks.tasks;
    g_tasks.tasks = nullptr;

    LogInfo("Task system shutdown");
}

void noz::AddDependency(Task task, Task dependency) {
    if (!IsValid(task))
        return;

    std::lock_guard lock(g_tasks.mutex);
    TaskImpl* impl = GetTask(task);
    TaskImpl* dep = GetTask(dependency);
    if (!impl || !dep)
        return;

    assert(dep->dependency_next == -1 && "Task is already a dependency of another task");
    dep->dependency_next = impl->dependency_head;
    dep->dependent = task.id;
    impl->dependency_head = dependency.id;
    impl->dependency_count++;
}
