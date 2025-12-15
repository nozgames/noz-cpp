//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

namespace noz {

    struct Task;

    enum TaskState {
        TASK_STATE_PENDING,
        TASK_STATE_RUNNING,
        TASK_STATE_COMPLETED,
        TASK_STATE_CANCELED
    };

    typedef void (*TaskUpdateFunc)(Task* task, void* user_data);
    typedef void (*TaskCancelFunc)(Task* task, void* user_data);

    struct TaskImpl : Task {
        TaskUpdateFunc update_func;
        TaskCancelFunc cancel_func;
        u64 start_frame;
        TaskState state;
    };

    struct TaskSystem {
        PoolAllocator* tasks;
        i32 run_count;
        bool state_changed;
    };

    static TaskSystem g_task_system = {};

    Task* CreateTask(Allocator* allocator) {
        (void)allocator;

        TaskImpl* impl = static_cast<TaskImpl*>(Alloc(g_task_system.tasks, sizeof(TaskImpl)));
        if (!impl)
            return nullptr;

        impl->start_frame = GetFrameIndex();

        return nullptr;
    }

    void UpdateTasks() {

    }

    void InitTasks(const ApplicationTraits& traits) {
        g_task_system.tasks = CreatePoolAllocator(sizeof(TaskImpl), traits.max_tasks);
    }
}
