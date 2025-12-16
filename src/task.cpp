//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <noz/task.h>

namespace noz {

    struct TaskImpl {
        std::atomic<TaskState> state{TASK_STATE_FREE};
        TaskRunFunc run_func;
        TaskCompleteFunc complete_func;
        TaskDestroyFunc destroy_func;
        void* result;
        Task parent;
        i32 first_dep;      // Index of first dependency (-1 if none)
        i32 next_dep;       // Next dependency in parent's chain (-1 if last)
        u64 start_frame;
        u32 generation;
        bool is_virtual;
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

    static TaskSystem g_tasks = {};

    static TaskImpl* GetTask(Task handle) {
        if (handle.generation == 0 || handle.id >= static_cast<u32>(g_tasks.max_tasks))
            return nullptr;

        TaskImpl& task = g_tasks.tasks[handle.id];
        if (task.generation != handle.generation)
            return nullptr;

        return &task;
    }

    inline Task GetHandle(TaskImpl* task) {
        return { static_cast<u32>(task - g_tasks.tasks), task->generation };
    }

    static Task CreateTaskInternal(TaskRunFunc run_func, TaskCompleteFunc complete_func,
                                          TaskDestroyFunc destroy_func, const Task* deps, i32 dep_count) {
        std::lock_guard lock(g_tasks.mutex);

        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            TaskState expected = TASK_STATE_FREE;
            if (!task.state.compare_exchange_strong(expected, TASK_STATE_PENDING))
                continue;

            task.generation = ++g_tasks.next_generation;
            task.run_func = std::move(run_func);
            task.complete_func = std::move(complete_func);
            task.destroy_func = std::move(destroy_func);
            task.parent = nullptr;
            task.result = nullptr;
            task.start_frame = g_tasks.current_frame;
            task.is_virtual = false;

            // Set up dependency linked list
            if (dep_count > 0) {
                task.first_dep = static_cast<i32>(deps[0].id);
                for (i32 j = 0; j < dep_count - 1; j++) {
                    TaskImpl& dep = g_tasks.tasks[deps[j].id];
                    assert(dep.next_dep == -1 && "Task is already a dependency of another task");
                    dep.next_dep = static_cast<i32>(deps[j + 1].id);
                }
                // Last dependency has no next
                g_tasks.tasks[deps[dep_count - 1].id].next_dep = -1;
            } else {
                task.first_dep = -1;
            }
            task.next_dep = -1;

            return { static_cast<u32>(i), task.generation };
        }

        return nullptr;
    }

    Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        TaskDestroyFunc destroy_func) {
        return CreateTaskInternal(std::move(run_func), std::move(complete_func), std::move(destroy_func), nullptr, 0);
    }

    Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        Task depends_on,
        TaskDestroyFunc destroy_func) {
        return CreateTaskInternal(std::move(run_func), std::move(complete_func), std::move(destroy_func), &depends_on, 1);
    }

    Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        std::initializer_list<Task> depends_on,
        TaskDestroyFunc destroy_func) {
        return CreateTaskInternal(std::move(run_func), std::move(complete_func), std::move(destroy_func),
                                  depends_on.begin(), static_cast<i32>(depends_on.size()));
    }

    Task CreateTask(
        TaskRunFunc run_func,
        TaskCompleteFunc complete_func,
        const Task* depends_on,
        int count,
        TaskDestroyFunc destroy_func) {
        return CreateTaskInternal(
            std::move(run_func),
            std::move(complete_func),
            std::move(destroy_func),
            depends_on,
            count);
    }

    Task CreateTask(TaskDestroyFunc destroy_func) {
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
            task.first_dep = -1;
            task.next_dep = -1;
            task.parent = nullptr;
            task.result = nullptr;
            task.start_frame = g_tasks.current_frame;
            task.is_virtual = true;

            return { static_cast<u32>(task_index), task.generation };
        }

        return nullptr;
    }

    void Complete(Task handle, void* result) {
        if (handle.generation == 0)
            return;

        std::lock_guard lock(g_tasks.mutex);
        TaskImpl* task = GetTask(handle);
        if (!task || !task->is_virtual)
            return;

        task->result = result;
        task->state.store(TASK_STATE_COMPLETE);
        g_tasks.tasks_completed.store(true);
    }

    void* GetResult(Task handle) {
        if (handle.generation == 0)
            return nullptr;

        if (handle.id >= static_cast<u32>(g_tasks.max_tasks))
            return nullptr;

        TaskImpl& task = g_tasks.tasks[handle.id];
        if (task.generation != handle.generation)
            return nullptr;

        return task.result;
    }

    void* ReleaseResult(Task handle) {
        if (handle.generation == 0)
            return nullptr;

        std::lock_guard lock(g_tasks.mutex);
        TaskImpl* task = GetTask(handle);
        if (!task)
            return nullptr;

        void* result = task->result;
        task->result = nullptr;
        task->destroy_func = nullptr;  // Caller takes ownership, no cleanup needed
        return result;
    }

    int GetDependencyCount(Task handle) {
        if (handle.generation == 0)
            return 0;

        TaskImpl* task = GetTask(handle);
        if (!task)
            return 0;

        int count = 0;
        i32 dep_idx = task->first_dep;
        while (dep_idx >= 0) {
            count++;
            dep_idx = g_tasks.tasks[dep_idx].next_dep;
        }
        return count;
    }

    Task GetDependencyAt(Task handle, int index) {
        if (handle.generation == 0 || index < 0)
            return nullptr;

        TaskImpl* task = GetTask(handle);
        if (!task)
            return nullptr;

        i32 dep_idx = task->first_dep;
        for (int i = 0; i < index && dep_idx >= 0; i++) {
            dep_idx = g_tasks.tasks[dep_idx].next_dep;
        }

        if (dep_idx < 0)
            return nullptr;

        TaskImpl& dep = g_tasks.tasks[dep_idx];
        return { static_cast<u32>(dep_idx), dep.generation };
    }

    Task GetParent(Task handle) {
        if (handle.generation == 0)
            return nullptr;

        TaskImpl* task = GetTask(handle);
        if (!task)
            return nullptr;

        return task->parent;
    }

    void SetParent(Task handle, Task parent) {
        if (handle.generation == 0)
            return;

        std::lock_guard lock(g_tasks.mutex);
        TaskImpl* task = GetTask(handle);
        if (task)
            task->parent = parent;
    }

    bool IsTaskValid(Task handle) {
        if (handle.generation == 0)
            return false;

        std::lock_guard lock(g_tasks.mutex);
        return GetTask(handle) != nullptr;
    }

    bool IsComplete(Task handle) {
        if (handle.generation == 0)
            return true;

        std::lock_guard lock(g_tasks.mutex);
        TaskImpl* task = GetTask(handle);
        if (!task)
            return true;

        TaskState state = task->state.load();
        return state == TASK_STATE_COMPLETE || state == TASK_STATE_CANCELED;
    }

    bool IsCancelled(Task handle) {
        if (handle.generation == 0)
            return true;

        if (handle.id >= (u32)g_tasks.max_tasks)
            return true;

        // No lock - this is called frequently from worker thread
        TaskImpl& task = g_tasks.tasks[handle.id];
        if (task.generation != handle.generation)
            return true;

        if (task.state.load() == TASK_STATE_CANCELED)
            return true;

        // Check parent chain - if parent is cancelled, we're cancelled too
        if (task.parent)
            return IsCancelled(task.parent);

        return false;
    }

    void Cancel(Task handle) {
        if (handle.generation == 0)
            return;

        std::lock_guard lock(g_tasks.mutex);
        TaskImpl* task = GetTask(handle);
        if (!task)
            return;

        TaskState expected = TASK_STATE_PENDING;
        if (task->state.compare_exchange_strong(expected, TASK_STATE_CANCELED))
            return;

        expected = TASK_STATE_RUNNING;
        task->state.compare_exchange_strong(expected, TASK_STATE_CANCELED);
    }

    static bool AreAllDependenciesReady(TaskImpl& task) {
        i32 dep_idx = task.first_dep;
        while (dep_idx >= 0) {
            TaskImpl& dep = g_tasks.tasks[dep_idx];
            if (dep.state.load() != TASK_STATE_COMPLETE)
                return false;
            dep_idx = dep.next_dep;
        }
        return true;
    }

    static TaskImpl* FindOldestPendingTask() {
        TaskImpl* oldest = nullptr;
        u64 oldest_frame = UINT64_MAX;

        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            if (task.state.load() == TASK_STATE_PENDING) {
                if (!AreAllDependenciesReady(task))
                    continue;

                if (task.start_frame < oldest_frame) {
                    oldest_frame = task.start_frame;
                    oldest = &task;
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

    static bool HasPendingDependents(Task handle) {
        i32 target_idx = static_cast<i32>(handle.id);
        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            if (task.state.load() != TASK_STATE_PENDING)
                continue;

            i32 dep_idx = task.first_dep;
            while (dep_idx >= 0) {
                if (dep_idx == target_idx)
                    return true;
                dep_idx = g_tasks.tasks[dep_idx].next_dep;
            }
        }
        return false;
    }

    void UpdateTasks() {
        g_tasks.current_frame++;

        // Process completed tasks on main thread
        if (g_tasks.tasks_completed.exchange(false)) {
            for (i32 i = 0; i < g_tasks.max_tasks; i++) {
                TaskImpl& task = g_tasks.tasks[i];
                TaskState state = task.state.load();

                if (state == TASK_STATE_COMPLETE) {
                    Task handle = GetHandle(&task);

                    if (task.complete_func)
                        task.complete_func(handle, task.result);

                    // Don't free if pending tasks still depend on us
                    if (HasPendingDependents(handle))
                        continue;

                    // Call destructor before freeing
                    if (task.destroy_func)
                        task.destroy_func(task.result);

                    task.run_func = nullptr;
                    task.complete_func = nullptr;
                    task.destroy_func = nullptr;
                    task.result = nullptr;
                    task.generation = 0;
                    task.state.store(TASK_STATE_FREE);
                } else if (state == TASK_STATE_CANCELED) {
                    Task handle = GetHandle(&task);

                    if (HasPendingDependents(handle))
                        continue;

                    // Call destructor before freeing
                    if (task.destroy_func)
                        task.destroy_func(task.result);

                    task.run_func = nullptr;
                    task.complete_func = nullptr;
                    task.destroy_func = nullptr;
                    task.result = nullptr;
                    task.generation = 0;
                    task.state.store(TASK_STATE_FREE);
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
    }

    static void WorkerProc(TaskWorker* worker, int worker_index) {
        char name[32];
        snprintf(name, sizeof(name), "task_worker_%d", worker_index);
        SetThreadName(name);

        worker->running = true;

        while (worker->running) {
            TaskImpl* task = nullptr;

            {
                std::unique_lock lock(worker->cv_mutex);
                worker->cv.wait(lock, [worker] {
                    return worker->current_task.load() != nullptr || !worker->running;
                });

                if (!worker->running)
                    break;

                task = worker->current_task.load();
            }

            if (task) {
                TaskState expected = TASK_STATE_RUNNING;
                if (task->state.load() == expected && task->run_func) {
                    task->result = task->run_func(GetHandle(task));
                }

                expected = TASK_STATE_RUNNING;
                if (task->state.compare_exchange_strong(expected, TASK_STATE_COMPLETE))
                    g_tasks.tasks_completed.store(true);
            }

            worker->current_task.store(nullptr);
        }
    }

    void InitTasks(const ApplicationTraits& traits) {
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
            g_tasks.tasks[i].state = TASK_STATE_FREE;
            g_tasks.tasks[i].generation = 0;
            g_tasks.tasks[i].destroy_func = nullptr;
            g_tasks.tasks[i].first_dep = -1;
            g_tasks.tasks[i].next_dep = -1;
        }

        for (i32 i = 0; i < worker_count; i++) {
            g_tasks.workers[i].running = false;
            g_tasks.workers[i].current_task = nullptr;
            g_tasks.workers[i].thread = std::thread(WorkerProc, &g_tasks.workers[i], i);
        }
    }

    void ShutdownTasks() {
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

} // namespace noz
