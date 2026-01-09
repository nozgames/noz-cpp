//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    constexpr int STATE_STACK_SIZE = 16;
    constexpr int MAX_PALETTES = 64;

    struct Command;

    enum NotificationType {
        NOTIFICATION_TYPE_INFO,
        NOTIFICATION_TYPE_ERROR
    };

    enum WorkspaceState {
        VIEW_STATE_DEFAULT,
        VIEW_STATE_EDIT,
        VIEW_STATE_COMMAND,
    };

    enum ViewDrawMode {
        VIEW_DRAW_MODE_WIREFRAME,
        VIEW_DRAW_MODE_SHADED
    };

    typedef const Name* (*PreviewCommandFunc)(const Command& command);

    struct ViewVtable {
        void (*update)();
        void (*draw)();
        void (*shutdown)();
        void (*rename)(const Name* new_name);
        const Name* (*preview_command)(const Command& command);
        bool (*allow_text_input)();
    };

    struct Shortcut;
    struct Command;

    struct Workspace {
        WorkspaceState state;
        Camera* camera;
        Material* shaded_skinned_material;
        Material* shaded_material;
        Material* vertex_material;
        Material* editor_mesh_material;
        Material* editor_material;
        Mesh* vertex_mesh;
        Mesh* arrow_mesh;
        Mesh* circle_mesh;
        Mesh* circle_stroke_mesh;
        Mesh* arc_mesh[101];
        Mesh* edge_mesh;
        Mesh* quad_mesh;
        Collider* bone_collider;
        int zoom_version;
        float zoom;
        float zoom_ref_scale;
        float select_size;
        float ui_scale;
        float dpi;
        float user_ui_scale;
        InputSet* input;
        InputSet* input_tool;
        bool clear_selection_on_release;
        Vec2 pan_position_camera;

        u32 selected_asset_count;

        bool drag;
        bool drag_started;
        bool drag_ended;  // True for one frame after drag ends
        InputCode drag_button;  // Which button started the drag (MOUSE_LEFT or MOUSE_RIGHT)
        Vec2 drag_position;
        Vec2 drag_world_position;
        Vec2 drag_delta;
        Vec2 drag_world_delta;
        Vec2 mouse_position;
        Vec2 mouse_world_position;

        Vec2 light_dir;

        ViewVtable vtable;

        Shortcut* shortcuts;
        bool show_names;
        ViewDrawMode draw_mode;
        bool grid;
    };

    extern Workspace g_view;

    // @view
    extern void InitView();
    extern void UpdateView();
    extern void UpdateViewUI();
    extern void ShutdownView();
    extern void InitViewUserConfig(Props* user_config);
    extern void SaveViewUserConfig(Props* user_config);
    extern void BeginBoxSelect(void (*callback)(const Bounds2& bounds));
    extern void ClearBoxSelect();
    extern void SetState(WorkspaceState state);
    extern void HandleRename(const Name* name);
    extern void AddEditorAsset(Document* doc);
    extern void EndEdit();
    extern void BeginDrag(InputCode button = MOUSE_LEFT);
    extern void EndDrag();
    extern void EnableCommonShortcuts(InputSet* input_set);
    extern Vec2Int GetUIRefSize();
    extern float GetUIScale();

    // @grid
    extern void InitGrid();
    extern void ShutdownGrid();
    extern void DrawGrid(Camera* camera);

    // @undo
    extern void InitUndo();
    extern void ShutdownUndo();
    extern void RecordUndo(Document* a);
    extern void RecordUndo();
    extern void BeginUndoGroup();
    extern void EndUndoGroup();
    extern bool Undo();
    extern bool Redo();
    extern void CancelUndo();
    extern void RemoveFromUndoRedo(Document* a);

    // @notifications
    extern void InitNotifications();
    extern void UpdateNotifications();
    extern void AddNotification(NotificationType type, const char* format, ...);

    // @draw
    extern void DrawRect(const noz::Rect& rect);
    extern void DrawLine(const Vec2& v0, const Vec2& v1);
    extern void DrawLine(const Vec2& v0, const Vec2& v1, f32 width);
    extern void DrawVertex(const Vec2& v);
    extern void DrawVertex(const Vec2& v, f32 size);
    extern void DrawArrow(const Vec2& v, const Vec2& dir);
    extern void DrawArrow(const Vec2& v, const Vec2& dir, f32 size);
    extern void DrawOrigin(Document* a);
    extern void DrawBounds(Document* a, float expand=0, const Color& color=COLOR_BLACK);
    extern void DrawBounds(const Bounds2& bounds, const Vec2& position, const Color& color);
    extern void DrawBone(const Vec2& a, const Vec2& b);
    extern void DrawBone(const Mat3& transform, const Mat3& parent_transform, const Vec2& position, float length=BONE_DEFAULT_LENGTH);
    extern void DrawBone(const Mat3& transform, float length);
    extern void DrawDashedLine(const Vec2& v0, const Vec2& v1, f32 width, f32 length);
    extern void DrawDashedLine(const Vec2& v0, const Vec2& v1);

    // @inspector
    extern void BeginInspector();
    extern void EndInspector();
    extern void InspectorHeader(const char* title);
    extern void BeginRadioButtonGroup();
    extern int InspectorRadioButton(const char* name, int state);
    extern bool InspectorCheckbox(const char* name, bool state);
    extern void BeginInspectorGroup();
    extern void EndInspectorGroup();

    constexpr int CANVAS_ID_COMMAND = CANVAS_ID_MIN + 0;
    constexpr int CANVAS_ID_CONFIRM = CANVAS_ID_MIN + 1;
    constexpr int CANVAS_ID_OVERLAY = CANVAS_ID_MIN + 2;
    constexpr int CANVAS_ID_HELP = CANVAS_ID_MIN + 3;
    constexpr int CANVAS_ID_CONTEXT_MENU = CANVAS_ID_MIN + 4;
    constexpr int CANVAS_ID_ANIMATION_EDITOR = CANVAS_ID_MIN + 5;

    constexpr ElementId OVERLAY_BASE_ID = ELEMENT_ID_MIN + 0;
}
