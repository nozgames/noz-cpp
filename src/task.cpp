//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace noz {
    struct TaskImpl {
        std::atomic<TaskState> state{TASK_STATE_FREE};
        TaskRunFunc run_func;
        TaskCompleteFunc complete_func;
        void* user_data;
        u64 start_frame;
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
    };

    static TaskSystem g_tasks = {};

    Task* CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func, void* user_data) {
        std::lock_guard lock(g_tasks.mutex);

        for (i32 i = 0; i < g_tasks.max_tasks; i++) {
            TaskImpl& task = g_tasks.tasks[i];
            TaskState expected = TASK_STATE_FREE;
            if (!task.state.compare_exchange_strong(expected, TASK_STATE_PENDING))
                continue;

            task.run_func = run_func;
            task.complete_func = complete_func;
            task.user_data = user_data;
            task.start_frame = g_tasks.current_frame;
            return reinterpret_cast<Task*>(&task);
        }

        return nullptr;
    }

    bool IsTaskComplete(Task* task) {
        if (!task)
            return true;

        TaskImpl* impl = reinterpret_cast<TaskImpl*>(task);
        TaskState state = impl->state.load();
        return state == TASK_STATE_FREE || state == TASK_STATE_COMPLETE || state == TASK_STATE_CANCELED;
    }

    void CancelTask(Task* task) {
        if (!task)
            return;

        TaskImpl* impl = reinterpret_cast<TaskImpl*>(task);
        TaskState expected = TASK_STATE_PENDING;
        if (impl->state.compare_exchange_strong(expected, TASK_STATE_CANCELED)) {
            // Successfully canceled before it started running
            return;
        }

        // If already running, mark for cancellation (won't call complete_func)
        expected = TASK_STATE_RUNNING;
        impl->state.compare_exchange_strong(expected, TASK_STATE_CANCELED);
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
            if (worker.current_task.load() == nullptr) {
                return &worker;
            }
        }
        return nullptr;
    }

    void UpdateTasks() {
        g_tasks.current_frame++;

        // Process completed tasks on main thread
        if (g_tasks.tasks_completed.exchange(false)) {
            for (i32 i = 0; i < g_tasks.max_tasks; i++) {
                TaskImpl& task = g_tasks.tasks[i];
                TaskState state = task.state.load();

                if (state == TASK_STATE_COMPLETE) {
                    if (task.complete_func) {
                        task.complete_func(task.user_data);
                    }
                    task.state.store(TASK_STATE_FREE);
                } else if (state == TASK_STATE_CANCELED) {
                    // Free canceled tasks without calling complete
                    task.state.store(TASK_STATE_FREE);
                }
            }
        }

        // Assign pending tasks to idle workers
        std::lock_guard<std::mutex> lock(g_tasks.mutex);

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

            // Wait for work
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
                // Check if canceled before running
                TaskState expected = TASK_STATE_RUNNING;
                if (task->state.load() == expected && task->run_func) {
                    task->run_func(task->user_data);
                }

                // Only mark complete if not canceled
                expected = TASK_STATE_RUNNING;
                if (task->state.compare_exchange_strong(expected, TASK_STATE_COMPLETE)) {
                    g_tasks.tasks_completed.store(true);
                }
            }

            worker->current_task.store(nullptr);
        }
    }

    void InitTasks(i32 worker_count, i32 max_tasks) {
        g_tasks.max_tasks = max_tasks;
        g_tasks.tasks = new TaskImpl[max_tasks];
        g_tasks.worker_count = worker_count;
        g_tasks.workers = new TaskWorker[worker_count];
        g_tasks.running = true;
        g_tasks.current_frame = 0;

        for (i32 i = 0; i < max_tasks; i++) {
            g_tasks.tasks[i].state = TASK_STATE_FREE;
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
            if (g_tasks.workers[i].thread.joinable()) {
                g_tasks.workers[i].thread.join();
            }
        }

        delete[] g_tasks.workers;
        g_tasks.workers = nullptr;

        delete[] g_tasks.tasks;
        g_tasks.tasks = nullptr;

        LogInfo("Task system shutdown");
    }

} // namespace noz
