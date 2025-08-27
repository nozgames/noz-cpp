//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @STL

#include "file_watcher.h"
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <chrono>

#ifndef nullptr
#define nullptr NULL
#endif

#define MAX_WATCHED_DIRS 32
#define MAX_EVENTS_QUEUE 4096
#define MAX_TRACKED_FILES 16384

struct FileInfo
{
    std::filesystem::path path;
    uint64_t modified_time;
    uint64_t size;
};

struct FileQueue
{
    FileChangeEvent events[MAX_EVENTS_QUEUE];
    size_t head;
    size_t tail;
    size_t count;
    SDL_Mutex* mutex;
};

// Global file watcher state
struct FileWatcher
{
    int poll_interval_ms;
    std::vector<std::filesystem::path> watched_dirs;
    std::map<std::string, FileInfo> file_map;
    FileQueue queue;
    SDL_Thread* thread;
    SDL_AtomicInt should_stop;
    SDL_Mutex* dirs_mutex;
    bool initialized;
    bool running;
};

static FileWatcher g_watcher = {};

static int FileWatcherThread(void* data);
static void scan_directory_recursive(const char* dir_path);
static void process_file(const char* file_path, uint64_t modified_time, uint64_t size);
static void queue_event(const std::filesystem::path& path, FileChangeType type);
static bool StartFileWatcher();
static void scan_directory_files(const std::filesystem::path& dir_path);

void InitFileWatcher(int poll_interval_ms)
{
    if (g_watcher.initialized)
        return;

    g_watcher.poll_interval_ms = poll_interval_ms > 0 ? poll_interval_ms : 1000;
    
    // Initialize STL containers (they're automatically initialized)
    
    // Initialize queue
    g_watcher.queue.head = 0;
    g_watcher.queue.tail = 0;
    g_watcher.queue.count = 0;
    g_watcher.queue.mutex = SDL_CreateMutex();
    
    // Initialize synchronization
    SDL_SetAtomicInt(&g_watcher.should_stop, 0);
    g_watcher.dirs_mutex = SDL_CreateMutex();
    
    g_watcher.initialized = true;
    g_watcher.running = false;
}

void ShutdownFileWatcher()
{
    if (!g_watcher.initialized)
        return;

    // Signal thread to stop
    SDL_SetAtomicInt(&g_watcher.should_stop, 1);

    // Wait for thread to finish
    if (g_watcher.thread)
    {
        SDL_WaitThread(g_watcher.thread, nullptr);
        g_watcher.thread = nullptr;
    }
    
    // Clean up STL containers (they clean themselves up automatically)
    g_watcher.watched_dirs.clear();
    g_watcher.file_map.clear();
    
    // Clean up synchronization
    if (g_watcher.queue.mutex)
    {
        SDL_DestroyMutex(g_watcher.queue.mutex);
        g_watcher.queue.mutex = nullptr;
    }
    
    if (g_watcher.dirs_mutex)
    {
        SDL_DestroyMutex(g_watcher.dirs_mutex);
        g_watcher.dirs_mutex = nullptr;
    }
    
    g_watcher.initialized = false;
}

// Add a directory to watch
bool WatchDirectory(const std::filesystem::path& directory)
{
    if (!g_watcher.initialized || directory.empty())
    {
        return false;
    }
    
    std::string dir_str = directory.string();
    const char* dir_cstr = dir_str.c_str();
    
    SDL_LockMutex(g_watcher.dirs_mutex);
    
    // Check if already watching this directory
    auto it = std::find(g_watcher.watched_dirs.begin(), g_watcher.watched_dirs.end(), directory);
    if (it != g_watcher.watched_dirs.end())
    {
        SDL_UnlockMutex(g_watcher.dirs_mutex);
        return true;  // Already watching
    }
    
    // Add new directory
    if (g_watcher.watched_dirs.size() >= MAX_WATCHED_DIRS)
    {
        SDL_UnlockMutex(g_watcher.dirs_mutex);
        return false;  // Too many directories
    }
    
    g_watcher.watched_dirs.push_back(directory);
    
    // Auto-start the watcher when the first directory is added
    bool should_start = !g_watcher.running && g_watcher.watched_dirs.size() == 1;
    
    SDL_UnlockMutex(g_watcher.dirs_mutex);
    
    if (should_start)
    {
        // Start the file watcher automatically
        StartFileWatcher();
    }
    else if (g_watcher.running)
    {
        // If already running, do an initial scan of the new directory
        scan_directory_recursive(dir_str.c_str());
    }
    
    return true;
}


static bool StartFileWatcher()
{
    if (!g_watcher.initialized || g_watcher.running)
        return false;

    if (g_watcher.watched_dirs.empty())
        return false;  // No directories to watch

    // Do initial scan of all directories
    SDL_LockMutex(g_watcher.dirs_mutex);
    for (const auto& dir : g_watcher.watched_dirs)
    {
        scan_directory_recursive(dir.string().c_str());
    }
    SDL_UnlockMutex(g_watcher.dirs_mutex);
    
    // Start the watching thread
    SDL_SetAtomicInt(&g_watcher.should_stop, 0);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4191) // SDL_CreateThread causes this on Windows
#endif
    g_watcher.thread = SDL_CreateThread(FileWatcherThread, "FileWatcher", NULL);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
    if (!g_watcher.thread)
    {
        return false;
    }
    
    g_watcher.running = true;
    return true;
}



// Poll for file changes
bool GetFileChangeEvent(FileChangeEvent* event)
{
    if (!g_watcher.initialized || !event)
    {
        return false;
    }
    
    SDL_LockMutex(g_watcher.queue.mutex);
    
    if (g_watcher.queue.count == 0)
    {
        SDL_UnlockMutex(g_watcher.queue.mutex);
        return false;
    }
    
    // Dequeue event
    *event = g_watcher.queue.events[g_watcher.queue.head];
    g_watcher.queue.head = (g_watcher.queue.head + 1) % MAX_EVENTS_QUEUE;
    g_watcher.queue.count--;
    
    SDL_UnlockMutex(g_watcher.queue.mutex);
    return true;
}

static int FileWatcherThread(void* data)
{
    (void)data;
    
    while (SDL_GetAtomicInt(&g_watcher.should_stop) == 0)
    {
        // Mark all existing files as "not seen"
        for (auto& pair : g_watcher.file_map)
        {
            pair.second.size = 0;  // Mark as not seen
        }
        
        // Scan all watched directories
        SDL_LockMutex(g_watcher.dirs_mutex);
        for (const auto& dir : g_watcher.watched_dirs)
        {
            scan_directory_recursive(dir.string().c_str());
        }
        SDL_UnlockMutex(g_watcher.dirs_mutex);
        
        // Check for deleted files (files that weren't seen in this pass)
        auto it = g_watcher.file_map.begin();
        while (it != g_watcher.file_map.end())
        {
            if (it->second.size == 0)
            {
                // File was not seen in this pass, it was deleted
                queue_event(it->second.path, FILE_CHANGE_TYPE_DELETED);
                it = g_watcher.file_map.erase(it);
            }
            else
            {
                ++it;
            }
        }
        
        // Sleep for poll interval
        SDL_Delay(g_watcher.poll_interval_ms);
    }
    
    return 0;
}

// Callback function for STL filesystem directory scanning
static void scan_directory_files(const std::filesystem::path& dir_path)
{
    std::error_code ec;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path, ec))
    {
        if (ec)
        {
            // Skip directories that we can't access
            ec.clear();
            continue;
        }
        
        if (entry.is_regular_file(ec) && !ec)
        {
            auto file_time = entry.last_write_time(ec);
            if (!ec)
            {
                auto size = entry.file_size(ec);
                if (!ec)
                {
                    // Convert filesystem time to timestamp
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(sctp.time_since_epoch()).count();
                    
                    process_file(entry.path().string().c_str(), timestamp, size);
                }
            }
        }
    }
}

// STL filesystem implementation
static void scan_directory_recursive(const char* dir_path)
{
    std::filesystem::path path(dir_path);
    scan_directory_files(path);
}

// Process a single file (STL filesystem)
static void process_file(const char* file_path, uint64_t modified_time, uint64_t size)
{
    std::filesystem::path path(file_path);
    std::string path_str = path.string();
    
    auto it = g_watcher.file_map.find(path_str);
    
    if (it != g_watcher.file_map.end())
    {
        // File already tracked, check for modifications
        FileInfo& existing = it->second;
        if (existing.modified_time != modified_time)
        {
            // File was modified
            queue_event(existing.path, FILE_CHANGE_TYPE_MODIFIED);
            existing.modified_time = modified_time;
        }
        // Mark as seen by restoring the size
        existing.size = size;
    }
    else
    {
        // New file
        FileInfo info;
        info.path = path;
        info.modified_time = modified_time;
        info.size = size;
        
        g_watcher.file_map[path_str] = info;
        queue_event(info.path, FILE_CHANGE_TYPE_ADDED);
    }
}

// Queue an event
static void queue_event(const std::filesystem::path& path, FileChangeType type)
{
    SDL_LockMutex(g_watcher.queue.mutex);
    
    if (g_watcher.queue.count >= MAX_EVENTS_QUEUE)
    {
        // Queue is full, drop oldest event
        g_watcher.queue.head = (g_watcher.queue.head + 1) % MAX_EVENTS_QUEUE;
        g_watcher.queue.count--;
    }
    
    // Add new event
    FileChangeEvent* event = &g_watcher.queue.events[g_watcher.queue.tail];
    event->path = path;
    event->type = type;
    event->timestamp = SDL_GetTicks();
    
    g_watcher.queue.tail = (g_watcher.queue.tail + 1) % MAX_EVENTS_QUEUE;
    g_watcher.queue.count++;
    
    SDL_UnlockMutex(g_watcher.queue.mutex);
}

