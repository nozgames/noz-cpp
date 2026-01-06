//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "asset_registry.h"
#include "asset/atlas_data.h"
#include "asset/atlas_manager.h"
#include <stb_image.h>

namespace fs = std::filesystem;

// Forward declarations for palette reloading
static const Name* g_palette_texture_name;
static void LoadPaletteColors();

Editor g_editor = {};
Props* g_config = nullptr;

#if defined(NOZ_EDITOR_LIB)
EditorTraits g_editor_traits = {};
#endif

static std::thread::id g_main_thread_id;

struct LogQueue {
    std::mutex mutex;
    std::queue<std::string> queue;
};

static LogQueue& GetLogQueue() {
    static LogQueue instance;
    return instance;
}

static void HandleLog(LogType type, const char* message) {
    // Add type prefix with color for display
    std::string formatted_message;
    switch(type) {
    case LOG_TYPE_INFO:
        formatted_message = std::string(message);
        break;
    case LOG_TYPE_WARNING:
        // Nice yellow (not fully saturated) - RGB(200, 180, 0)
        formatted_message = "\033[38;2;200;180;0m[WARNING]\033[0m " + std::string(message);
        break;
    case LOG_TYPE_ERROR:
        // Nice red (not fully saturated) - RGB(200, 80, 80)
        formatted_message = "\033[38;2;200;80;80m[ERROR]\033[0m " + std::string(message);
        break;
    default:
        formatted_message = std::string(message);
        break;
    }

    if (std::this_thread::get_id() == g_main_thread_id) {
        printf("%s\n", formatted_message.c_str());
    } else {
        LogQueue& log_queue = GetLogQueue();
        std::lock_guard lock(log_queue.mutex);
        log_queue.queue.push(formatted_message);
    }
}

static void ProcessQueuedLogMessages() {
    LogQueue& log_queue = GetLogQueue();
    std::lock_guard lock(log_queue.mutex);
    while (!log_queue.queue.empty()) {
        std::string message = log_queue.queue.front();
        log_queue.queue.pop();
        printf("%s\n", message.c_str());
    }
}

extern void DrawView();

static void UpdateEditor() {
    UpdateImporter();
    ProcessQueuedLogMessages();

    Vec2Int ui_ref = GetUIRefSize();
    BeginUI(ui_ref.x, ui_ref.y);
    UpdateViewUI();
#if defined(NOZ_EDITOR_LIB)
    if (g_editor_traits.update) g_editor_traits.update();
#endif
    EndUI();

    UpdateView();

    BeginRender(STYLE_WORKSPACE_COLOR());
    DrawView();
    DrawVfx();
    DrawUI();
    DrawDebugGizmos();
    EndRender();
}

void HandleStatsEvents(EventId event_id, const void* event_data) {
    (void)event_id;
    EditorEventStats* stats = (EditorEventStats*)event_data;
    g_editor.fps = stats->fps;
    g_editor.stats_requested = false;
}

void HandleImported(EventId event_id, const void* event_data) {
    (void)event_id;

    ImportEvent* import_event = (ImportEvent*)event_data;
    AssetLoadedEvent event = { import_event->name, import_event->type };
    Send(EVENT_HOTLOAD, &event);

    // Reload palette colors if the palette texture was reimported
    if (import_event->type == ASSET_TYPE_TEXTURE && import_event->name == g_palette_texture_name) {
        LoadPaletteColors();
    }

    AddNotification(NOTIFICATION_TYPE_INFO, "imported '%s'", import_event->name->value);
}

static void SaveUserConfig(Props* user_config) {
    SaveViewUserConfig(user_config);

#if defined(NOZ_EDITOR_LIB)
    if (g_editor_traits.save_user_config) g_editor_traits.save_user_config(user_config);
#endif

    SaveProps(user_config, "./.noz/user.cfg");
}

static void SaveUserConfig() {
    Stream* config_stream = CreateStream(ALLOCATOR_DEFAULT, 8192);
    if (!config_stream)
        return;

    Props* user_config = Props::Load(config_stream);
    if (!user_config)
        user_config = new Props();

    SaveUserConfig(user_config);

    delete user_config;
    Free(config_stream);
}

static void InitUserConfig(Props* user_config) {
    InitViewUserConfig(user_config);

#if defined(NOZ_EDITOR_LIB)
    if (g_editor_traits.load_user_config) g_editor_traits.load_user_config(user_config);
#endif
}

static void InitUserConfig() {
    if (Stream* config_stream = LoadStream(nullptr, "./.noz/user.cfg")) {
        if (Props* user_config = Props::Load(config_stream)) {
            InitUserConfig(user_config);
            delete user_config;
        }

        Free(config_stream);
    }
}

static void InitConfig() {
    // Determine config path - use project path if specified, otherwise current directory
    fs::path config_path;
    const char* project_arg = GetArgValue("project");
    if (project_arg) {
        fs::path project_path = project_arg;
        if (project_path.is_relative()) {
            project_path = fs::current_path() / project_path;
            std::string temp = project_path.string();
            project_path = fs::absolute(temp);
        }
        config_path = project_path / "editor.cfg";
    } else {
        config_path = "./editor.cfg";
    }

    if (fs::exists(config_path)) {
        g_editor.config_timestamp = fs::last_write_time(config_path);
    }

    if (Stream* config_stream = LoadStream(nullptr, config_path)) {
        g_config = Props::Load(config_stream);
        Free(config_stream);
    }

    if (g_config == nullptr) {
        LogError("missing configuration '%s'", config_path.string().c_str());
        g_config = new Props();
    }

    fs::path project_path = project_arg
        ? fs::absolute(fs::path(project_arg).is_relative() ? fs::current_path() / project_arg : fs::path(project_arg))
        : fs::current_path();

    // Read in the source paths
    for (auto& path : g_config->GetKeys("source")) {
        fs::path full_path = project_path / path;
        full_path = weakly_canonical(full_path);
        if (!fs::exists(full_path))
            fs::create_directories(full_path);
        Set(g_editor.source_paths[g_editor.source_path_count], full_path.string().c_str());
        Lower(g_editor.source_paths[g_editor.source_path_count]);
        g_editor.source_path_count++;
    }

    g_editor.output_path = fs::absolute(project_path / fs::path(g_config->GetString("editor", "output_path", "assets"))).string();
    g_editor.unity_path = fs::absolute(project_path / fs::path(g_config->GetString("editor", "unity_path", "./assets/noz")));
    g_editor.save_dir = g_config->GetString("editor", "save_path", "assets");
    g_editor.project_path = project_path.string();
    g_editor.atlas_size = g_config->GetInt("editor", "atlas_size", ATLAS_DEFAULT_SIZE);

    fs::create_directories(g_editor.output_path);
}


void InitEditor() {
    g_main_thread_id = std::this_thread::get_id();
    g_editor.asset_allocator = CreatePoolAllocator(sizeof(GenericAssetData), MAX_ASSETS);

    InitEditorAssets();
    InitAtlasManager();
    InitLog(HandleLog);
    Listen(EDITOR_EVENT_STATS, HandleStatsEvents);
    Listen(EDITOR_EVENT_IMPORTED, HandleImported);
}

void ShutdownEditor() {
    SaveUserConfig();
    ShutdownCommandInput();
    ShutdownView();
    ShutdownAtlasManager();
    //ShutdownEditorServer();
    ShutdownImporter();
}

void EditorHotLoad(const Name* name, AssetType asset_type) {
    HotloadAsset(name, asset_type);
    HotloadEditorAsset( asset_type, name);
}

void BeginTool(const Tool& tool) {
    assert(g_editor.tool.type == TOOL_TYPE_NONE);
    g_editor.tool = tool;

    if (g_editor.tool.input == nullptr)
        g_editor.tool.input = g_view.input_tool;

    PushInputSet(g_editor.tool.input, tool.inherit_input);
}

void EndTool() {
    assert(g_editor.tool.type != TOOL_TYPE_NONE);
    g_editor.tool = {};
    PopInputSet();
    SetSystemCursor(SYSTEM_CURSOR_DEFAULT);
}

void CancelTool() {
    assert(g_editor.tool.type != TOOL_TYPE_NONE);
    if (g_editor.tool.vtable.cancel)
        g_editor.tool.vtable.cancel();

    EndTool();
}

static void SamplePaletteColors(u8* pixels, int width, int height) {
    constexpr int CELL_SIZE = (int)COLOR_UV_SIZE;  // 8 pixels per color cell

    for (int p = 0; p < g_editor.palette_count; p++) {
        PaletteDef& palette = g_editor.palettes[p];
        int row = palette.id;

        // Sample from center of each 8x8 cell
        int y = row * CELL_SIZE + CELL_SIZE / 2;
        if (y < 0 || y >= height)
            continue;

        for (int c = 0; c < COLOR_COUNT; c++) {
            int x = c * CELL_SIZE + CELL_SIZE / 2;
            if (x >= width)
                break;

            int pixel_index = (y * width + x) * 4;
            palette.colors[c] = Color32ToColor(
                pixels[pixel_index + 0],
                pixels[pixel_index + 1],
                pixels[pixel_index + 2],
                pixels[pixel_index + 3]
            );
        }
    }
}

static fs::path FindAssetFile(const char* name, const char* ext) {
    const char* subdirs[] = { "", "textures", "texture", "reference", nullptr };

    for (int i = 0; i < g_editor.source_path_count; i++) {
        for (int s = 0; subdirs[s] != nullptr; s++) {
            fs::path path = fs::path(g_editor.source_paths[i].value) / subdirs[s] / (std::string(name) + ext);
            if (fs::exists(path))
                return path;
        }
    }
    return {};
}

static void LoadPaletteColors() {
    if (!g_palette_texture_name)
        return;

    fs::path palette_path = FindAssetFile(g_palette_texture_name->value, ".png");
    if (palette_path.empty()) {
        LogError("Palette texture not found: %s.png", g_palette_texture_name->value);
        return;
    }

    int width, height, channels;
    u8* pixels = stbi_load(palette_path.string().c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        LogError("Failed to load palette texture: %s", palette_path.string().c_str());
        return;
    }

    SamplePaletteColors(pixels, width, height);
    stbi_image_free(pixels);
}

static void InitPalettes() {
    for (auto& palette_key : g_config->GetKeys("palettes")) {
        std::string palette_value = g_config->GetString("palettes", palette_key.c_str(), nullptr);
        Tokenizer tk;
        Init(tk, palette_value.c_str());
        int palette_id = ExpectInt(tk);
        g_editor.palette_map[palette_id] = g_editor.palette_count;

        PaletteDef& palette = g_editor.palettes[g_editor.palette_count++];
        palette.name = GetName(palette_key.c_str());
        palette.id = palette_id;

        // Initialize colors to magenta (visible default for debugging)
        for (int c = 0; c < COLOR_COUNT; c++) {
            palette.colors[c] = {1.0f, 0.0f, 1.0f, 1.0f};
        }
    }

    // Store palette texture name for reloading on reimport
    g_palette_texture_name = GetName(g_config->GetString("editor", "palette", "palette").c_str());

    LoadPaletteColors();
}

static void ResolveAssetPaths() {
    g_editor.editor_assets_path = fs::absolute(fs::current_path() / "assets").string();
    g_editor.asset_paths[g_editor.asset_path_count++] = g_editor.output_path.c_str();
    g_editor.asset_paths[g_editor.asset_path_count++] = g_editor.editor_assets_path.c_str();
    g_editor.asset_paths[g_editor.asset_path_count] = nullptr;
}

#if defined(NOZ_EDITOR_LIB)
void EditorMain(const EditorTraits& editor_traits) {
    g_editor_traits = editor_traits;
#else
void Main() {
#endif

    ApplicationTraits traits;
    Init(traits);
    traits.title = "NoZ Editor";

#if defined(NOZ_EDITOR_LIB)
    if (g_editor_traits.title) traits.title = g_editor_traits.title;
#endif

    InitConfig();
    ResolveAssetPaths();

    traits.asset_paths = g_editor.asset_paths;
    traits.load_assets = LoadAssets;
    traits.unload_assets = UnloadAssets;
    traits.hotload_asset = EditorHotLoad;
    traits.renderer.msaa_samples = 4;
    traits.scratch_memory_size = noz::MB * 128;
    traits.update = UpdateEditor;
    traits.shutdown = ShutdownEditor;

    InitApplication(&traits);
    InitPalettes();

    InitEditor();
    InitLog(HandleLog);
    InitAssetData();
    LoadAssetData();

    InitNotifications();
    InitImporter();
    InitWindow();
    PostLoadAssetData();

    MESH = g_editor.meshes;
    MESH_COUNT = MAX_ASSETS;
    FONT_DEFAULT = FONT_SEGUISB;

    InitView();
    InitCommandInput();
    InitUserConfig();
}

