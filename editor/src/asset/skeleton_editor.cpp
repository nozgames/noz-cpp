//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

extern Font* FONT_SEGUISB;

struct SkeletonEditor {
    void (*state_update)();
    void (*state_draw)();
    bool clear_selection_on_up;
    bool ignore_up;
    Vec2 command_world_position;
    Vec2 selection_drag_start;
    Vec2 selection_center;
    Vec2 selection_center_world;
    Shortcut* shortcuts;
    InputSet* input;
    BoneData saved_bones[MAX_BONES];
    Mesh* editor_mesh;
};


static SkeletonEditor g_skeleton_editor = {};

inline SkeletonData* GetSkeletonData() {
    AssetData* ea = GetAssetData();
    assert(ea);
    assert(ea->type == ASSET_TYPE_SKELETON);
    return (SkeletonData*)ea;
}

static bool IsBoneSelected(int bone_index) { return GetSkeletonData()->impl->bones[bone_index].selected; }
static bool IsAncestorSelected(int bone_index) {
    SkeletonData* es = GetSkeletonData();
    SkeletonDataImpl* impl = es->impl;
    int parent_index = impl->bones[bone_index].parent_index;
    while (parent_index >= 0) {
        if (impl->bones[parent_index].selected)
            return true;
        parent_index = impl->bones[parent_index].parent_index;
    }

    return false;
}

static void SetBoneSelected(int bone_index, bool selected) {
    if (IsBoneSelected(bone_index) == selected)
        return;
    SkeletonData* es = GetSkeletonData();
    SkeletonDataImpl* impl = es->impl;
    impl->bones[bone_index].selected = selected;
    impl->selected_bone_count += selected ? 1 : -1;
}

static int GetFirstSelectedBoneIndex() {
    SkeletonData* es = GetSkeletonData();
    SkeletonDataImpl* impl = es->impl;
    for (int i=0; i<impl->bone_count; i++)
        if (IsBoneSelected(i))
            return i;
    return -1;
}

static void UpdateAllAnimationTransforms(SkeletonData* s) {
    extern void UpdateTransforms(AnimationData* n, int frame_index=-1);

    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AnimationData* a = static_cast<AnimationData*>(GetAssetData(i));
        if (a->type != ASSET_TYPE_ANIMATION)
            continue;

        if (s != a->impl->skeleton)
            continue;

        UpdateTransforms(a);
    }
}

static void UpdateAllAnimations(SkeletonData* s) {
    extern void UpdateSkeleton(AnimationData* en);

    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AnimationData* a = static_cast<AnimationData*>(GetAssetData(i));
        if (a->type != ASSET_TYPE_ANIMATION)
            continue;

        if (s != a->impl->skeleton)
            continue;

        RecordUndo(a);
        UpdateSkeleton(a);
        MarkModified(a);
    }
}

static void UpdateBoneNames() {
    if (!IsAltDown(g_skeleton_editor.input) && !g_view.show_names)
        return;

    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    for (u16 i=0; i<impl->bone_count; i++) {
        BoneData* b = impl->bones + i;
        const Mat3& transform = b->local_to_world;
        Vec2 p = (TransformPoint(Translate(s->position) * transform, Vec2{b->length * 0.5f, }));
        BeginCanvas({.type = CANVAS_TYPE_WORLD, .world_camera=g_view.camera, .world_position=p, .world_size={6,1}});
            BeginCenter();
                Label(b->name->value, {.font = FONT_SEGUISB, .font_size=12, .color=b->selected ? COLOR_VERTEX_SELECTED : COLOR_WHITE} );
            EndCenter();
        EndCanvas();
    }
}

static void UpdateSelectionCenter() {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    Vec2 center = VEC2_ZERO;
    float center_count = 0.0f;
    for (int i=0; i<impl->bone_count; i++) {
        BoneData& eb = impl->bones[i];
        if (!IsBoneSelected(i))
            continue;
        center += TransformPoint(eb.local_to_world);
        center_count += 1.0f;
    }

    g_skeleton_editor.selection_center =
        center_count < F32_EPSILON
            ? center
            : center / center_count;
    g_skeleton_editor.selection_center_world = g_skeleton_editor.selection_center + s->position;
}

static void SaveState() {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    for (int bone_index=0; bone_index<impl->bone_count; bone_index++)
        g_skeleton_editor.saved_bones[bone_index] = impl->bones[bone_index];
}

static void RevertToSavedState() {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    for (int bone_index=0; bone_index<impl->bone_count; bone_index++)
        impl->bones[bone_index] = g_skeleton_editor.saved_bones[bone_index];

    UpdateTransforms(s);
    UpdateSelectionCenter();
}

static void ClearSelection() {
    SkeletonData* es = GetSkeletonData();
    SkeletonDataImpl* impl = es->impl;
    for (int bone_index=0; bone_index<impl->bone_count; bone_index++)
        SetBoneSelected(bone_index, false);
}

static bool TrySelect() {
    SkeletonData* es = GetSkeletonData();
    SkeletonDataImpl* impl = es->impl;
    int bone_index = HitTestBone(es, g_view.mouse_world_position);
    if (bone_index == -1)
        return false;

    BoneData* eb = &impl->bones[bone_index];
    if (IsShiftDown(g_skeleton_editor.input)) {
        SetBoneSelected(bone_index, !eb->selected);
    } else {
        ClearSelection();
        SetBoneSelected(bone_index, true);
    }

    return true;
}

static void HandleBoxSelect(const Bounds2& bounds) {
    if (!IsShiftDown(g_skeleton_editor.input))
        ClearSelection();

    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    for (int bone_index=0; bone_index<impl->bone_count; bone_index++) {
        BoneData* b = &impl->bones[bone_index];
        Mat3 collider_transform =
            Translate(s->position) *
            b->local_to_world *
            Rotate(b->transform.rotation) *
            Scale(b->length);
        if (OverlapBounds(g_view.bone_collider, collider_transform, bounds))
            SetBoneSelected(bone_index, true);
    }
}

static void UpdateDefaultState() {
    if (!IsToolActive() && g_view.drag_started) {
        BeginBoxSelect(HandleBoxSelect);
        return;
    }

    if (!g_skeleton_editor.ignore_up && !g_view.drag && WasButtonReleased(g_skeleton_editor.input, MOUSE_LEFT)) {
        g_skeleton_editor.clear_selection_on_up = false;

        if (TrySelect())
            return;

        g_skeleton_editor.clear_selection_on_up = true;
    }

    g_skeleton_editor.ignore_up &= !WasButtonReleased(g_skeleton_editor.input, MOUSE_LEFT);

    if (WasButtonReleased(g_skeleton_editor.input, MOUSE_LEFT) && g_skeleton_editor.clear_selection_on_up) {
        ClearSelection();
    }
}

void UpdateSkeletonEditor() {
    CheckShortcuts(g_skeleton_editor.shortcuts, g_skeleton_editor.input);
    UpdateBoneNames();

    if (g_skeleton_editor.state_update)
        g_skeleton_editor.state_update();

    UpdateDefaultState();
}

static void BuildSkeletonEditorMesh(MeshBuilder* builder, SkeletonData* s, const Vec2& position) {
    SkeletonDataImpl* impl = s->impl;
    float line_width = STYLE_SKELETON_BONE_WIDTH * g_view.zoom_ref_scale;
    float origin_size = STYLE_SKELETON_BONE_RADIUS * g_view.zoom_ref_scale;
    float dash_length = STYLE_SKELETON_PARENT_DASH * g_view.zoom_ref_scale;

    for (int bone_index = 0; bone_index < impl->bone_count; bone_index++) {
        BoneData* b = impl->bones + bone_index;
        bool selected = b->selected;
        Color bone_color = selected ? COLOR_BONE_SELECTED : STYLE_SKELETON_BONE_COLOR;

        Vec2 p0 = TransformPoint(b->local_to_world) + position;
        Vec2 p1 = TransformPoint(b->local_to_world, Vec2{b->length, 0}) + position;

        if (b->parent_index >= 0) {
            Mat3 parent_transform = GetParentLocalToWorld(s, b, b->local_to_world);
            Vec2 pp = TransformPoint(parent_transform) + position;
            AddEditorDashedLine(builder, pp, p0, line_width, dash_length, bone_color);
        }

        AddEditorBone(builder, p0, p1, line_width, bone_color);
        AddEditorCircle(builder, p0, origin_size, bone_color);
    }
}

static void DrawSkeleton() {
    AssetData* ea = GetAssetData();
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;

    BindIdentitySkeleton();
    BindColor(COLOR_WHITE);
    BindDepth(0.0);
    Mat3 local_to_world = Translate(s->position);
    for (int i = 0; i < impl->skin_count; i++) {
        MeshData* skinned_mesh = impl->skins[i].mesh;
        if (!skinned_mesh)
            continue;
        DrawMesh(skinned_mesh, local_to_world, g_view.shaded_skinned_material);
    }

    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4096, 8192);
    BuildSkeletonEditorMesh(builder, s, ea->position);

    if (!g_skeleton_editor.editor_mesh)
        g_skeleton_editor.editor_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
    else
        UpdateMeshFromBuilder(g_skeleton_editor.editor_mesh, builder);
    PopScratch();

    BindDepth(0.0f);
    BindMaterial(g_view.editor_material);
    BindTransform(MAT3_IDENTITY);
    DrawMesh(g_skeleton_editor.editor_mesh);
}

void DrawSkeletonData() {
    DrawBounds(GetSkeletonData(), 0, COLOR_BLACK);
    DrawSkeleton();

    if (g_skeleton_editor.state_draw)
        g_skeleton_editor.state_draw();
}

static void CancelSkeletonTool() {
    CancelUndo();
    RevertToSavedState();
}

static void CounterActParentTransform(SkeletonData* s, int parent_index) {
    SkeletonDataImpl* impl = s->impl;
    BoneData& parent = impl->bones[parent_index];

    for (int child_index = 0; child_index < impl->bone_count; child_index++) {
        BoneData& child = impl->bones[child_index];
        if (child.parent_index != parent_index || IsBoneSelected(child_index))
            continue;

        // Compute what the child's local transform should be to preserve its original world position
        BoneData& saved_child = g_skeleton_editor.saved_bones[child_index];
        Mat3 new_local = parent.world_to_local * saved_child.local_to_world;

        // Extract position and rotation from the new local transform
        child.transform.position = Vec2{new_local.m[6], new_local.m[7]};
        child.transform.rotation = GetRotation(new_local);
    }
}

static void UpdateMoveTool(const Vec2& delta) {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    for (int bone_index=0; bone_index<impl->bone_count; bone_index++) {
        if (!IsBoneSelected(bone_index) || IsAncestorSelected(bone_index))
            continue;

        BoneData& b = impl->bones[bone_index];
        BoneData& p = bone_index >= 0 ? impl->bones[b.parent_index] : impl->bones[0];
        BoneData& sb = g_skeleton_editor.saved_bones[bone_index];

        b.transform.position = TransformPoint(p.world_to_local, TransformPoint(sb.local_to_world) + delta);
    }

    UpdateTransforms(s);

    // Counter-act the movement on unselected children
    for (int bone_index = 0; bone_index < impl->bone_count; bone_index++) {
        if (!IsBoneSelected(bone_index))
            continue;
        CounterActParentTransform(s, bone_index);
    }

    UpdateTransforms(s);
    UpdateAllAnimationTransforms(s);
}

static void CommitMoveTool(const Vec2&) {
    MarkModified();
}

static void BeginMoveTool(bool record_undo) {
    if (GetSkeletonData()->impl->selected_bone_count <= 0)
        return;

    SaveState();
    if (record_undo)
        RecordUndo();

    SetSystemCursor(SYSTEM_CURSOR_MOVE);
    BeginMoveTool({.update=UpdateMoveTool, .commit=CommitMoveTool, .cancel=CancelSkeletonTool});
}

static void BeginMoveTool() {
    BeginMoveTool(true);
}

static void UpdateRotateTool(float angle) {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;

    for (int bone_index=0; bone_index<impl->bone_count; bone_index++) {
        if (!IsBoneSelected(bone_index))
            continue;

        BoneData& b = impl->bones[bone_index];
        BoneData& sb = g_skeleton_editor.saved_bones[bone_index];
        if (IsCtrlDown())
            b.transform.rotation = SnapAngle(sb.transform.rotation + angle);
        else
            b.transform.rotation = sb.transform.rotation + angle;
    }

    UpdateTransforms(s);

    // Counter-act the rotation on unselected children
    for (int bone_index = 0; bone_index < impl->bone_count; bone_index++) {
        if (!IsBoneSelected(bone_index))
            continue;
        CounterActParentTransform(s, bone_index);
    }

    UpdateTransforms(s);
    UpdateAllAnimationTransforms(s);
    MarkModified();
}

static void CommitRotateTool(float) {
}

static void BeginRotateTool() {
    SkeletonData* s = GetSkeletonData();
    if (s->impl->selected_bone_count <= 0)
        return;

    UpdateSelectionCenter();
    SaveState();
    RecordUndo();
    BeginRotateTool({.origin=g_skeleton_editor.selection_center_world, .update=UpdateRotateTool, .commit=CommitRotateTool, .cancel=CancelSkeletonTool});
}

static void UpdateScaleTool(const Vec2& scale) {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    for (i32 bone_index=0; bone_index<impl->bone_count; bone_index++) {
        if (!IsBoneSelected(bone_index))
            continue;

        BoneData& b = impl->bones[bone_index];
        BoneData& sb = g_skeleton_editor.saved_bones[bone_index];
        b.length = Clamp(sb.length * scale.x, 0.05f, 10.0f);
    }

    UpdateTransforms(s);
    UpdateAllAnimationTransforms(s);
}

static void CommitScaleTool(const Vec2&) {
    MarkModified();
}

static void BeginScaleTool() {
    SkeletonData* s = GetSkeletonData();
    if (s->impl->selected_bone_count <= 0)
        return;

    UpdateSelectionCenter();
    SaveState();
    RecordUndo();
    BeginScaleTool({.origin=g_skeleton_editor.selection_center_world, .update=UpdateScaleTool, .commit=CommitScaleTool, .cancel=CancelSkeletonTool});
}

static void HandleRemove() {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    if (impl->selected_bone_count <= 0)
        return;

    BeginUndoGroup();
    RecordUndo();

    for (int i=impl->bone_count - 1; i >=0; i--) {
        if (!IsBoneSelected(i))
            continue;

        RemoveBone(s, i);
    }

    UpdateAllAnimations(s);
    EndUndoGroup();
    ClearSelection();
    MarkModified();
}

static void CommitParentTool(const Vec2& position) {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    int bone_index = HitTestBone(s, position);
    if (bone_index != -1) {
        BeginUndoGroup();
        RecordUndo(s);
        bone_index = ReparentBone(s, GetFirstSelectedBoneIndex(), bone_index);
        ClearSelection();
        SetBoneSelected(bone_index, true);
        UpdateAllAnimations(s);
        EndUndoGroup();
        return;
    }

    AssetData* hit_asset = HitTestAssets(position);
    if (!hit_asset || hit_asset->type != ASSET_TYPE_MESH)
        return;

    RecordUndo();
    impl->skins[impl->skin_count++] = {
        .asset_name = hit_asset->name,
        .mesh = (MeshData*)hit_asset,
    };
    UpdateTransforms(s);

    MarkModified();
}

static void BeginParentTool() {
    BeginSelectTool({.commit=CommitParentTool});
}

static void CommitUnparentTool(const Vec2& position) {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    for (int i=0; i<impl->skin_count; i++) {
        Skin& sm = impl->skins[i];
        if (!sm.mesh || !OverlapPoint(sm.mesh, s->position, position))
            continue;

        RecordUndo(s);
        for (int j=i; j<impl->skin_count-1; j++)
            impl->skins[j] = impl->skins[j+1];

        impl->skin_count--;

        MarkModified();
        return;
    }
}

static void BeginUnparentTool() {
    BeginSelectTool({.commit=CommitUnparentTool});
}

static void BeginExtrudeTool() {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    if (impl->selected_bone_count != 1)
        return;

    if (impl->bone_count >= MAX_BONES)
        return;

    int parent_bone_index = GetFirstSelectedBoneIndex();
    assert(parent_bone_index != -1);

    BoneData& parent_bone = impl->bones[parent_bone_index];

    RecordUndo();

    impl->bones[impl->bone_count] = {
        .name = GetUniqueBoneName(s),
        .index = impl->bone_count,
        .parent_index = parent_bone_index,
        .transform = { .scale = VEC2_ONE },
        .length = parent_bone.length
    };
    impl->bone_count++;

    UpdateTransforms(s);
    ClearSelection();
    SetBoneSelected(impl->bone_count-1, true);
    BeginMoveTool(false);
}

static void RenameBoneCommand(const Command& command) {
    if (command.arg_count != 0)
        return;

    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    if (impl->selected_bone_count != 1) {
        LogError("can only rename a single selected bone");
        return;
    }

    MarkModified();
    BeginUndoGroup();
    RecordUndo();
    impl->bones[GetFirstSelectedBoneIndex()].name = command.name;
    UpdateAllAnimations(s);
    EndUndoGroup();
}

static void BeginRenameCommand() {
    static CommandHandler commands[] = {
        {NAME_NONE, NAME_NONE, RenameBoneCommand},
        {nullptr, nullptr, nullptr}
    };

    int bone_index = GetFirstSelectedBoneIndex();
    if (bone_index == -1)
        return;

    BeginCommandInput({
        .commands = commands,
        .initial_text = GetSkeletonData()->impl->bones[bone_index].name->value
    });
}

static void BeginSkeletonEditor(AssetData*) {
    PushInputSet(g_skeleton_editor.input);
    ClearSelection();
}

static void EndSkeletonEditor() {
    PopInputSet();
}

static void ResetRotation() {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    RecordUndo(s);
    for (int bone_index=1; bone_index<impl->bone_count; bone_index++) {
        if (!IsBoneSelected(bone_index))
            continue;

        BoneData& b = impl->bones[bone_index];
        b.transform.rotation = 0;
    }

    UpdateTransforms(s);
    MarkModified(s);
}

static void ResetTranslation() {
    SkeletonData* s = GetSkeletonData();
    SkeletonDataImpl* impl = s->impl;
    RecordUndo(s);

    if (IsBoneSelected(0)) {
        BoneData& bone = impl->bones[0];
        bone.transform.position = VEC2_ZERO;
    }

    for (int bone_index=1; bone_index<impl->bone_count; bone_index++) {
        if (!IsBoneSelected(bone_index))
            continue;

        BoneData& b = impl->bones[bone_index];
        BoneData& p = impl->bones[b.parent_index];
        b.transform.position = Vec2{p.length, 0};
    }

    UpdateTransforms(s);
    MarkModified(s);
}

void InitSkeletonEditor(SkeletonData* s) {
    s->vtable.editor_begin = BeginSkeletonEditor;
    s->vtable.editor_end = EndSkeletonEditor;
    s->vtable.editor_draw = DrawSkeletonData;
    s->vtable.editor_update = UpdateSkeletonEditor;
}

void InitSkeletonEditor() {
    static Shortcut shortcuts[] = {
        { KEY_G, false, false, false, BeginMoveTool },
        { KEY_P, false, false, false, BeginParentTool },
        { KEY_P, false, true, false, BeginUnparentTool },
        { KEY_E, false, true, false, BeginExtrudeTool },
        { KEY_R, false, false, false, BeginRotateTool },
        { KEY_X, false, false, false, HandleRemove },
        { KEY_S, false, false, false, BeginScaleTool },
        { KEY_F2, false, false, false, BeginRenameCommand },
        { KEY_R, true, false, false, ResetRotation },
        { KEY_G, true, false, false, ResetTranslation },
        { INPUT_CODE_NONE }
    };

    g_skeleton_editor.input = CreateInputSet(ALLOCATOR_DEFAULT);
    EnableButton(g_skeleton_editor.input, MOUSE_LEFT);
    EnableButton(g_skeleton_editor.input, KEY_LEFT_SHIFT);
    EnableButton(g_skeleton_editor.input, KEY_RIGHT_SHIFT);
    EnableButton(g_skeleton_editor.input, MOUSE_SCROLL_Y);

    g_skeleton_editor.shortcuts = shortcuts;
    EnableShortcuts(g_skeleton_editor.shortcuts, g_skeleton_editor.input);
    EnableCommonShortcuts(g_skeleton_editor.input);
}

void ShutdownSkeletonEditor() {
    if (g_skeleton_editor.editor_mesh)
        Free(g_skeleton_editor.editor_mesh);
    g_skeleton_editor.editor_mesh = nullptr;
}
