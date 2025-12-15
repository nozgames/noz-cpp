//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace noz {

struct Task {};

using TaskRunFunc = void (*)(void* user_data);
using TaskCompleteFunc = void (*)(void* user_data);

extern void InitTasks(i32 worker_count, i32 max_tasks);
extern void ShutdownTasks();
extern void UpdateTasks();

extern Task* CreateTask(TaskRunFunc run_func, TaskCompleteFunc complete_func, void* user_data);
extern bool IsTaskComplete(Task* task);
extern void CancelTask(Task* task);

} // namespace noz
