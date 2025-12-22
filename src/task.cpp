//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0
#define TASK_DEBUG
#endif

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

#if defined(TASK_DEBUG)
    String128 name;
    f64 debug_start_time;
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
    TaskWorker* workers;
    i32 worker_count;
    std::mutex mutex;
    std::atomic<bool> running{false};
    std::atomic<bool> tasks_completed{false};
    u64 current_frame;
    u32 next_generation;
};

void UpdateTasks();
void InitTasks(const ApplicationTraits& traits);
void ShutdownTasks();

static TaskSystem g_tasks = {};
} // namespace noz

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

    for (i32 task_index=0; task_index < g_tasks.max_tasks; task_index++) {
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
        impl.dependent = -1;

#if defined(TASK_DEBUG)
        Set(impl.name, name);
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

#if defined(TASK_DEBUG)
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

    for (i32 task_index = 0; task_index < g_tasks.max_tasks; task_index++) {
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

#if defined(TASK_DEBUG)
        Set(task.name, name);
        task.debug_start_time = GetTime();
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
    f64 elapsed_time = GetTime() - impl->debug_start_time;
    LogInfo(
        "[TASK] COMPLETE : %3d : 0x%llx: %s (%dms)", GetTaskIndex(impl), GetThreadId(), impl->name,
        static_cast<int>(elapsed_time * 1000.0f)
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
    if (task.generation == 0)
        return 0;

    TaskImpl* impl = GetTask(task);
    if (!impl)
        return 0;

    return impl->dependency_count;
}

Task noz::GetDependencyAt(Task task, int index) {
    if (task.generation == 0 || index < 0)
        return nullptr;

    TaskImpl* impl = GetTask(task);
    if (!impl)
        return nullptr;

    i32 dep_idx = impl->dependency_head;
    for (int i = 0; i < index && dep_idx >= 0; i++) {
        dep_idx = g_tasks.tasks[dep_idx].dependency_next;
    }

    if (dep_idx < 0)
        return nullptr;

    TaskImpl& dep = g_tasks.tasks[dep_idx];
    return {dep_idx, dep.generation};
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
    TaskImpl* oldest = nullptr;
    u64 oldest_frame = UINT64_MAX;

    for (i32 i = 0; i < g_tasks.max_tasks; i++) {
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
    i32 task_id = GetTaskIndex(impl);
    while (dep_idx >= 0) {
        TaskImpl& dep = g_tasks.tasks[dep_idx];
        assert(dep.dependent == task_id);
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

#if defined(TASK_DEBUG)
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
#if defined(TASK_DEBUG)
        LogInfo("[TASK] DESTROY: %s: %3d: %s", GetStateName(*impl), GetTaskIndex(impl), impl->name);
#endif
        try {
            impl->destroy_func(impl->result);
        } catch (...) {
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
    g_tasks.current_frame++;

#if defined(TASK_DEBUG)
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

#if defined(TASK_DEBUG)
                f64 before_complete_time = GetRealTime();
                f64 elapsed_time = GetTime() - impl.debug_start_time;
                LogInfo(
                    "[TASK] DONE     : %3d: %s (%dms)", task_index, impl.name,
                    static_cast<int>(elapsed_time * 1000.0f)
                );
#endif

                if (impl.complete_func) {
                    try {
                        impl.complete_func(handle, impl.result);
                    } catch (...) {
                    }
                }


#if defined(TASK_DEBUG)
                elapsed_time = GetRealTime() - before_complete_time;
                LogInfo(
                    "[TASK] COMPLETE : %3d : %s (%dms)", task_index, impl.name,
                    static_cast<int>(elapsed_time * 1000.0f)
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
            } else if (state == TASK_STATE_WAIT_DEPENDENT) {
                if (HasActiveDependents(&impl))
                    continue;

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

#if defined(TASK_DEBUG)
    if (should_print) {
        int pending_count = 0;
        int running_count = 0;
        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            if (task.state == TASK_STATE_PENDING)
                pending_count++;
            else if (task.state == TASK_STATE_RUNNING)
                running_count++;
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
#if defined(TASK_DEBUG)
            LogInfo("[TASK] WORKER_WORK: %s: %3d : %s", name, GetTaskIndex(impl), impl->name);
#endif

            TaskState expected = TASK_STATE_RUNNING;
            if (impl->state.load() == expected && impl->run_func) {
#if defined(TASK_DEBUG)
                impl->debug_start_time = GetTime();
                LogInfo("[TASK] RUN_BEGIN: %3d : 0x%llx: %s", GetTaskIndex(impl), GetThreadId(), impl->name);
#endif
                impl->result = impl->run_func(GetHandle(impl));

#if defined(TASK_DEBUG)
                impl->debug_end_time = GetTime();
                LogInfo(
                    "[TASK] RUN_END  : %3d : 0x%llx: %s (%dms)", GetTaskIndex(impl), GetThreadId(), impl->name,
                    static_cast<int>((impl->debug_end_time - impl->debug_start_time) * 1000.0f)
                );
#endif
            }

            expected = TASK_STATE_RUNNING;
            if (impl->state.compare_exchange_strong(expected, TASK_STATE_COMPLETE))
                g_tasks.tasks_completed.store(true);

#if defined(TASK_DEBUG)
            LogInfo("[TASK] WORKER_DONE: %s: %3d : %s", name, GetTaskIndex(impl), impl->name);
#endif
        } else {
#if defined(TASK_DEBUG)
            LogInfo("[TASK] WORKER_WORK: nullptr");
#endif
        }

        worker->current_task.store(nullptr);
    }
}

void noz::InitTasks(const ApplicationTraits& traits) {
    i32 max_tasks = traits.max_tasks;
    i32 worker_count = traits.max_task_worker_count;

    g_tasks.max_tasks = max_tasks;
    g_tasks.tasks = new TaskImpl[max_tasks];
    g_tasks.worker_count = worker_count;
    g_tasks.workers = new TaskWorker[worker_count];
    g_tasks.running = true;
    g_tasks.current_frame = 0;
    g_tasks.next_generation = 0;

    for (i32 i = 0; i < max_tasks; i++) {
        TaskImpl& impl = g_tasks.tasks[i];
        impl.state = TASK_STATE_FREE;
        impl.generation = 0;
        impl.destroy_func = nullptr;
        impl.dependency_count = 0;
        impl.dependency_head = -1;
        impl.dependency_next = -1;
        impl.dependent = -1;
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
