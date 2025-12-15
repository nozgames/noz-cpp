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
        void* result;
        u64 start_frame;
        u32 generation;
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

    static TaskImpl* GetTask(TaskHandle handle) {
        if (handle.generation == 0 || handle.id >= (u32)g_tasks.max_tasks)
            return nullptr;

        TaskImpl& task = g_tasks.tasks[handle.id];
        if (task.generation != handle.generation)
            return nullptr;

        return &task;
    }

    inline TaskHandle GetHandle(TaskImpl* task) {
        return { static_cast<u32>(task - g_tasks.tasks), task->generation };
    }

    TaskHandle CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func) {
        std::lock_guard lock(g_tasks.mutex);

        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            TaskState expected = TASK_STATE_FREE;
            if (!task.state.compare_exchange_strong(expected, TASK_STATE_PENDING))
                continue;

            task.generation = ++g_tasks.next_generation;
            task.run_func = std::move(run_func);
            task.complete_func = std::move(complete_func);
            task.start_frame = g_tasks.current_frame;

            return { static_cast<u32>(i), task.generation };
        }

        return TASK_HANDLE_INVALID;
    }

    bool IsTaskValid(TaskHandle handle) {
        if (handle.generation == 0)
            return false;

        std::lock_guard lock(g_tasks.mutex);
        return GetTask(handle) != nullptr;
    }

    bool IsTaskComplete(TaskHandle handle) {
        if (handle.generation == 0)
            return true;

        std::lock_guard lock(g_tasks.mutex);
        TaskImpl* task = GetTask(handle);
        if (!task)
            return true;  // Invalid handle = treat as complete

        TaskState state = task->state.load();
        return state == TASK_STATE_COMPLETE || state == TASK_STATE_CANCELED;
    }

    bool IsTaskCanceled(TaskHandle handle) {
        if (handle.generation == 0)
            return true;

        if (handle.id >= (u32)g_tasks.max_tasks)
            return true;

        // No lock - this is called frequently from worker thread
        TaskImpl& task = g_tasks.tasks[handle.id];
        if (task.generation != handle.generation)
            return true;

        return task.state.load() == TASK_STATE_CANCELED;
    }

    void CancelTask(TaskHandle handle) {
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

    static TaskImpl* FindOldestPendingTask() {
        TaskImpl* oldest = nullptr;
        u64 oldest_frame = UINT64_MAX;

        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            if (task.state.load() == TASK_STATE_PENDING) {
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

    void UpdateTasks() {
        g_tasks.current_frame++;

        // Process completed tasks on main thread
        if (g_tasks.tasks_completed.exchange(false)) {
            LogInfo("task: processing completed tasks on main thread");

            for (i32 i = 0; i < g_tasks.max_tasks; i++) {
                TaskImpl& task = g_tasks.tasks[i];
                TaskState state = task.state.load();

                if (state == TASK_STATE_COMPLETE) {
                    if (task.complete_func)
                        task.complete_func(GetHandle(&task), task.result);
                    task.run_func = nullptr;
                    task.complete_func = nullptr;
                    task.result = nullptr;
                    task.generation = 0;  // Invalidate handle before freeing
                    task.state.store(TASK_STATE_FREE);
                } else if (state == TASK_STATE_CANCELED) {
                    task.run_func = nullptr;
                    task.complete_func = nullptr;
                    task.result = nullptr;
                    task.generation = 0;  // Invalidate handle before freeing
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
                if (task->state.load() == expected && task->run_func)
                    task->result = task->run_func(GetHandle(task));

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
        }

        for (i32 i = 0; i < worker_count; i++) {
            g_tasks.workers[i].running = false;
            g_tasks.workers[i].current_task = nullptr;
            g_tasks.workers[i].thread = std::thread(WorkerProc, &g_tasks.workers[i], i);
        }

        LogInfo("Task system initialized with %d workers, %d max tasks", worker_count, max_tasks);
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
