//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "document.h"

namespace noz::editor {
    // @constants
    constexpr int EDITOR_MAX_DOCUMENTS = 1024;
    constexpr int MAX_VIEWS = 16;
    constexpr int MAX_ASSET_PATHS = 8;
    constexpr float BONE_WIDTH = 0.10f;
    constexpr float BONE_DEFAULT_LENGTH = 0.25f;
    constexpr float BONE_ORIGIN_SIZE = 0.12f;
    constexpr float BOUNDS_PADDING = 0.02f;

    struct TextInputBox;
    struct DocumentImporter;
    struct Document;
    struct MeshDocument;
    struct Command;

#if defined(NOZ_EDITOR_LIB)
    struct EditorTraits {
        const char* title;
        void (*update)();
        void (*load_user_config)(Props* user_config);
        void (*save_user_config)(Props* user_config);
        bool skip_lua_loader;  // If true, generate .d.luau but not global.lua
    };

    extern EditorTraits g_editor_traits;
#endif

    // @tool
    struct ToolVtable {
        void (*cancel)();
        void (*update)();
        void (*draw)();
    };

    enum ToolType {
        TOOL_TYPE_NONE,
        TOOL_TYPE_BOX_SELECT,
        TOOL_TYPE_MOVE,
        TOOL_TYPE_ROTATE,
        TOOL_TYPE_SCALE,
        TOOL_TYPE_SELECT,
        TOOL_TYPE_WEIGHT,
        TOOL_TYPE_CURVE,
    };

    struct Tool {
        ToolType type = TOOL_TYPE_NONE;
        ToolVtable vtable = {};
        InputSet* input;
        bool inherit_input;
        bool hide_selected;
    };

    struct PaletteDef {
        const Name* name;
        int id;
        Color colors[COLOR_COUNT];
    };

    void BeginTool(const Tool& tool);
    void CancelTool();
    void EndTool();

    struct Editor {
        Props* config = nullptr;
        Document* documents[EDITOR_MAX_DOCUMENTS];
        int document_count;

        TextInputBox* command_input;
        TextInputBox* search_input;
        bool command_mode;
        bool search_mode;
        bool auto_quit;
        int fps;
        bool stats_requested;
        std::filesystem::file_time_type config_timestamp;
        std::string output_path;
        std::filesystem::path unity_path;

        std::string project_path;
        std::string editor_assets_path;
        const char* asset_paths[3];
        int asset_path_count;

        Text source_paths[MAX_ASSET_PATHS];
        int source_path_count;

        Document* active_document;

        Tool tool;

        Mesh* meshes[EDITOR_MAX_DOCUMENTS];
        Texture* textures[EDITOR_MAX_DOCUMENTS];

        std::filesystem::path save_dir;

        PaletteDef palettes[COLOR_PALETTE_COUNT];
        int palette_map[COLOR_PALETTE_COUNT];
        int palette_count;

        MeshBuilder* mesh_builder;

        struct {
            int size = 1024;
            int dpi = 96;
            int padding = 4;
            String32 prefix;
            TextureFilter filter = TEXTURE_FILTER_LINEAR;
            bool antialias = false;
        } atlas;
    };

    extern Editor g_editor;

    // @command
    constexpr int MAX_COMMAND_ARGS = 4;
    constexpr int MAX_COMMAND_ARG_SIZE = 128;

    struct Command {
        const Name* name;
        int arg_count;
        char args[MAX_COMMAND_ARGS][MAX_COMMAND_ARG_SIZE];
    };

    struct CommandHandler {
        const Name* short_name;
        const Name* name;
        void (*handler)(const Command&);
    };

    struct CommandInputOptions {
        const CommandHandler* commands;
        const char* prefix;
        const char* placeholder;
        const char* initial_text;
        bool hide_empty;
        InputSet* input;
    };

    extern void InitCommandInput();
    extern void ShutdownCommandInput();
    extern void BeginCommandInput(const CommandInputOptions& options);
    extern void UpdateCommandInput();
    extern void EndCommandInput();
    extern bool IsCommandInputActive();
    extern const char* GetVarTypeNameFromAssetType(AssetType asset_type);

    // @import
    struct ImportEvent {
        const Name* name;
        AssetType type;
    };

    extern void InitImporter();
    extern void ShutdownImporter();
    extern void UpdateImporter();
    extern void QueueImport(const std::filesystem::path& path);
    extern void WaitForImportTasks();
    extern void ReimportAll();

    extern const std::filesystem::path& GetManifestCppPath();
    extern const std::filesystem::path& GetManifestLuaPath();

    // @grid
    extern Vec2 SnapToPixelGrid(const Vec2& position);
    extern Vec2 SnapToGrid(const Vec2& position);
    extern float SnapAngle(float angle);

    // @shortcut
    struct Shortcut {
        InputCode button;
        bool alt;
        bool ctrl;
        bool shift;
        void (*action)();
        const char* description;
    };

    extern void EnableShortcuts(const Shortcut* shortcuts, InputSet* input_set=nullptr);
    extern void CheckShortcuts(const Shortcut* shortcuts, InputSet* input_set=nullptr);

    // @ui
    struct EditorButtonConfig {
        ElementId id;
        float width = STYLE_TOGGLE_BUTTON_HEIGHT;
        float height = STYLE_TOGGLE_BUTTON_HEIGHT;
        Mesh* icon;
        bool checked;
        bool disabled;
        void (*content_func)();
        bool (*popup_func)();
    };

    extern Color GetButtonHoverColor(ElementFlags state, float time, void* user_data);
    extern void UpdateConfirmDialog();
    extern void ShowConfirmDialog(const char* message, const std::function<void()>& callback);
    extern bool UpdateHelp();
    extern void ToggleHelp();
    extern void HelpGroup(const char* title, const Shortcut* shortcuts);
    extern bool EditorCloseButton(ElementId id);
    extern bool UpdateContextMenu();
    extern bool EditorButton(const EditorButtonConfig& config);
    extern bool EditorButton(ElementId id, Mesh* icon, bool state, bool disabled=false);
    extern void BeginOverlay(ElementId id=ELEMENT_ID_NONE, Align align=ALIGN_TOP_LEFT);
    extern void EndOverlay();

    // @context_menu
    constexpr int CONTEXT_MENU_MAX_ITEMS = 64;

    struct ContextMenuItem {
        const char* label;
        void (*action)();
        bool enabled;
        int level;
    };

    struct ContextMenuConfig {
        const char* title;
        ContextMenuItem items[CONTEXT_MENU_MAX_ITEMS];
        int item_count;
    };

    extern void OpenContextMenuAtMouse(const ContextMenuConfig& config);
    extern Vec2 GetContextMenuWorldPosition();

    // @document
    extern void LoadDocuments();
    extern void PostLoadDocuments();
    extern void SaveDocuments();
    inline int GetDocumentCount() { return g_editor.document_count; }
    inline Document* GetActiveDocument() { return g_editor.active_document; }
    extern Document* FindDocument(AssetType type, const Name* name);
    extern Document* FindDocument(const std::filesystem::path& path);
    inline bool IsToolActive() { return g_editor.tool.type != TOOL_TYPE_NONE; }
    inline void MarkModified() { MarkModified(GetActiveDocument()); }
    inline void MarkMetaModified() { MarkMetaModified(GetActiveDocument()); }

    inline Document* GetDocument(int index) {
        assert(index >= 0 && index < GetDocumentCount());
        assert(g_editor.documents[index]);
        return g_editor.documents[index];
    }

    inline bool DoesToolHideSelected() { return IsToolActive() && g_editor.tool.hide_selected; }

    // @build
    extern void Build();
}

#include "workspace.h"
#include "tool.h"

