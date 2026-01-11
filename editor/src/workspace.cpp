//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "document/atlas_manager.h"

namespace noz::editor {
    extern void InitMeshEditor();
    extern void InitSpriteEditor();
    extern void InitTextureEditor();
    extern void InitSkeletonEditor();
    extern void InitAnimationEditor();
    extern void InitStyles();
    extern void ShutdownSpriteEditor();

    constexpr float SELECT_SIZE = 20.0f;
    constexpr float DRAG_MIN = 5;
    constexpr float DEFAULT_DPI = 72.0f;
    constexpr float ZOOM_MIN = 0.01f;
    constexpr float ZOOM_MAX = 200.0f;
    constexpr float ZOOM_STEP = 0.1f;
    constexpr float ZOOM_DEFAULT = 1.0f;
    constexpr float VERTEX_SIZE = 0.1f;
    constexpr Color VERTEX_COLOR = { 0.95f, 0.95f, 0.95f, 1.0f};
    constexpr float FRAME_VIEW_PERCENTAGE = 1.0f / 0.75f;
    constexpr float UI_SCALE_MIN = 0.5f;
    constexpr float UI_SCALE_MAX = 3.0f;
    constexpr float UI_SCALE_STEP = 0.1f;

    Workspace g_workspace = {};

    float GetUIScale() {
        return GetSystemDPIScale() * g_workspace.user_ui_scale;
    }

    Vec2Int GetUIRefSize() {
        Vec2Int screen_size = GetScreenSize();
        float scale = GetUIScale();
        return {
            static_cast<i32>(screen_size.x / scale),
            static_cast<i32>(screen_size.y / scale)
        };
    }

    inline WorkspaceState GetState() { return g_workspace.state; }
    static void CheckCommonShortcuts();
    static void BeginSetOriginTool();
    static void ToggleGrid();
    static void OpenAssetContextMenu();

    static void UpdateCamera() {
        float DPI = g_workspace.dpi * g_workspace.ui_scale * g_workspace.zoom;
        Vec2Int screen_size = GetScreenSize();
        f32 world_width = screen_size.x / DPI;
        f32 world_height = screen_size.y / ((f32)screen_size.y * DPI / (f32)screen_size.y);
        f32 half_width = world_width * 0.5f;
        f32 half_height = world_height * 0.5f;
        SetExtents(g_workspace.camera, -half_width, half_width, -half_height, half_height);
        Update(g_workspace.camera);

        g_workspace.zoom_ref_scale = 1.0f / g_workspace.zoom;
        g_workspace.select_size = Abs((ScreenToWorld(g_workspace.camera, Vec2{0, SELECT_SIZE}) - ScreenToWorld(g_workspace.camera, VEC2_ZERO)).y);
    }

    static Bounds2 GetViewBounds(Document* doc) {
        if (doc == GetActiveDocument() && GetActiveDocument()->vtable.editor_bounds)
            return doc->vtable.editor_bounds() + doc->position;

        return GetBounds(doc) + doc->position;
    }

    static void FrameSelected() {
        if (g_workspace.selected_asset_count == 0)
            return;

        Bounds2 bounds = {};
        bool first = true;

        if (first) {
            for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
                Document* ea = GetDocument(i);
                assert(ea);
                if (!ea->selected)
                    continue;

                if (first)
                    bounds = GetViewBounds(ea);
                else
                    bounds = Union(bounds, GetViewBounds(ea));

                first = false;
            }
        }

        Vec2 center = GetCenter(bounds);
        Vec2 size = GetSize(bounds);

        f32 max_dimension = Max(size.x, size.y);
        if (max_dimension < ZOOM_MIN)
            max_dimension = ZOOM_MIN;

        Vec2Int screen_size = GetScreenSize();
        f32 target_world_height = max_dimension * FRAME_VIEW_PERCENTAGE;
        g_workspace.zoom = Clamp((f32)screen_size.y / (g_workspace.dpi * g_workspace.ui_scale * target_world_height), ZOOM_MIN, ZOOM_MAX);
        g_workspace.zoom_version++;

        SetPosition(g_workspace.camera, center);
        UpdateCamera();
    }

    static void CommitBoxSelect(const Bounds2& bounds) {
        if (!IsShiftDown())
            ClearAssetSelection();

        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);

            if (OverlapBounds(a, bounds))
                SetSelected(a, true);
        }
    }

    static void UpdatePanState() {
        // Track camera position when right-click starts (for panning)
        if (WasButtonPressed(GetInputSet(), MOUSE_RIGHT)) {
            g_workspace.pan_position_camera = GetPosition(g_workspace.camera);
        }

        // Open context menu only if right-click released without significant drag
        // (drag_position is set when button pressed, so we can check distance even after drag ends)
        if (WasButtonReleased(MOUSE_RIGHT)) {
            if (Distance(g_workspace.mouse_position, g_workspace.drag_position) < DRAG_MIN) {
                OpenAssetContextMenu();
            }
        }

        // Pan camera with right-drag
        if (g_workspace.drag && g_workspace.drag_button == MOUSE_RIGHT) {
            Vec2 world_delta = ScreenToWorld(g_workspace.camera, g_workspace.drag_delta) - ScreenToWorld(g_workspace.camera, VEC2_ZERO);
            SetPosition(g_workspace.camera, g_workspace.pan_position_camera - world_delta);
        }
    }

    static void UpdateZoom() {
        float zoom_axis = GetAxis(GetInputSet(), MOUSE_SCROLL_Y);
        if (zoom_axis > -0.5f && zoom_axis < 0.5f)
            return;

        Vec2 mouse_screen = GetMousePosition();
        Vec2 world_under_cursor = ScreenToWorld(g_workspace.camera, mouse_screen);

        f32 zoom_factor = 1.0f + zoom_axis * ZOOM_STEP;
        g_workspace.zoom *= zoom_factor;
        g_workspace.zoom = Clamp(g_workspace.zoom, ZOOM_MIN, ZOOM_MAX);
        g_workspace.zoom_version++;

        UpdateCamera();

        Vec2 world_under_cursor_after = ScreenToWorld(g_workspace.camera, mouse_screen);
        Vec2 current_position = GetPosition(g_workspace.camera);
        Vec2 world_offset = world_under_cursor - world_under_cursor_after;
        SetPosition(g_workspace.camera, current_position + world_offset);
        UpdateCamera();
    }

    static void UpdateMoveTool(const Vec2& delta) {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (!a->selected)
                continue;

            Vec2 new_pos = a->saved_position + delta;
            new_pos = IsCtrlDown(GetInputSet()) ? SnapToGrid(new_pos) : SnapToPixelGrid(new_pos);
            SetPosition(a, new_pos);
        }
    }

    static void CancelMoveTool() {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (!a->selected)
                continue;
            a->position = a->saved_position;
        }

        CancelUndo();
    }

    static void ToggleEdit() {
        if (g_workspace.state == VIEW_STATE_EDIT) {
            EndEdit();
            return;
        }

        if (g_workspace.selected_asset_count != 1)
            return;

        Document* doc = GetFirstSelectedAsset();
        assert(doc);

        if (!doc->vtable.editor_begin)
            return;

        g_editor.active_document = doc;
        doc->editing = true;
        SetState(VIEW_STATE_EDIT);
        doc->vtable.editor_begin(doc);
    }

    static void UpdateDefaultState() {
        CheckShortcuts(g_workspace.shortcuts);

        if (WasButtonPressed(g_workspace.input, MOUSE_LEFT)) {
            Document* hit_asset = HitTestAssets(g_workspace.mouse_world_position);
            if (hit_asset != nullptr) {
                g_workspace.clear_selection_on_release = false;
                if (IsShiftDown())
                    ToggleSelected(hit_asset);
                else {
                    ClearAssetSelection();
                    SetSelected(hit_asset, true);
                }
                return;
            }

            g_workspace.clear_selection_on_release = !IsShiftDown();
        }

        if (g_workspace.drag_started && g_workspace.drag_button == MOUSE_LEFT && g_editor.tool.type == TOOL_TYPE_NONE) {
            BeginBoxSelect(CommitBoxSelect);
            return;
        }

        if (WasButtonReleased(g_workspace.input, MOUSE_LEFT) && g_workspace.clear_selection_on_release) {
            ClearAssetSelection();
            return;
        }
    }

    void SetState(WorkspaceState state) {
        if (state == GetState())
            return;

        switch (g_workspace.state) {
            case VIEW_STATE_EDIT:
                GetActiveDocument()->editing = false;
                g_editor.active_document = nullptr;
                g_workspace.vtable = {};
                break;

            default:
                break;
        }

        g_workspace.state = state;
    }

    static void UpdateDrag() {
        g_workspace.drag_delta = g_workspace.mouse_position - g_workspace.drag_position;
        g_workspace.drag_world_delta = g_workspace.mouse_world_position - g_workspace.drag_world_position;
        g_workspace.drag_started = false;
    }

    void EndDrag() {
        if (g_workspace.drag_button != INPUT_CODE_NONE)
            ConsumeButton(g_workspace.drag_button);
        g_workspace.drag = false;
        g_workspace.drag_started = false;
        g_workspace.drag_ended = true;
        g_workspace.drag_button = INPUT_CODE_NONE;
    }

    void BeginDrag(InputCode button) {
        if (!IsButtonDown(GetInputSet(), button)) {
            g_workspace.drag_position = g_workspace.mouse_position;
            g_workspace.drag_world_position = g_workspace.mouse_world_position;
        }

        UpdateDrag();

        g_workspace.drag = true;
        g_workspace.drag_started = true;
        g_workspace.drag_button = button;
    }

    static void UpdateMouse() {
        g_workspace.mouse_position = GetMousePosition();
        g_workspace.mouse_world_position = ScreenToWorld(g_workspace.camera, g_workspace.mouse_position);
        g_workspace.drag_ended = false;  // Clear at start of frame

        if (g_workspace.drag) {
            if (WasButtonReleased(GetInputSet(), g_workspace.drag_button))
                EndDrag();
            else
                UpdateDrag();
        } else if (WasButtonPressed(GetInputSet(), MOUSE_LEFT)) {
            g_workspace.drag_position = g_workspace.mouse_position;
            g_workspace.drag_world_position = g_workspace.mouse_world_position;
        } else if (WasButtonPressed(GetInputSet(), MOUSE_RIGHT)) {
            g_workspace.drag_position = g_workspace.mouse_position;
            g_workspace.drag_world_position = g_workspace.mouse_world_position;
        } else if (IsButtonDown(GetInputSet(), MOUSE_LEFT) && Distance(g_workspace.mouse_position, g_workspace.drag_position) >= DRAG_MIN) {
            BeginDrag(MOUSE_LEFT);
        } else if (IsButtonDown(GetInputSet(), MOUSE_RIGHT) && Distance(g_workspace.mouse_position, g_workspace.drag_position) >= DRAG_MIN) {
            BeginDrag(MOUSE_RIGHT);
        }
    }


    void DrawView() {
        BindCamera(g_workspace.camera);

        if (g_workspace.grid)
            DrawGrid(g_workspace.camera);

        Bounds2 camera_bounds = GetWorldBounds(g_workspace.camera);
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            a->clipped = !Intersects(camera_bounds, GetBounds(a) + a->position);
        }

        bool show_names = g_workspace.state == VIEW_STATE_DEFAULT && (g_workspace.show_names || IsAltDown(g_workspace.input));
        if (show_names) {
            for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
                Document* a = GetDocument(i);
                if (a->clipped)
                    continue;
                DrawBounds(a);
            }
        }

        BindColor(COLOR_WHITE);
        BindMaterial(g_workspace.shaded_material);
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (a->clipped)
                continue;

            if (a->editing && a->vtable.editor_draw)
                continue;

            DrawAsset(a);
        }

        if (g_workspace.state == VIEW_STATE_EDIT && g_editor.active_document)
            DrawBounds(g_editor.active_document, 0, COLOR_EDGE);

        if (g_editor.active_document && g_editor.active_document->vtable.editor_draw)
            g_editor.active_document->vtable.editor_draw();

        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (a->clipped)
                continue;

            if (!g_editor.active_document && a->selected) {
                DrawBounds(a, 0, COLOR_VERTEX_SELECTED);
                DrawOrigin(a);
            }
        }

        if (IsButtonDown(g_workspace.input, MOUSE_MIDDLE)) {
            Bounds2 bounds = GetWorldBounds(g_workspace.camera);
            DrawDashedLine(g_workspace.mouse_world_position, GetCenter(bounds));
            BindColor(COLOR_VERTEX_SELECTED);
            DrawVertex(g_workspace.mouse_world_position);
            DrawVertex(GetCenter(bounds));
        }

        if (g_editor.tool.type != TOOL_TYPE_NONE && g_editor.tool.vtable.draw)
            g_editor.tool.vtable.draw();
    }

    static void UpdateAssetNames() {
        if (GetState() != VIEW_STATE_DEFAULT &&
            GetState() != VIEW_STATE_COMMAND)
            return;

        if (!IsAltDown(g_workspace.input) && !g_workspace.show_names)
            return;

        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            if (a->clipped)
                continue;

            Bounds2 bounds = GetBounds(a);
            Vec2 p = a->position + Vec2{(bounds.min.x + bounds.max.x) * 0.5f, GetBounds(a).max.y};
            BeginCanvas({
                .type=CANVAS_TYPE_WORLD,
                .world_camera=g_workspace.camera,
                .world_position=p,
                .world_size={6,1}
            });
            BeginContainer({.align=ALIGN_CENTER, .margin=EdgeInsetsTop(16)});
            Label(a->name->value, {
                .font = FONT_SEGUISB,
                .font_size=12,
                .color=a->selected ? COLOR_VERTEX_SELECTED : COLOR_WHITE
            });
            EndContainer();
            EndCanvas();
        }
    }

    static void ViewOverlay() {
        BeginCanvas({.id=CANVAS_ID_OVERLAY});

        if (GetState() == VIEW_STATE_EDIT) {
            assert(g_editor.active_document);
            if (g_editor.active_document->vtable.editor_overlay)
                g_editor.active_document->vtable.editor_overlay();
        }

        EndCanvas();
    }

    void UpdateViewUI() {
        if (UpdateHelp())
            return;

        UpdateConfirmDialog();

        if (GetState() == VIEW_STATE_EDIT) {
            assert(g_editor.active_document);
            if (g_editor.active_document->vtable.editor_update)
                g_editor.active_document->vtable.editor_update();
        }

        ViewOverlay();
        UpdateAssetNames();
        UpdateCommandInput();
        UpdateNotifications();

        UpdateContextMenu();
    }

    void UpdateView() {
        CheckCommonShortcuts();
        UpdateCamera();
        UpdateMouse();
        UpdatePanState();
        UpdateZoom();

        if (GetState() == VIEW_STATE_DEFAULT)
            UpdateDefaultState();

        if (IsButtonDown(g_workspace.input, MOUSE_MIDDLE)) {
            Vec2 dir = Normalize(GetScreenCenter() - g_workspace.mouse_position);
            g_workspace.light_dir = Vec2{-dir.x, dir.y};
        }

        if (g_editor.tool.type != TOOL_TYPE_NONE && g_editor.tool.vtable.update)
            g_editor.tool.vtable.update();
    }

    void InitViewUserConfig(Props* user_config){
        SetPosition(g_workspace.camera, user_config->GetVec2("view", "camera_position", VEC2_ZERO));
        g_workspace.zoom = user_config->GetFloat("view", "camera_zoom", ZOOM_DEFAULT);
        g_workspace.show_names = user_config->GetBool("view", "show_names", false);
        g_workspace.grid = user_config->GetBool("view", "show_grid", true);
        g_workspace.user_ui_scale = user_config->GetFloat("view", "ui_scale", 1.0f);
        UpdateCamera();
    }

    void SaveViewUserConfig(Props* user_config) {
        user_config->SetVec2("view", "camera_position", GetPosition(g_workspace.camera));
        user_config->SetFloat("view", "camera_zoom", g_workspace.zoom);
        user_config->SetBool("view", "show_names", g_workspace.show_names);
        user_config->SetBool("view", "show_grid", g_workspace.grid);
        user_config->SetFloat("view", "ui_scale", g_workspace.user_ui_scale);
    }

    static void ToggleNames() {
        g_workspace.show_names = !g_workspace.show_names;
    }

    static void ToggleModeWireframe() {
        g_workspace.draw_mode = g_workspace.draw_mode == VIEW_DRAW_MODE_SHADED
            ? VIEW_DRAW_MODE_WIREFRAME
            : VIEW_DRAW_MODE_SHADED;
    }

    static void IncreaseUIScale() {
        g_workspace.user_ui_scale = Clamp(g_workspace.user_ui_scale + UI_SCALE_STEP, UI_SCALE_MIN, UI_SCALE_MAX);
    }

    static void DecreaseUIScale() {
        g_workspace.user_ui_scale = Clamp(g_workspace.user_ui_scale - UI_SCALE_STEP, UI_SCALE_MIN, UI_SCALE_MAX);
    }

    static void ResetUIScale() {
        g_workspace.user_ui_scale = 1.0f;
    }

    static void BringForward() {
        if (g_workspace.selected_asset_count == 0)
            return;

        BeginUndoGroup();
        for (u32 i=0, c=GetDocumentCount();i<c;i++) {
            Document* a = GetDocument(i);
            RecordUndo(a);
            if (!a->selected)
                continue;

            if (a->def->type == ASSET_TYPE_MESH) {
                MeshDocument* mdoc = static_cast<MeshDocument*>(a);
                mdoc->depth = Clamp(mdoc->depth+1, MESH_MIN_DEPTH, MESH_MAX_DEPTH);
                MarkDirty(mdoc);
                MarkModified(a);
            }
        }
        EndUndoGroup();
    }

    static void BringToFront() {
        if (g_workspace.selected_asset_count == 0)
            return;

        BeginUndoGroup();
        for (u32 i=0, c=GetDocumentCount();i<c;i++) {
            Document* a = GetDocument(i);
            RecordUndo(a);
            if (!a->selected)
                continue;

            MarkMetaModified(a);
        }
        EndUndoGroup();
    }

    static void SendBackward() {
        if (g_workspace.selected_asset_count == 0)
            return;

        BeginUndoGroup();
        for (u32 i=0, c=GetDocumentCount();i<c;i++) {
            Document* a = GetDocument(i);
            RecordUndo(a);
            if (!a->selected)
                continue;

            if (a->def->type == ASSET_TYPE_MESH) {
                MeshDocument* mdoc = static_cast<MeshDocument*>(a);
                mdoc->depth = Clamp(mdoc->depth-1, MESH_MIN_DEPTH, MESH_MAX_DEPTH);
                MarkDirty(mdoc);
                MarkModified(a);
            }
        }
        EndUndoGroup();
    }

    static void SendToBack() {
        if (g_workspace.selected_asset_count == 0)
            return;

        BeginUndoGroup();
        for (u32 i=0, c=GetDocumentCount();i<c;i++) {
            Document* a = GetDocument(i);
            RecordUndo(a);
            if (!a->selected)
                continue;

            MarkModified(a);
        }
        EndUndoGroup();
    }

    static void DeleteSelectedAssets() {
        if (g_workspace.selected_asset_count == 0)
            return;

        ShowConfirmDialog("Delete asset?", [] {
            Document* selected[EDITOR_MAX_DOCUMENTS];
            int selected_count = GetSelectedAssets(selected, EDITOR_MAX_DOCUMENTS);
            for (int i=0; i<selected_count; i++) {
                Document* a = selected[i];
                RemoveFromUndoRedo(a);
                DeleteAsset(a);
            }
            g_workspace.selected_asset_count=0;
        });
    }

    void EndEdit() {
        Document* a = GetActiveDocument();
        assert(a);
        if (a->vtable.editor_end)
            a->vtable.editor_end();

        SetSystemCursor(SYSTEM_CURSOR_DEFAULT);
        SetState(VIEW_STATE_DEFAULT);
        SaveDocuments();
    }

    void HandleUndo() { Undo(); }
    void HandleRedo() { Redo(); }

    static void BeginMoveTool() {
        BeginUndoGroup();
        for (u32 i=0, c=GetDocumentCount(); i<c; i++)
        {
            Document* a = GetDocument(i);
            assert(a);
            if (!a->selected)
                continue;
            RecordUndo(a);
            a->saved_position = a->position;
        }
        EndUndoGroup();
        BeginMoveTool({.update=UpdateMoveTool, .cancel=CancelMoveTool});
    }

    // @command
    static void SaveAssetsCommand(const Command& command) {
        (void)command;
        SaveDocuments();
    }

    static void NewAssetCommand(const Command& command) {
        if (command.arg_count < 1) {
            LogError("missing asset type (mesh, etc)");
            return;
        }

        const Name* type = GetName(command.args[0]);
        if (command.arg_count < 2) {
            LogError("missing asset name");
            return;
        }

        const Name* asset_name = GetName(command.args[1]);

        AssetType asset_type = ASSET_TYPE_UNKNOWN;
        if (type == NAME_MESH || type == NAME_M)
            asset_type = ASSET_TYPE_MESH;
        else if (type == NAME_SKELETON || type == NAME_S)
            asset_type = ASSET_TYPE_SKELETON;
        else if (type == NAME_ANIMATION || type == NAME_A)
            asset_type = ASSET_TYPE_ANIMATION;
        else if (type == NAME_VFX)
            asset_type = ASSET_TYPE_VFX;
        else if (type == NAME_EVENT || type == NAME_E)
            asset_type = ASSET_TYPE_EVENT;
        else if (type == NAME_ATLAS)
            asset_type = ASSET_TYPE_ATLAS;

        NewAsset(asset_type, asset_name);
    }

    static void DuplicateAsset() {
        if (g_workspace.selected_asset_count == 0) {
            AddNotification(NOTIFICATION_TYPE_ERROR, "NO ASSET SELECTED");
            return;
        }

        Document* selected[EDITOR_MAX_DOCUMENTS];
        int selected_count = GetSelectedAssets(selected, EDITOR_MAX_DOCUMENTS);

        ClearAssetSelection();

        for (int i=0; i<selected_count; i++) {
            Document* a = selected[i];
            Document* d = Duplicate(a);
            if (!d) {
                AddNotification(NOTIFICATION_TYPE_ERROR, "DUPLICATE FAILED");
                continue;
            }

            d->position = a->position + Vec2{0.5f, -0.5f};
            SetSelected(d, true);
        }

        SaveDocuments();
        BeginMoveTool();
    }

    static void BuildAssets(const Command& command) {
        (void)command;
        Build();
    }

    static void ReimportAssets(const Command& command) {
        (void)command;
        ReimportAll();
        AddNotification(NOTIFICATION_TYPE_INFO, "queued all assets for reimport");
    }

    static void ScaleCommand(const Command& command) {
        if (command.arg_count < 1) {
            AddNotification(NOTIFICATION_TYPE_ERROR, "usage: scale <value>");
            return;
        }

        float scale = static_cast<float>(atof(command.args[0]));
        if (scale <= 0.0f) {
            AddNotification(NOTIFICATION_TYPE_ERROR, "scale must be positive");
            return;
        }

        int asset_count = 0;

        BeginUndoGroup();

        for (u32 i = 0, c = GetDocumentCount(); i < c; i++) {
            Document* doc = GetDocument(i);
            if (!doc->selected)
                continue;

            RecordUndo(doc);

            if (doc->def->type == ASSET_TYPE_MESH) {
                MeshDocument* m = static_cast<MeshDocument*>(doc);
                MeshFrameData* frame = GetCurrentFrame(m);

                for (u16 vi = 0; vi < frame->geom.vert_count; vi++)
                    frame->geom.verts[vi].position = frame->geom.verts[vi].position * scale;

                MarkDirty(m);
                MarkModified(m);
                asset_count++;

            } else if (doc->def->type == ASSET_TYPE_SKELETON) {
                SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
                for (int bi = 0; bi < sdoc->bone_count; bi++) {
                    sdoc->bones[bi].length *= scale;
                    sdoc->bones[bi].transform.position = sdoc->bones[bi].transform.position * scale;
                }

                UpdateTransforms(sdoc);
                MarkModified(sdoc);
                asset_count++;

            } else if (doc->def->type == ASSET_TYPE_ANIMATION) {
                AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);

                for (int fi = 0; fi < adoc->frame_count; fi++) {
                    for (int bi = 0; bi < adoc->bone_count; bi++) {
                        Transform& t = adoc->frames[fi].transforms[bi];
                        t.position = t.position * scale;
                    }
                }

                UpdateTransforms(adoc);
                MarkModified(adoc);
                asset_count++;

            }
        }

        EndUndoGroup();

        if (asset_count > 0)
            AddNotification(NOTIFICATION_TYPE_INFO, "scaled %d asset(s) by %.2f", asset_count, scale);
    }

    static void RebuildAtlasesCommand(const Command& cmd) {
        (void)cmd;
        RebuildAllAtlases();
        SaveDocuments();  // Save atlas files with new rect data before reimport
        ReimportAll();    // Re-import all assets to pick up atlas changes
        AddNotification(NOTIFICATION_TYPE_INFO, "Atlases rebuilt and optimized");
    }

    static void BeginCommandInput() {
        static CommandHandler commands[] = {
            { NAME_S, NAME_SAVE, SaveAssetsCommand },
            { NAME_N, NAME_NEW, NewAssetCommand },
            { NAME_B, NAME_BUILD, BuildAssets },
            { GetName("reimport"), GetName("reimport"), ReimportAssets },
            { GetName("scale"), GetName("scale"), ScaleCommand },
            { GetName("reatlas"), GetName("reatlas"), RebuildAtlasesCommand },
            { nullptr, nullptr, nullptr }
        };

        BeginCommandInput({.commands=commands, .prefix=":"});
    }

    static void HandleRenameCommand(const Command& command) {
        Document* a = GetFirstSelectedAsset();
        if (!a)
            return;

        if (!command.name) {
            AddNotification(NOTIFICATION_TYPE_ERROR, "invalid name");
            return;
        }

        MarkModified(a);
        RecordUndo(a);
        if (!Rename(a, command.name))
            AddNotification(NOTIFICATION_TYPE_ERROR, "rename failed");
    }

    static void RenameAsset() {
        static CommandHandler commands[] = {
            {NAME_NONE, NAME_NONE, HandleRenameCommand},
            {nullptr, nullptr, nullptr}
        };

        if (g_workspace.selected_asset_count != 1)
            return;

        Document* a = GetFirstSelectedAsset();
        BeginCommandInput({
            .commands = commands,
            .initial_text = a->name->value
        });
    }

    static void PlayAsset() {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (!a->selected)
                continue;

            if (a->vtable.play)
                a->vtable.play(a);
        }
    }

    static void ToggleDebugUI() {
        noz::ToggleDebugUI(nullptr);
    }

    static Shortcut g_workspace_shortcuts[] = {
        { KEY_G, false, false, false, BeginMoveTool, "Move asset" },
        { KEY_X, false, false, false, DeleteSelectedAssets, "Delete asset" },
        { KEY_D, false, true, false, DuplicateAsset, "Duplicate asset" },
        { KEY_F2, false, false, false, RenameAsset, "Rename asset" },
        { KEY_O, false, false, true, BeginSetOriginTool },
        { KEY_LEFT_BRACKET, false, false, false, SendBackward },
        { KEY_RIGHT_BRACKET, false, false, false, BringForward },
        { KEY_RIGHT_BRACKET, false, true, false, BringToFront },
        { KEY_LEFT_BRACKET, false, true, false, SendToBack },
        { KEY_SEMICOLON, false, false, true, BeginCommandInput },
        { KEY_SPACE, false, false, false, PlayAsset },
        { INPUT_CODE_NONE }
    };

    // @shortcut
    static Shortcut g_general_shortcuts[] = {
        { KEY_F1, false, false, false, ToggleHelp, "Toggle this help screen" },
        { KEY_S, false, true, false, SaveDocuments, "Save All" },
        { KEY_Z, false, true, false, HandleUndo, "Undo" },
        { KEY_Y, false, true, false, HandleRedo, "Redo" },
        { KEY_TAB, false, false, false, ToggleEdit, "Enter/Exit edit mode" },
        { KEY_F, false, false, false, FrameSelected, "Frame selected" },
        { KEY_QUOTE, true, false, false, ToggleGrid, "Toggle grid" },
        { KEY_N, true, false, false, ToggleNames, "Toggle names" },
        { KEY_W, true, false, false, ToggleModeWireframe, "Toggle wireframe" },
        { KEY_EQUALS, false, true, false, IncreaseUIScale, "Increase UI scale" },
        { KEY_MINUS, false, true, false, DecreaseUIScale, "Decrease UI scale" },
        { KEY_0, false, true, false, ResetUIScale, "Reset UI scale" },
        { KEY_TILDE, false, false, false, ToggleDebugUI, nullptr },
        { INPUT_CODE_NONE }
    };


    void EnableCommonShortcuts(InputSet* input_set) {
        EnableShortcuts(g_general_shortcuts, input_set);
        EnableModifiers(input_set);
        EnableButton(input_set, MOUSE_RIGHT);
    }

    void CheckCommonShortcuts() {
        CheckShortcuts(g_general_shortcuts, GetInputSet());
    }

    void GeneralHelp() {
        HelpGroup("General", g_general_shortcuts);
    }

    void WorkspaceHelp() {
        HelpGroup("Workspace", g_workspace_shortcuts);
    }

    static void DrawSetOriginTool(const Vec2& position) {
        Vec2 snapped_position = IsCtrlDown() ? SnapToGrid(position) : SnapToPixelGrid(position);

        BindColor(COLOR_WHITE);
        DrawVertex(snapped_position, -0.2f);
    }

    static void CommitSetOriginTool(const Vec2& position) {
        Vec2 snapped_position = IsCtrlDown() ? SnapToGrid(position) : SnapToPixelGrid(position);

        BeginUndoGroup();

        Document* selected[EDITOR_MAX_DOCUMENTS];
        int selected_count = GetSelectedAssets(selected, EDITOR_MAX_DOCUMENTS);
        for (int i=0; i<selected_count; i++) {
            Document* a = selected[i];
            if (a->def->type == ASSET_TYPE_MESH) {
                RecordUndo(a);
                SetOrigin(static_cast<MeshDocument*>(a), snapped_position);
                MarkModified(a);
            }
        }

        EndUndoGroup();
    }

    void BeginSetOriginTool() {
        Document* selected[EDITOR_MAX_DOCUMENTS];
        int selected_count = GetSelectedAssets(selected, EDITOR_MAX_DOCUMENTS);
        int viable_count = 0;
        for (int i=0; i<selected_count; i++) {
            Document* a = selected[i];
            if (a->def->type == ASSET_TYPE_MESH)
                viable_count++;
        }

        if (!viable_count)
            return;

        BeginSelectTool({
            .commit = CommitSetOriginTool,
            .draw = DrawSetOriginTool
        });
    }

    static void ToggleGrid() {
        g_workspace.grid = !g_workspace.grid;
    }

    static void NewMesh() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_MESH, nullptr, &pos); }
    static void NewEvent() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_EVENT, nullptr, &pos); }
    static void NewAtlas() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_ATLAS, nullptr, &pos); }
    static void NewSkeleton() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_SKELETON, nullptr, &pos); }
    static void NewAnimation() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_ANIMATION, nullptr, &pos); }
    static void NewVfx() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_VFX, nullptr, &pos); }

    static void OpenAssetContextMenu() {
        if (g_workspace.state == VIEW_STATE_EDIT) {
            assert(g_editor.active_document);
            if (g_editor.active_document->vtable.editor_context_menu)
                g_editor.active_document->vtable.editor_context_menu();

        } else if (g_workspace.state == VIEW_STATE_DEFAULT) {
            bool any_selected = g_workspace.selected_asset_count > 0;
            OpenContextMenuAtMouse({
                .title="Asset",
                .items = {
                    { "New", nullptr, true },
                    { "Animation", NewAnimation, true, 1 },
                    { "Atlas", NewAtlas, true, 1 },
                    { "Event", NewEvent, true, 1 },
                    { "Mesh", NewMesh, true, 1 },
                    { "Skeleton", NewSkeleton, true, 1 },
                    { "Vfx", NewVfx, true, 1 },
                    { nullptr, nullptr, true },
                    { "Edit", ToggleEdit, any_selected },
                    { "Duplicate", DuplicateAsset, any_selected },
                    { "Rename", RenameAsset, any_selected },
                    { "Delete", DeleteSelectedAssets, any_selected },
                },
                .item_count = 12
            });
        }
    }

    void InitView() {
        InitUndo();
        InitStyles();

        g_workspace.camera = CreateCamera(ALLOCATOR_DEFAULT);
        g_workspace.shaded_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_MESH);
        g_workspace.vertex_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
        g_workspace.editor_mesh_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_EDITOR_MESH);
        g_workspace.shaded_skinned_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_SKINNED_MESH);
        g_workspace.editor_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_EDITOR);
        g_workspace.zoom = ZOOM_DEFAULT;
        g_workspace.ui_scale = 1.0f;
        g_workspace.dpi = 72.0f;
        g_workspace.user_ui_scale = 1.0f;
        g_workspace.light_dir = { -1, 0 };
        g_workspace.draw_mode = VIEW_DRAW_MODE_SHADED;

        SetUICompositeMaterial(CreateMaterial(ALLOCATOR_DEFAULT, SHADER_POSTPROCESS_UI_COMPOSITE));

        UpdateCamera();

        g_workspace.input = CreateInputSet(ALLOCATOR_DEFAULT);
        EnableButton(g_workspace.input, KEY_LEFT_CTRL);
        EnableButton(g_workspace.input, KEY_LEFT_SHIFT);
        EnableButton(g_workspace.input, KEY_LEFT_ALT);
        EnableButton(g_workspace.input, KEY_RIGHT_CTRL);
        EnableButton(g_workspace.input, KEY_RIGHT_SHIFT);
        EnableButton(g_workspace.input, KEY_RIGHT_ALT);
        EnableButton(g_workspace.input, KEY_ESCAPE);
        EnableButton(g_workspace.input, MOUSE_LEFT);
        EnableButton(g_workspace.input, MOUSE_LEFT_DOUBLE_CLICK);
        EnableButton(g_workspace.input, MOUSE_RIGHT);
        EnableButton(g_workspace.input, MOUSE_MIDDLE);
        EnableCommonShortcuts(g_workspace.input);
        PushInputSet(g_workspace.input);

        g_workspace.input_tool = CreateInputSet(ALLOCATOR_DEFAULT, GetName("tool"));
        EnableModifiers(g_workspace.input_tool);
        EnableButton(g_workspace.input_tool, KEY_ESCAPE);
        EnableButton(g_workspace.input_tool, KEY_ENTER);
        EnableButton(g_workspace.input_tool, MOUSE_LEFT);
        EnableButton(g_workspace.input_tool, MOUSE_RIGHT);
        EnableButton(g_workspace.input_tool, KEY_X);
        EnableButton(g_workspace.input_tool, KEY_Y);

        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, 1024, 1024);
        AddCircle(builder, VEC2_ZERO, 0.5f, 8, ColorUV(7,0));
        g_workspace.vertex_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

        Clear(builder);
        AddVertex(builder, Vec2{ 0.5f, 0.0f});
        AddVertex(builder, Vec2{ 0.0f, 0.4f});
        AddVertex(builder, Vec2{ 0.0f,-0.4f});
        AddTriangle(builder, 0, 1, 2);
        g_workspace.arrow_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

        Clear(builder);
        AddCircle(builder, VEC2_ZERO, 2.0f, 32, VEC2_ZERO);
        g_workspace.circle_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

        Clear(builder);
        AddCircleStroke(builder, VEC2_ZERO, 2.0f, 0.4f, 32, VEC2_ZERO);
        g_workspace.circle_stroke_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

        for (int i=0; i<=100; i++) {
            Clear(builder);
            AddArc(builder, VEC2_ZERO, 2.0f, -270, -270 + 360.0f * (i / 100.0f), 32, VEC2_ZERO);
            g_workspace.arc_mesh[i] = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);
        }

        Clear(builder);
        AddVertex(builder, Vec2{ -1, -1});
        AddVertex(builder, Vec2{  1, -1});
        AddVertex(builder, Vec2{  1,  1});
        AddVertex(builder, Vec2{ -1,  1});
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 2, 3, 0);
        g_workspace.edge_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

        Clear(builder);
        AddVertex(builder, Vec2{ -0.5f, -0.5f}, Vec2{0,1});
        AddVertex(builder, Vec2{  0.5f, -0.5f}, Vec2{1,1});
        AddVertex(builder, Vec2{  0.5f,  0.5f}, Vec2{1,0});
        AddVertex(builder, Vec2{ -0.5f,  0.5f}, Vec2{0,0});
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 2, 3, 0);
        g_workspace.quad_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

        Vec2 bone_collider_verts[] = {
            {0, 0},
            {BONE_WIDTH, -BONE_WIDTH},
            {1, 0},
            {BONE_WIDTH, BONE_WIDTH}
        };
        g_workspace.bone_collider = CreateCollider(ALLOCATOR_DEFAULT, bone_collider_verts, 4);

        Free(builder);

        InitGrid();
        g_workspace.state = VIEW_STATE_DEFAULT;

        g_workspace.shortcuts = g_workspace_shortcuts;
        EnableShortcuts(g_workspace_shortcuts);

        InitMeshEditor();
        InitTextureEditor();
        InitSkeletonEditor();
        InitAnimationEditor();
        InitSpriteEditor();

        NAME_MESH = GetName("mesh");
        NAME_VFX = GetName("vfx");
        NAME_BUILD = GetName("build");
        NAME_ANIMATION = GetName("animation");
        NAME_EVENT = GetName("event");
        NAME_NEW = GetName("new");
        NAME_RENAME = GetName("rename");
        NAME_E = GetName("e");
        NAME_A = GetName("a");
        NAME_N = GetName("n");
        NAME_MIRROR = GetName("mirror");
        NAME_B = GetName("b");
        NAME_EDIT = GetName("edit");
        NAME_R = GetName("r");
        NAME_M = GetName("m");
        NAME_SAVE = GetName("save");
        NAME_RU = GetName("ru");
        NAME_S = GetName("s");
        NAME_SKELETON = GetName("skeleton");

        SetTexture(g_workspace.editor_mesh_material, ATLAS_ARRAY);

        TextureDocument* palette_texture_data = static_cast<TextureDocument*>(FindDocument(
            ASSET_TYPE_TEXTURE,
            GetName(g_editor.config->GetString("editor", "palette", "palette").c_str())));
        if (palette_texture_data) {
            SetTexture(g_workspace.shaded_material, palette_texture_data->texture);
            SetTexture(g_workspace.shaded_skinned_material, palette_texture_data->texture);
            SetPaletteTexture(palette_texture_data->texture);
        } else {
            LogInfo("[View] ERROR: palette texture not found!");
        }
    }

    void ShutdownView() {
        extern void ShutdownMeshEditor();
        extern void ShutdownSkeletonEditor();

        ShutdownMeshEditor();
        ShutdownSkeletonEditor();
        ShutdownSpriteEditor();

        g_workspace = {};

        ShutdownGrid();
        ShutdownWindow();
        ShutdownUndo();
    }
}
