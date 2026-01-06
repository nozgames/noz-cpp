//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

extern void InitMeshEditor();
extern void InitTextureEditor();
extern void InitSkeletonEditor();
extern void InitAnimationEditor();
extern void InitStyles();

constexpr float SELECT_SIZE = 60.0f;
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

View g_view = {};

float GetUIScale() {
    return GetSystemDPIScale() * g_view.user_ui_scale;
}

Vec2Int GetUIRefSize() {
    Vec2Int screen_size = GetScreenSize();
    float scale = GetUIScale();
    return {
        static_cast<i32>(screen_size.x / scale),
        static_cast<i32>(screen_size.y / scale)
    };
}

inline ViewState GetState() { return g_view.state; }
static void CheckCommonShortcuts();
static void BeginSetOriginTool();
static void ToggleGrid();
static void OpenAssetContextMenu();

static void UpdateCamera() {
    float DPI = g_view.dpi * g_view.ui_scale * g_view.zoom;
    Vec2Int screen_size = GetScreenSize();
    f32 world_width = screen_size.x / DPI;
    f32 world_height = screen_size.y / ((f32)screen_size.y * DPI / (f32)screen_size.y);
    f32 half_width = world_width * 0.5f;
    f32 half_height = world_height * 0.5f;
    SetExtents(g_view.camera, -half_width, half_width, -half_height, half_height);
    Update(g_view.camera);

    g_view.zoom_ref_scale = 1.0f / g_view.zoom;
    g_view.select_size = Abs((ScreenToWorld(g_view.camera, Vec2{0, SELECT_SIZE}) - ScreenToWorld(g_view.camera, VEC2_ZERO)).y);
}

static Bounds2 GetViewBounds(AssetData* a) {
    if (a == g_editor.editing_asset && g_editor.editing_asset->vtable.editor_bounds)
        return a->vtable.editor_bounds() + a->position;

    return GetBounds(a) + a->position;
}

static void FrameSelected() {
    if (g_view.selected_asset_count == 0)
        return;

    Bounds2 bounds = {};
    bool first = true;

    if (first) {
        for (u32 i=0, c=GetAssetCount(); i<c; i++) {
            AssetData* ea = GetAssetData(i);
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
    g_view.zoom = Clamp((f32)screen_size.y / (g_view.dpi * g_view.ui_scale * target_world_height), ZOOM_MIN, ZOOM_MAX);
    g_view.zoom_version++;

    SetPosition(g_view.camera, center);
    UpdateCamera();
}

static void CommitBoxSelect(const Bounds2& bounds) {
    if (!IsShiftDown())
        ClearAssetSelection();

    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);

        if (OverlapBounds(a, bounds))
            SetSelected(a, true);
    }
}

static void UpdatePanState() {
    // Track camera position when right-click starts (for panning)
    if (WasButtonPressed(GetInputSet(), MOUSE_RIGHT)) {
        g_view.pan_position_camera = GetPosition(g_view.camera);
    }

    // Open context menu only if right-click released without significant drag
    // (drag_position is set when button pressed, so we can check distance even after drag ends)
    if (WasButtonReleased(MOUSE_RIGHT)) {
        if (Distance(g_view.mouse_position, g_view.drag_position) < DRAG_MIN) {
            OpenAssetContextMenu();
        }
    }

    // Pan camera with right-drag
    if (g_view.drag && g_view.drag_button == MOUSE_RIGHT) {
        Vec2 world_delta = ScreenToWorld(g_view.camera, g_view.drag_delta) - ScreenToWorld(g_view.camera, VEC2_ZERO);
        SetPosition(g_view.camera, g_view.pan_position_camera - world_delta);
    }
}

static void UpdateZoom() {
    float zoom_axis = GetAxis(GetInputSet(), MOUSE_SCROLL_Y);
    if (zoom_axis > -0.5f && zoom_axis < 0.5f)
        return;

    Vec2 mouse_screen = GetMousePosition();
    Vec2 world_under_cursor = ScreenToWorld(g_view.camera, mouse_screen);

    f32 zoom_factor = 1.0f + zoom_axis * ZOOM_STEP;
    g_view.zoom *= zoom_factor;
    g_view.zoom = Clamp(g_view.zoom, ZOOM_MIN, ZOOM_MAX);
    g_view.zoom_version++;

    UpdateCamera();

    Vec2 world_under_cursor_after = ScreenToWorld(g_view.camera, mouse_screen);
    Vec2 current_position = GetPosition(g_view.camera);
    Vec2 world_offset = world_under_cursor - world_under_cursor_after;
    SetPosition(g_view.camera, current_position + world_offset);
    UpdateCamera();
}

static void UpdateMoveTool(const Vec2& delta) {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (!a->selected)
            continue;

        SetPosition(a, IsCtrlDown(GetInputSet())
            ? SnapToGrid(a->saved_position + delta)
            : a->saved_position + delta);
    }
}

static void CancelMoveTool() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (!a->selected)
            continue;
        a->position = a->saved_position;
    }

    CancelUndo();
}

static void ToggleEdit() {
    if (g_view.state == VIEW_STATE_EDIT) {
        EndEdit();
        return;
    }

    if (g_view.selected_asset_count != 1)
        return;

    AssetData* a = GetFirstSelectedAsset();
    assert(a);

    if (!a->vtable.editor_begin)
        return;

    g_editor.editing_asset = a;
    a->editing = true;
    SetState(VIEW_STATE_EDIT);
    a->vtable.editor_begin(a);
}

static void UpdateDefaultState() {
    CheckShortcuts(g_view.shortcuts);

    if (WasButtonPressed(g_view.input, MOUSE_LEFT)) {
        AssetData* hit_asset = HitTestAssets(g_view.mouse_world_position);
        if (hit_asset != nullptr) {
            g_view.clear_selection_on_release = false;
            if (IsShiftDown())
                ToggleSelected(hit_asset);
            else {
                ClearAssetSelection();
                SetSelected(hit_asset, true);
            }
            return;
        }

        g_view.clear_selection_on_release = !IsShiftDown();
    }

    if (g_view.drag_started && g_editor.tool.type == TOOL_TYPE_NONE) {
        BeginBoxSelect(CommitBoxSelect);
        return;
    }

    if (WasButtonReleased(g_view.input, MOUSE_LEFT) && g_view.clear_selection_on_release) {
        ClearAssetSelection();
        return;
    }
}

void SetState(ViewState state) {
    if (state == GetState())
        return;

    switch (g_view.state) {
    case VIEW_STATE_EDIT:
        GetAssetData()->editing = false;
        g_editor.editing_asset = nullptr;
        g_view.vtable = {};
        break;

    default:
        break;
    }

    g_view.state = state;
}

static void UpdateDrag() {
    g_view.drag_delta = g_view.mouse_position - g_view.drag_position;
    g_view.drag_world_delta = g_view.mouse_world_position - g_view.drag_world_position;
    g_view.drag_started = false;
}

void EndDrag() {
    if (g_view.drag_button != INPUT_CODE_NONE)
        ConsumeButton(g_view.drag_button);
    g_view.drag = false;
    g_view.drag_started = false;
    g_view.drag_ended = true;
    g_view.drag_button = INPUT_CODE_NONE;
}

void BeginDrag(InputCode button) {
    if (!IsButtonDown(GetInputSet(), button)) {
        g_view.drag_position = g_view.mouse_position;
        g_view.drag_world_position = g_view.mouse_world_position;
    }

    UpdateDrag();

    g_view.drag = true;
    g_view.drag_started = true;
    g_view.drag_button = button;
}

static void UpdateMouse() {
    g_view.mouse_position = GetMousePosition();
    g_view.mouse_world_position = ScreenToWorld(g_view.camera, g_view.mouse_position);
    g_view.drag_ended = false;  // Clear at start of frame

    if (g_view.drag) {
        if (WasButtonReleased(GetInputSet(), g_view.drag_button))
            EndDrag();
        else
            UpdateDrag();
    } else if (WasButtonPressed(GetInputSet(), MOUSE_LEFT)) {
        g_view.drag_position = g_view.mouse_position;
        g_view.drag_world_position = g_view.mouse_world_position;
    } else if (WasButtonPressed(GetInputSet(), MOUSE_RIGHT)) {
        g_view.drag_position = g_view.mouse_position;
        g_view.drag_world_position = g_view.mouse_world_position;
    } else if (IsButtonDown(GetInputSet(), MOUSE_LEFT) && Distance(g_view.mouse_position, g_view.drag_position) >= DRAG_MIN) {
        BeginDrag(MOUSE_LEFT);
    } else if (IsButtonDown(GetInputSet(), MOUSE_RIGHT) && Distance(g_view.mouse_position, g_view.drag_position) >= DRAG_MIN) {
        BeginDrag(MOUSE_RIGHT);
    }
}


void DrawView() {
    BindCamera(g_view.camera);

    if (g_view.grid)
        DrawGrid(g_view.camera);

    Bounds2 camera_bounds = GetWorldBounds(g_view.camera);
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        a->clipped = !Intersects(camera_bounds, GetBounds(a) + a->position);
    }

    bool show_names = g_view.state == VIEW_STATE_DEFAULT && (g_view.show_names || IsAltDown(g_view.input));
    if (show_names) {
        for (u32 i=0, c=GetAssetCount(); i<c; i++) {
            AssetData* a = GetAssetData(i);
            if (a->clipped)
                continue;
            DrawBounds(a);
        }
    }

    BindColor(COLOR_WHITE);
    BindMaterial(g_view.shaded_material);
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (a->clipped)
            continue;

        if (a->editing && a->vtable.editor_draw)
            continue;

        DrawAsset(a);
    }

    if (g_view.state == VIEW_STATE_EDIT && g_editor.editing_asset)
        DrawBounds(g_editor.editing_asset, 0, COLOR_EDGE);

    if (g_editor.editing_asset && g_editor.editing_asset->vtable.editor_draw)
        g_editor.editing_asset->vtable.editor_draw();

    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (a->clipped)
            continue;

        if (!g_editor.editing_asset && a->selected) {
            DrawBounds(a, 0, COLOR_VERTEX_SELECTED);
            DrawOrigin(a);
        }
    }

    if (IsButtonDown(g_view.input, MOUSE_MIDDLE)) {
        Bounds2 bounds = GetWorldBounds(g_view.camera);
        DrawDashedLine(g_view.mouse_world_position, GetCenter(bounds));
        BindColor(COLOR_VERTEX_SELECTED);
        DrawVertex(g_view.mouse_world_position);
        DrawVertex(GetCenter(bounds));
    }

    if (g_editor.tool.type != TOOL_TYPE_NONE && g_editor.tool.vtable.draw)
        g_editor.tool.vtable.draw();
}

static void UpdateAssetNames() {
    if (GetState() != VIEW_STATE_DEFAULT &&
        GetState() != VIEW_STATE_COMMAND)
        return;

    if (!IsAltDown(g_view.input) && !g_view.show_names)
        return;

    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        if (a->clipped)
            continue;

        Bounds2 bounds = GetBounds(a);
        Vec2 p = a->position + Vec2{(bounds.min.x + bounds.max.x) * 0.5f, GetBounds(a).max.y};
        BeginCanvas({
            .type=CANVAS_TYPE_WORLD,
            .world_camera=g_view.camera,
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
        assert(g_editor.editing_asset);
        if (g_editor.editing_asset->vtable.editor_overlay)
            g_editor.editing_asset->vtable.editor_overlay();
    }

    EndCanvas();
}

void UpdateViewUI() {
    if (UpdateHelp())
        return;

    UpdateConfirmDialog();

    if (GetState() == VIEW_STATE_EDIT) {
        assert(g_editor.editing_asset);
        if (g_editor.editing_asset->vtable.editor_update)
            g_editor.editing_asset->vtable.editor_update();
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

    if (IsButtonDown(g_view.input, MOUSE_MIDDLE)) {
        Vec2 dir = Normalize(GetScreenCenter() - g_view.mouse_position);
        g_view.light_dir = Vec2{-dir.x, dir.y};
    }

    if (g_editor.tool.type != TOOL_TYPE_NONE && g_editor.tool.vtable.update)
        g_editor.tool.vtable.update();
}

void InitViewUserConfig(Props* user_config){
    SetPosition(g_view.camera, user_config->GetVec2("view", "camera_position", VEC2_ZERO));
    g_view.zoom = user_config->GetFloat("view", "camera_zoom", ZOOM_DEFAULT);
    g_view.show_names = user_config->GetBool("view", "show_names", false);
    g_view.grid = user_config->GetBool("view", "show_grid", true);
    g_view.user_ui_scale = user_config->GetFloat("view", "ui_scale", 1.0f);
    UpdateCamera();
}

void SaveViewUserConfig(Props* user_config) {
    user_config->SetVec2("view", "camera_position", GetPosition(g_view.camera));
    user_config->SetFloat("view", "camera_zoom", g_view.zoom);
    user_config->SetBool("view", "show_names", g_view.show_names);
    user_config->SetBool("view", "show_grid", g_view.grid);
    user_config->SetFloat("view", "ui_scale", g_view.user_ui_scale);
}

static void ToggleNames() {
    g_view.show_names = !g_view.show_names;
}

static void ToggleModeWireframe() {
    g_view.draw_mode = g_view.draw_mode == VIEW_DRAW_MODE_SHADED
        ? VIEW_DRAW_MODE_WIREFRAME
        : VIEW_DRAW_MODE_SHADED;
}

static void IncreaseUIScale() {
    g_view.user_ui_scale = Clamp(g_view.user_ui_scale + UI_SCALE_STEP, UI_SCALE_MIN, UI_SCALE_MAX);
}

static void DecreaseUIScale() {
    g_view.user_ui_scale = Clamp(g_view.user_ui_scale - UI_SCALE_STEP, UI_SCALE_MIN, UI_SCALE_MAX);
}

static void ResetUIScale() {
    g_view.user_ui_scale = 1.0f;
}

static void BringForward() {
    if (g_view.selected_asset_count == 0)
        return;

    BeginUndoGroup();
    for (u32 i=0, c=GetAssetCount();i<c;i++) {
        AssetData* a = GetAssetData(i);
        RecordUndo(a);
        if (!a->selected)
            continue;

        if (a->type == ASSET_TYPE_MESH) {
            MeshData* m = static_cast<MeshData*>(a);
            m->impl->depth = Clamp(m->impl->depth+1, MIN_DEPTH, MAX_DEPTH);
            MarkDirty(m);
            MarkModified(a);
        }
    }
    EndUndoGroup();
    SortAssets();
}

static void BringToFront() {
    if (g_view.selected_asset_count == 0)
        return;

    BeginUndoGroup();
    for (u32 i=0, c=GetAssetCount();i<c;i++) {
        AssetData* a = GetAssetData(i);
        RecordUndo(a);
        if (!a->selected)
            continue;

        MarkMetaModified(a);
    }
    EndUndoGroup();
    SortAssets();
}

static void SendBackward() {
    if (g_view.selected_asset_count == 0)
        return;

    BeginUndoGroup();
    for (u32 i=0, c=GetAssetCount();i<c;i++) {
        AssetData* a = GetAssetData(i);
        RecordUndo(a);
        if (!a->selected)
            continue;

        if (a->type == ASSET_TYPE_MESH) {
            MeshData* m = static_cast<MeshData*>(a);
            m->impl->depth = Clamp(m->impl->depth-1, MIN_DEPTH, MAX_DEPTH);
            MarkDirty(m);
            MarkModified(a);
        }
    }
    EndUndoGroup();
    SortAssets();
}

static void SendToBack() {
    if (g_view.selected_asset_count == 0)
        return;

    BeginUndoGroup();
    for (u32 i=0, c=GetAssetCount();i<c;i++) {
        AssetData* a = GetAssetData(i);
        RecordUndo(a);
        if (!a->selected)
            continue;

        MarkModified(a);
    }
    EndUndoGroup();
    SortAssets();
}

static void DeleteSelectedAssets() {
    if (g_view.selected_asset_count == 0)
        return;

    ShowConfirmDialog("Delete asset?", [] {
        AssetData* selected[MAX_ASSETS];
        int selected_count = GetSelectedAssets(selected, MAX_ASSETS);
        for (int i=0; i<selected_count; i++) {
            AssetData* a = selected[i];
            RemoveFromUndoRedo(a);
            DeleteAsset(a);
        }
        g_view.selected_asset_count=0;
        SortAssets();
    });
}

void EndEdit() {
    AssetData* a = GetAssetData();
    assert(a);
    if (a->vtable.editor_end)
        a->vtable.editor_end();

    SetSystemCursor(SYSTEM_CURSOR_DEFAULT);
    SetState(VIEW_STATE_DEFAULT);
    SaveAssetData();
}

void HandleUndo() { Undo(); }
void HandleRedo() { Redo(); }

static void BeginMoveTool() {
    BeginUndoGroup();
    for (u32 i=0, c=GetAssetCount(); i<c; i++)
    {
        AssetData* a = GetAssetData(i);
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
    SaveAssetData();
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
    if (g_view.selected_asset_count == 0) {
        AddNotification(NOTIFICATION_TYPE_ERROR, "NO ASSET SELECTED");
        return;
    }

    AssetData* selected[MAX_ASSETS];
    int selected_count = GetSelectedAssets(selected, MAX_ASSETS);

    ClearAssetSelection();

    for (int i=0; i<selected_count; i++) {
        AssetData* a = selected[i];
        AssetData* d = Duplicate(a);
        if (!d) {
            AddNotification(NOTIFICATION_TYPE_ERROR, "DUPLICATE FAILED");
            continue;
        }

        d->position = a->position + Vec2{0.5f, -0.5f};
        SetSelected(d, true);
    }

    SaveAssetData();
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

    for (u32 i = 0, c = GetAssetCount(); i < c; i++) {
        AssetData* a = GetAssetData(i);
        if (!a->selected)
            continue;

        RecordUndo(a);

        if (a->type == ASSET_TYPE_MESH) {
            MeshData* m = static_cast<MeshData*>(a);
            MeshFrameData* frame = GetCurrentFrame(m);

            for (int vi = 0; vi < frame->vertex_count; vi++)
                frame->vertices[vi].position = frame->vertices[vi].position * scale;

            MarkDirty(m);
            MarkModified(m);
            asset_count++;

        } else if (a->type == ASSET_TYPE_SKELETON) {
            SkeletonData* s = static_cast<SkeletonData*>(a);
            SkeletonDataImpl* impl = s->impl;

            for (int bi = 0; bi < impl->bone_count; bi++) {
                impl->bones[bi].length *= scale;
                impl->bones[bi].transform.position = impl->bones[bi].transform.position * scale;
            }

            UpdateTransforms(s);
            MarkModified(s);
            asset_count++;

        } else if (a->type == ASSET_TYPE_ANIMATION) {
            AnimationData* n = static_cast<AnimationData*>(a);
            AnimationDataImpl* impl = n->impl;

            for (int fi = 0; fi < impl->frame_count; fi++) {
                for (int bi = 0; bi < impl->bone_count; bi++) {
                    Transform& t = impl->frames[fi].transforms[bi];
                    t.position = t.position * scale;
                }
            }

            UpdateTransforms(n);
            MarkModified(n);
            asset_count++;

        }
    }

    EndUndoGroup();

    if (asset_count > 0)
        AddNotification(NOTIFICATION_TYPE_INFO, "scaled %d asset(s) by %.2f", asset_count, scale);
}

static void BeginCommandInput() {
    static CommandHandler commands[] = {
        { NAME_S, NAME_SAVE, SaveAssetsCommand },
        { NAME_N, NAME_NEW, NewAssetCommand },
        { NAME_B, NAME_BUILD, BuildAssets },
        { GetName("reimport"), GetName("reimport"), ReimportAssets },
        { GetName("scale"), GetName("scale"), ScaleCommand },
        { nullptr, nullptr, nullptr }
    };

    BeginCommandInput({.commands=commands, .prefix=":"});
}

static void HandleRenameCommand(const Command& command) {
    AssetData* a = GetFirstSelectedAsset();
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

    if (g_view.selected_asset_count != 1)
        return;

    AssetData* a = GetFirstSelectedAsset();
    BeginCommandInput({
        .commands = commands,
        .initial_text = a->name->value
    });
}

static void PlayAsset() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (!a->selected)
            continue;

        if (a->vtable.play)
            a->vtable.play(a);
    }
}

static void ToggleDebugUI() {
    ToggleDebugUI(nullptr);
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
    { KEY_S, false, true, false, SaveAssetData, "Save All" },
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
    Vec2 snapped_position = position;
    if (IsCtrlDown())
        snapped_position = SnapToGrid(position);

    BindColor(COLOR_WHITE);
    DrawVertex(snapped_position, -0.2f);
}

static void CommitSetOriginTool(const Vec2& position) {
    Vec2 snapped_position = position;
    if (IsCtrlDown())
        snapped_position = SnapToGrid(position);

    BeginUndoGroup();

    AssetData* selected[MAX_ASSETS];
    int selected_count = GetSelectedAssets(selected, MAX_ASSETS);
    for (int i=0; i<selected_count; i++) {
        AssetData* a = selected[i];
        if (a->type == ASSET_TYPE_MESH) {
            RecordUndo(a);
            SetOrigin(static_cast<MeshData*>(a), snapped_position);
            MarkModified(a);
        }
    }

    EndUndoGroup();
}

void BeginSetOriginTool() {
    AssetData* selected[MAX_ASSETS];
    int selected_count = GetSelectedAssets(selected, MAX_ASSETS);
    int viable_count = 0;
    for (int i=0; i<selected_count; i++) {
        AssetData* a = selected[i];
        if (a->type == ASSET_TYPE_MESH)
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
    g_view.grid = !g_view.grid;
}

static void NewMesh() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_MESH, nullptr, &pos); }
static void NewEvent() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_EVENT, nullptr, &pos); }
static void NewAtlas() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_ATLAS, nullptr, &pos); }
static void NewSkeleton() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_SKELETON, nullptr, &pos); }
static void NewAnimation() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_ANIMATION, nullptr, &pos); }
static void NewVfx() { Vec2 pos = GetContextMenuWorldPosition(); NewAsset(ASSET_TYPE_VFX, nullptr, &pos); }

static void OpenAssetContextMenu() {
    if (g_view.state == VIEW_STATE_EDIT) {
        assert(g_editor.editing_asset);
        if (g_editor.editing_asset->vtable.editor_context_menu)
            g_editor.editing_asset->vtable.editor_context_menu();

    } else if (g_view.state == VIEW_STATE_DEFAULT) {
        bool any_selected = g_view.selected_asset_count > 0;
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

    g_view.camera = CreateCamera(ALLOCATOR_DEFAULT);
    g_view.shaded_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_MESH);
    g_view.vertex_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
    g_view.editor_mesh_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_MESH);
    g_view.shaded_skinned_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_SKINNED_MESH);
    g_view.editor_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_EDITOR);
    g_view.zoom = ZOOM_DEFAULT;
    g_view.ui_scale = 1.0f;
    g_view.dpi = 72.0f;
    g_view.user_ui_scale = 1.0f;
    g_view.light_dir = { -1, 0 };
    g_view.draw_mode = VIEW_DRAW_MODE_SHADED;

    SetUICompositeMaterial(CreateMaterial(ALLOCATOR_DEFAULT, SHADER_POSTPROCESS_UI_COMPOSITE));

    UpdateCamera();

    g_view.input = CreateInputSet(ALLOCATOR_DEFAULT);
    EnableButton(g_view.input, KEY_LEFT_CTRL);
    EnableButton(g_view.input, KEY_LEFT_SHIFT);
    EnableButton(g_view.input, KEY_LEFT_ALT);
    EnableButton(g_view.input, KEY_RIGHT_CTRL);
    EnableButton(g_view.input, KEY_RIGHT_SHIFT);
    EnableButton(g_view.input, KEY_RIGHT_ALT);
    EnableButton(g_view.input, KEY_ESCAPE);
    EnableButton(g_view.input, MOUSE_LEFT);
    EnableButton(g_view.input, MOUSE_LEFT_DOUBLE_CLICK);
    EnableButton(g_view.input, MOUSE_RIGHT);
    EnableButton(g_view.input, MOUSE_MIDDLE);
    EnableCommonShortcuts(g_view.input);
    PushInputSet(g_view.input);

    g_view.input_tool = CreateInputSet(ALLOCATOR_DEFAULT, GetName("tool"));
    EnableModifiers(g_view.input_tool);
    EnableButton(g_view.input_tool, KEY_ESCAPE);
    EnableButton(g_view.input_tool, KEY_ENTER);
    EnableButton(g_view.input_tool, MOUSE_LEFT);
    EnableButton(g_view.input_tool, MOUSE_RIGHT);
    EnableButton(g_view.input_tool, KEY_X);
    EnableButton(g_view.input_tool, KEY_Y);

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, 1024, 1024);
    AddCircle(builder, VEC2_ZERO, 0.5f, 8, ColorUV(7,0));
    g_view.vertex_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

    Clear(builder);
    AddVertex(builder, Vec2{ 0.5f, 0.0f});
    AddVertex(builder, Vec2{ 0.0f, 0.4f});
    AddVertex(builder, Vec2{ 0.0f,-0.4f});
    AddTriangle(builder, 0, 1, 2);
    g_view.arrow_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

    Clear(builder);
    AddCircle(builder, VEC2_ZERO, 2.0f, 32, VEC2_ZERO);
    g_view.circle_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

    Clear(builder);
    AddCircleStroke(builder, VEC2_ZERO, 2.0f, 0.4f, 32, VEC2_ZERO);
    g_view.circle_stroke_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

    for (int i=0; i<=100; i++) {
        Clear(builder);
        AddArc(builder, VEC2_ZERO, 2.0f, -270, -270 + 360.0f * (i / 100.0f), 32, VEC2_ZERO);
        g_view.arc_mesh[i] = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);
    }

    Clear(builder);
    AddVertex(builder, Vec2{ -1, -1});
    AddVertex(builder, Vec2{  1, -1});
    AddVertex(builder, Vec2{  1,  1});
    AddVertex(builder, Vec2{ -1,  1});
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 2, 3, 0);
    g_view.edge_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

    Clear(builder);
    AddVertex(builder, Vec2{ -0.5f, -0.5f}, Vec2{0,1});
    AddVertex(builder, Vec2{  0.5f, -0.5f}, Vec2{1,1});
    AddVertex(builder, Vec2{  0.5f,  0.5f}, Vec2{1,0});
    AddVertex(builder, Vec2{ -0.5f,  0.5f}, Vec2{0,0});
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 2, 3, 0);
    g_view.quad_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE);

    Vec2 bone_collider_verts[] = {
        {0, 0},
        {BONE_WIDTH, -BONE_WIDTH},
        {1, 0},
        {BONE_WIDTH, BONE_WIDTH}
    };
    g_view.bone_collider = CreateCollider(ALLOCATOR_DEFAULT, bone_collider_verts, 4);

    Free(builder);

    InitGrid();
    g_view.state = VIEW_STATE_DEFAULT;

    g_view.shortcuts = g_workspace_shortcuts;
    EnableShortcuts(g_workspace_shortcuts);

    InitMeshEditor();
    InitTextureEditor();
    InitSkeletonEditor();
    InitAnimationEditor();

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

    TextureData* palette_texture_data = static_cast<TextureData*>(GetAssetData(
        ASSET_TYPE_TEXTURE,
        GetName(g_config->GetString("editor", "palette", "palette").c_str())));
    if (palette_texture_data) {
        LogInfo("[View] Loading palette texture from: %s (asset_path_index=%d)", palette_texture_data->path, palette_texture_data->asset_path_index);
        SetTexture(g_view.editor_mesh_material, TEXTURE_EDITOR_PALETTE);
        SetTexture(g_view.shaded_material, palette_texture_data->impl->texture);
        SetTexture(g_view.shaded_skinned_material, palette_texture_data->impl->texture);
        SetPaletteTexture(palette_texture_data->impl->texture);
    } else {
        LogInfo("[View] ERROR: palette texture not found!");
    }
}

void ShutdownView() {
    extern void ShutdownMeshEditor();
    extern void ShutdownSkeletonEditor();

    ShutdownMeshEditor();
    ShutdownSkeletonEditor();

    g_view = {};

    ShutdownGrid();
    ShutdownWindow();
    ShutdownUndo();
}