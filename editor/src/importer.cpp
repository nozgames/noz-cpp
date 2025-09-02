//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/platform.h>
#include "file_watcher.h"
#include "asset_manifest.h"
#include "server.h"

namespace fs = std::filesystem;

extern AssetImporterTraits* GetShaderImporterTraits();
extern AssetImporterTraits* GetTextureImporterTraits();
extern AssetImporterTraits* GetFontImporterTraits();
extern AssetImporterTraits* GetMeshImporterTraits();
extern AssetImporterTraits* GetStyleSheetImporterTraits();
extern AssetImporterTraits* GetVfxImporterTraits();


struct ImportJob
{
    fs::path source_path;
    AssetImporterTraits* importer;

    ImportJob(const fs::path& path, AssetImporterTraits* imp)
        : source_path(path), importer(imp) {}
};

static std::vector<ImportJob> g_import_queue;
static Props* g_config = nullptr;
static std::atomic<bool> g_running{false};
static std::unique_ptr<std::thread> g_importer_thread;
static std::atomic<bool> g_thread_running{false};

// Importer uses NoZ logging system

void ProcessFileChange(const fs::path& file_path, FileChangeType change_type, std::vector<AssetImporterTraits*>& importers);
bool ProcessImportQueue(std::vector<AssetImporterTraits*>& importers);

// Helper function to derive file extension from asset signature
// GetExtensionFromSignature is now provided by asset.h

void signal_handler(int sig)
{
    if (sig != SIGINT)
        return;

    LogInfo("Shutting down...");
    g_running = false;
}

static bool LoadConfig()
{
    std::filesystem::path config_path = "./editor.cfg";
    if (Stream* config_stream = LoadStream(nullptr, config_path))
    {
        g_config = Props::Load(config_stream);
        Free(config_stream);

        if (g_config != nullptr)
        {
            LogInfo("loaded configuration '%s'", config_path.string().c_str());
            return true;
        }
    }

    LogError("missing configuration '%s'", config_path.string().c_str());
    return false;
}

void ProcessFileChange(const fs::path& file_path, FileChangeType change_type, std::vector<AssetImporterTraits*>& importers)
{
    if (change_type == FILE_CHANGE_TYPE_DELETED)
        return; // Don't process deleted files

    // Check if this is a .meta file
    std::string ext = file_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".meta")
    {
        // Remove .meta extension to get the asset file path
        fs::path asset_path = file_path;
        asset_path.replace_extension("");

        // Check if the associated asset file exists
        if (fs::exists(asset_path) && fs::is_regular_file(asset_path))
        {
            ProcessFileChange(asset_path, change_type, importers);
        }
        return;
    }

    // Find an importer that can handle this file based on extension
    AssetImporterTraits* selected_importer = nullptr;

    std::string file_ext = file_path.extension().string();
    std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);

    for (auto* importer : importers)
    {
        if (importer && importer->file_extensions)
        {
            // Check if this importer supports the file extension
            for (const char** ext_ptr = importer->file_extensions; *ext_ptr != nullptr; ++ext_ptr)
            {
                if (file_ext == *ext_ptr)
                {
                    selected_importer = importer;
                    break;
                }
            }
            if (selected_importer)
                break;
        }
    }

    // has an importer?
    if (!selected_importer)
        return;

    // Check if already in the queue
    auto it = std::find_if(g_import_queue.begin(), g_import_queue.end(),
        [&file_path](const ImportJob& job) {
            return job.source_path == file_path;
        });

    if (it != g_import_queue.end())
        return; // Already in queue

    // Add new job to queue
    g_import_queue.emplace_back(file_path, selected_importer);
}

bool ProcessImportQueue(std::vector<AssetImporterTraits*>& importers)
{
    if (g_import_queue.empty())
        return false;

    // Get output directory from config
    auto output_dir = g_config->GetString("output", "directory", "assets");

    // Convert to filesystem::path
    fs::path output_path = fs::absolute(fs::path(output_dir));

    // Ensure output directory exists
    fs::create_directories(output_path);

    std::vector<ImportJob> remaining_jobs;
    bool made_progress = true;
    bool any_imports_processed = false;

    // Keep processing until no more progress is made
    while (made_progress && !g_import_queue.empty())
    {
        made_progress = false;
        remaining_jobs.clear();

        for (const auto& job : g_import_queue)
        {
            bool can_import_now = true;

            // Check dependencies if the importer supports it
            if (job.importer->does_depend_on)
            {
                // Check if any files this one depends on are still in the queue
                for (const auto& other_job : g_import_queue)
                {
                    if (&job == &other_job)
                        continue; // Don't check against self

                    if (job.importer->does_depend_on(job.source_path, other_job.source_path))
                    {
                        can_import_now = false;
                        break;
                    }
                }
            }

            if (can_import_now)
            {
                // Import this file
                if (job.importer->import_func)
                {
                    try
                    {
                        // Create output stream
                        Stream* output_stream = CreateStream(nullptr, 4096);
                        if (!output_stream)
                        {
                            std::cout << job.source_path.string() << ": error: Failed to create output stream" << std::endl;
                            continue;
                        }

                        // Load .meta file or create default props
                        Props* meta = nullptr;
                        if (auto meta_stream = LoadStream(nullptr, fs::path(job.source_path.string() + ".meta")))
                        {
                            meta = Props::Load(meta_stream);
                            Free(meta_stream);
                        }

                        // Create default props if meta file failed to load
                        if (!meta)
                            meta = new Props();

                        // Call the importer
                        job.importer->import_func(job.source_path, output_stream, g_config, meta);

                        delete meta;

                        // Build output file path with correct extension
                        fs::path relative_path;
                        bool found_relative = false;

                        // Get source directories from config and find the relative path
                        auto source_list = g_config->GetKeys("source");
                        for (auto& source_dir_str : source_list)
                        {
                            fs::path source_dir(source_dir_str);
                            std::error_code ec;
                            relative_path = fs::relative(job.source_path, source_dir, ec);
                            if (!ec && !relative_path.empty() && relative_path.string().find("..") == std::string::npos)
                            {
                                found_relative = true;
                                break;
                            }
                        }

                        if (!found_relative)
                            relative_path = job.source_path.filename();

                        // Build final output path with extension derived from signature
                        fs::path final_path = output_path / relative_path;
                        std::string derived_extension = GetExtensionFromSignature(job.importer->signature);
                        final_path.replace_extension(derived_extension);

                        // Ensure output directory exists
                        fs::create_directories(final_path.parent_path());

                        // Save the output stream
                        if (!SaveStream(output_stream, final_path))
                        {
                            Free(output_stream);
                            throw std::runtime_error("Failed to save output file");
                        }

                        Free(output_stream);

                        // Print success message using the relative path we computed
                        fs::path asset_path = relative_path;
                        asset_path.replace_extension("");
                        std::string asset_name = asset_path.string();
                        std::replace(asset_name.begin(), asset_name.end(), '\\', '/');
                        LogInfo("Imported \033[38;2;128;128;128m%s", asset_name.c_str());

                        // todo: format nicely using TStringBuilder

                        // Broadcast hotload message
                        BroadcastAssetChange(asset_name);
                    }
                    catch (const std::exception& e)
                    {
                        std::cout << job.source_path.string() << ": error: " << e.what() << std::endl;
                        continue; // Skip to next job
                    }
                }
                made_progress = true;
                any_imports_processed = true;
            }
            else
            {
                // Keep this job for next iteration
                remaining_jobs.push_back(job);
            }
        }

        // Swap the queues - remaining_jobs becomes the new import queue
        g_import_queue = std::move(remaining_jobs);
    }

    return any_imports_processed;
}

static int RunImporterLoop()
{
    if (!LoadConfig())
        return 1;

    std::vector importers = {
        GetShaderImporterTraits(),
        GetTextureImporterTraits(),
        GetFontImporterTraits(),
        GetMeshImporterTraits(),
        GetStyleSheetImporterTraits(),
        GetVfxImporterTraits()
    };

    // Set up signal handler for Ctrl-C
    signal(SIGINT, signal_handler);

    InitFileWatcher(500);

    // Get source directories from config
    if (!g_config->HasGroup("source"))
    {
        LogError("No [source] section found in config");
        ShutdownFileWatcher();
        return 1;
    }

    // Add directories to watch (file watcher will auto-start when first directory is added)
    LogInfo("Adding directories to watch:");
    auto source = g_config->GetKeys("source");
    for (const auto& source_dir_str : source)
    {
        LogInfo("  - %s", source_dir_str.c_str());
        if (!WatchDirectory(fs::path(source_dir_str)))
            LogWarning("Failed to add directory '%s'", source_dir_str.c_str());
    }

    LogInfo("Watching for file changes...");

    while (g_running)
    {
        // Dont spin too fast
        thread_sleep_ms(100);

        // Update hotload server


        // Enqueue changed files
        FileChangeEvent event;
        while (GetFileChangeEvent(&event))
            ProcessFileChange(event.path, event.type, importers);

        // Process queue
        if (!ProcessImportQueue(importers))
            continue;

        // Generate a new asset manifest
        auto output_dir = g_config->GetString("output", "directory", "assets");
        auto manifest_path = g_config->GetString("manifest", "output_file", "src/assets.cpp");
        GenerateAssetManifest(fs::path(output_dir), fs::path(manifest_path), importers, g_config);

        // Initialize hotload server after first manifest generation if enabled
        static bool hotload_initialized = false;
        if (!hotload_initialized && g_config->HasGroup("hotload"))
        {
            int hotload_port = g_config->GetInt("hotload", "port", 8080);
            if (InitEditorServer(hotload_port))
            {
                LogInfo("Hotload server initialized on port %d", hotload_port);
            }
            else
            {
                LogWarning("Failed to initialize hotload server on port %d", hotload_port);
            }
            hotload_initialized = true;
        }
    }

    // Clean up
    ShutdownFileWatcher();
    ShutdownEditorServer();
    g_import_queue.clear();
    return 0;
}

bool InitImporter()
{
    if (g_thread_running)
    {
        LogInfo("Importer thread is already running");
        return true;
    }

    LogInfo("Starting asset importer thread...");
    g_running = true;
    g_thread_running = true;

    g_importer_thread = std::make_unique<std::thread>([](){
        int result = RunImporterLoop();
        g_thread_running = false;
        LogInfo("Importer thread stopped with code: %d", result);
    });

    return true;
}

void ShutdownImporter()
{
    if (!g_thread_running)
    {
        LogInfo("Importer thread is not running");
        return;
    }

    LogInfo("Shutting down importer thread...");
    g_running = false;

    if (g_importer_thread && g_importer_thread->joinable())
        g_importer_thread->join();

    g_importer_thread.reset();
}

bool IsImporterRunning()
{
    return g_thread_running;
}
