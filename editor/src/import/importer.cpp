//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <utils/file_watcher.h>
#include <noz/task.h>
#include "asset_manifest.h"
#include "../asset_registry.h"

static void ExecuteImportJob(struct ImportJob* job);
extern AssetData* CreateAssetDataForImport(const std::filesystem::path& path);

namespace fs = std::filesystem;

struct ImportJob {
    AssetData* asset;
    fs::path source_path;
    fs::path meta_path;
};

struct Importer {
    std::atomic<bool> running;
    std::atomic<bool> thread_running;
    std::unique_ptr<std::thread> thread;
    std::filesystem::path manifest_cpp_path;
    std::filesystem::path manifest_lua_path;
    std::mutex mutex;
    std::vector<noz::Task> tasks;
    std::vector<ImportEvent> import_events;
    noz::Task post_import_task;
};

static Importer g_importer = {};

static const AssetImporter* FindImporter(const fs::path& ext) {
    const EditorAssetTypeInfo* info = FindEditorAssetTypeByExtension(ext.string().c_str());
    if (info)
        return &info->importer;
    return nullptr;
}

bool InitImporter(AssetData* a) {
    fs::path path = a->path.value;
    if (!fs::exists(path))
        return false;

    const AssetImporter* importer = FindImporter(path.extension());
    if (!importer)
        return false;

    a->type = importer->type;
    return true;
}

static void QueueImport(AssetData* a, bool force = false) {
    fs::path path = a->path.value;
    if (!fs::exists(path))
        return;

    const EditorAssetTypeInfo* type_info = GetEditorAssetTypeInfo(a->type);
    if (!type_info)
        return;

    fs::path target_path = GetTargetPath(a);
    fs::path source_meta_path = path;
    source_meta_path += ".meta";

    if (!force) {
        bool target_exists = fs::exists(target_path);
        bool meta_changed = !target_exists || (fs::exists(source_meta_path) && CompareModifiedTime(source_meta_path, target_path) > 0);
        bool source_changed = !target_exists || CompareModifiedTime(path, target_path) > 0;
        bool config_changed = !target_exists || CompareModifiedTime(g_editor.config_timestamp, fs::last_write_time(target_path)) > 0;

        if (!meta_changed && !source_changed && !config_changed)
            return;
    }

    ImportJob* job = new ImportJob{
        .asset = a,
        .source_path = fs::path(path).make_preferred(),
        .meta_path = source_meta_path.make_preferred()
    };

    std::lock_guard lock(g_importer.mutex);
    g_importer.tasks.push_back(noz::CreateTask({
        .run = [job](noz::Task) -> void* {
            ExecuteImportJob(job);
            return noz::TASK_NO_RESULT;
        },
        .name = "import_asset"
    }));
}

void QueueImport(const fs::path& path) {
    const AssetImporter* importer = FindImporter(path.extension());
    if (!importer)
        return;

    const Name* asset_name = MakeCanonicalAssetName(fs::path(path));
    if (!asset_name)
        return;

    AssetData* a = GetAssetData(importer->type, asset_name);
    if (!a) {
        a = CreateAssetDataForImport(path);
        if (!a) return;
    }

    QueueImport(a);
}

static void HandleFileChangeEvent(const FileChangeEvent& event) {
    if (event.type == FILE_CHANGE_TYPE_DELETED)
        return;

    // Skip Luau definition files
    std::string filename = event.path.filename().string();
    if (filename.ends_with(".d.luau") || filename.ends_with(".d.lua"))
        return;

    fs::path source_ext = event.path.extension();
    if (source_ext == ".meta") {
        fs::path target_path = event.path;
        target_path.replace_extension("");
        QueueImport(target_path);
    } else {
        QueueImport(event.path);
    }
}

static void ExecuteImportJob(ImportJob* job) {
    std::unique_ptr<ImportJob> job_guard(job);

    if (!fs::exists(job->source_path))
        return;

    Props* meta = nullptr;
    std::filesystem::path meta_path = job->source_path;
    meta_path += ".meta";
    if (auto meta_stream = LoadStream(nullptr, meta_path)) {
        meta = Props::Load(meta_stream);
        Free(meta_stream);
    }

    if (!meta)
        meta = new Props();

    std::unique_ptr<Props> meta_guard(meta);

    const EditorAssetTypeInfo* type_info = GetEditorAssetTypeInfo(job->asset->type);
    assert(type_info);

    fs::path target_dir =
        fs::path(g_editor.output_path) /
        ToString(job->asset->type) /
        job->asset->name->value;

    std::string target_dir_lower = target_dir.string();
    Lower(target_dir_lower.data(), (u32)target_dir_lower.size());

    try {
        type_info->importer.import_func(job->asset, target_dir_lower, g_config, meta);
    } catch (const std::exception& e) {
        AddNotification(NOTIFICATION_TYPE_ERROR, "Failed to import asset '%s': %s", job->asset->name->value, e.what());
        return;
    }

    std::lock_guard lock(g_importer.mutex);
    g_importer.import_events.push_back({
        .name =  job->asset->name,
        .type = job->asset->type
    });
}

static void CleanupOrphanedAssets() {
    std::set<fs::path> source_paths;
    for (u32 i=0, c=GetAssetCount(); i<c; i++)
        source_paths.insert(GetTargetPath(GetAssetData(i)));

    // todo: this was deleting the glsl files, etc
#if 0
    std::vector<fs::path> target_paths;
    GetFilesInDirectory(g_editor.output_path, target_paths);

    for (const fs::path& target_path : target_paths) {
        if (!source_paths.contains(target_path)) {
            fs::remove(target_path);
            AddNotification(NOTIFICATION_TYPE_INFO, "Removed '%s'", target_path.filename().string().c_str());
        }
    }
#endif
}

static bool UpdateTasks() {
    std::lock_guard lock(g_importer.mutex);
    int old_task_count = (int)g_importer.tasks.size();
    if (!noz::IsComplete(g_importer.post_import_task))
        return true;

    if (old_task_count == 0)
        return false;

    for (int i=0; i<(int)g_importer.tasks.size(); ) {
        if (noz::IsComplete(g_importer.tasks[i]))
            g_importer.tasks.erase(g_importer.tasks.begin() + i);
        else
            i++;
    }

    int new_task_count = (int)g_importer.tasks.size();
    if (new_task_count > 0)
        return true;

    g_importer.post_import_task = noz::CreateTask({
        .run = [](noz::Task) -> void* {
            GenerateAssetManifest(g_editor.output_path, g_config);
            return noz::TASK_NO_RESULT;
        },
        .name = "post_import"
    });
    return true;
}

void WaitForImportTasks() {
    while (g_importer.running && UpdateTasks()) {
        noz::WaitForAllTasks();
    }
}

static void InitialImport() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++)
        QueueImport(GetAssetData(i));

    WaitForImportTasks();
    CleanupOrphanedAssets();
}

void ReimportAll() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++)
        QueueImport(GetAssetData(i), true);  // force reimport
}

static void RunImporter() {
    if (g_editor.source_path_count == 0)
        return;

    const char* dirs[MAX_ASSET_PATHS];
    for (int p=0; p<g_editor.source_path_count; p++)
        dirs[p] = g_editor.source_paths[p].value;
    dirs[g_editor.source_path_count] = nullptr;
    InitFileWatcher(500, dirs);

    while (g_importer.running) {
        ThreadYield();

        FileChangeEvent event;
        while (g_importer.running && GetFileChangeEvent(&event))
            HandleFileChangeEvent(event);
    }

    ShutdownFileWatcher();
}

void UpdateImporter() {
    if (UpdateTasks())
        return;

    std::vector<ImportEvent> events;
    {
        std::lock_guard lock(g_importer.mutex);
        if (g_importer.import_events.empty())
            return;

        events = std::move(g_importer.import_events);
    }

    SortAssets();
    CleanupOrphanedAssets();

    for (const ImportEvent& event : events)
        Send(EDITOR_EVENT_IMPORTED, &event);
}

void InitImporter() {
    assert(!g_importer.thread_running);

    g_importer.running = true;
    g_importer.thread_running = true;

    if (g_config->HasKey("manifest", "generate_lua"))
        g_importer.manifest_lua_path = fs::weakly_canonical(fs::path(g_editor.project_path)
            / g_config->GetString("manifest", "generate_lua", "./assets/lua/global.lua"));

    if (g_config->HasKey("manifest", "generate_cpp"))
        g_importer.manifest_cpp_path = fs::weakly_canonical(fs::path(g_editor.project_path)
            / g_config->GetString("manifest", "generate_cpp", "./src/game_assets.cpp"));

    g_importer.thread = std::make_unique<std::thread>([] {
        RunImporter();
        g_importer.thread_running = false;
    });

    InitialImport();
}

void ShutdownImporter() {
    if (!g_importer.thread_running)
        return;

    g_importer.running = false;

    if (g_importer.thread && g_importer.thread->joinable())
        g_importer.thread->join();

    g_importer.thread.reset();
}

const std::filesystem::path& GetManifestCppPath() {
    return g_importer.manifest_cpp_path;
}

const std::filesystem::path& GetManifestLuaPath() {
    return g_importer.manifest_lua_path;
}
