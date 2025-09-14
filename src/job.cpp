//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <mutex>
#include <semaphore>

constexpr int MAX_CONCURRENT_JOBS = 16;
constexpr int MAX_JOBS = 1024;

struct Job
{
    LinkedListNode node;
    JobRunFunc func;
    void* user_data;
    bool done;
    std::thread* thread;
    u32 version;
};

struct JobThread
{
    Job* job;
    std::thread thread;
    std::counting_semaphore<1> semaphore{0};
    std::atomic<bool> running;
};

struct JobSystem
{
    LinkedList queue;
    std::mutex* mutex;
    std::thread* queue_thread;
    JobThread* threads;
    bool running;
    PoolAllocator* job_allocator;
    u32 job_version[MAX_JOBS];
    u32 next_version;
};

static JobSystem g_jobs = {};

JobHandle CreateJob(JobRunFunc func, void* user_data)
{
    Job* job = (Job*)Alloc(g_jobs.job_allocator, sizeof(Job));
    if (!job)
        return { 0, 0 };

    job->version = g_jobs.next_version++;
    job->func = func;
    job->user_data = user_data;

    g_jobs.mutex->lock();
    PushBack(g_jobs.queue, job);
    g_jobs.mutex->unlock();

    return { GetIndex(g_jobs.job_allocator, job), job->version };
}

static void JobThreadProc(JobThread *job_thread)
{
    job_thread->running = true;

    while (job_thread->running)
    {
        job_thread->semaphore.acquire();

        if (!job_thread->running)
            break;

        if (job_thread->job && job_thread->job->func)
        {
            job_thread->job->func(job_thread->job->user_data);
            job_thread->job->done = true;
        }
    }
}

static void JobSchedulerProc()
{
    while (g_jobs.running)
    {
        g_jobs.mutex->lock();

        // Clean up finished jobs
        for (int thread_index = 0; thread_index < MAX_CONCURRENT_JOBS; thread_index++)
        {
            JobThread& thread = g_jobs.threads[thread_index];
            if (thread.job == nullptr)
                continue;

            if (!thread.job->done)
                continue;

            u32 job_index = GetIndex(g_jobs.job_allocator, thread.job);
            Free(thread.job);
            thread.job = nullptr;
            g_jobs.job_version[job_index] = 0;
        }

        // Start queued jobs
        for (int i = 0; i < MAX_CONCURRENT_JOBS && !IsEmpty(g_jobs.queue); i++)
        {
            JobThread& thread = g_jobs.threads[i];
            if (thread.job != nullptr)
                continue;

            Job* job = (Job*)GetFront(g_jobs.queue);
            Remove(g_jobs.queue, job);

            thread.job = job;
            thread.semaphore.release();
        }

        g_jobs.mutex->unlock();

        std::this_thread::yield();
    }
}

bool IsDone(JobHandle handle)
{
    assert(handle.id < MAX_JOBS);

    g_jobs.mutex->lock();
    bool done = g_jobs.job_version[handle.id] != handle.version;
    g_jobs.mutex->unlock();
    return done;
}

void InitJobs()
{
    Init(g_jobs.queue, offsetof(JobSystem, queue));

    g_jobs.job_allocator = CreatePoolAllocator(sizeof(Job), MAX_JOBS);
    g_jobs.next_version = 1;
    g_jobs.running = true;
    g_jobs.threads = new JobThread[MAX_CONCURRENT_JOBS];
    g_jobs.mutex = new std::mutex();

    for (int i = 0; i < MAX_CONCURRENT_JOBS; i++)
    {
        JobThread& thread = g_jobs.threads[i];
        thread.running = false;
        thread.job = nullptr;
        thread.thread = std::thread(JobThreadProc, &g_jobs.threads[i]);
    }

    g_jobs.queue_thread = new std::thread(JobSchedulerProc);
}

void ShutdownJobs()
{
    g_jobs.running = false;

    if (g_jobs.mutex)
    {
        g_jobs.mutex->lock();
        for (int i=0; i < MAX_CONCURRENT_JOBS; i++)
        {
            g_jobs.threads[i].running = false;
            g_jobs.threads[i].semaphore.release();
        }
        g_jobs.mutex->unlock();

        for (int i=0; i < MAX_CONCURRENT_JOBS; i++)
            g_jobs.threads[i].thread.join();
    }

    if (g_jobs.queue_thread)
    {
        g_jobs.queue_thread->join();
        delete g_jobs.queue_thread;
    }

    if (g_jobs.mutex)
        delete g_jobs.mutex;

    g_jobs = {};
}

