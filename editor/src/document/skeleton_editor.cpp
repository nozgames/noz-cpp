//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

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

    extern void UpdateTransforms(AnimationDocument* adoc, int frame_index);
    extern void UpdateSkeleton(AnimationDocument* adoc);

    inline SkeletonDocument* GetSkeletonData() {
        Document* doc = GetActiveDocument();
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        return static_cast<SkeletonDocument*>(doc);
    }

    static bool IsBoneSelected(int bone_index) { return GetSkeletonData()->bones[bone_index].selected; }
    static bool IsAncestorSelected(int bone_index) {
        SkeletonDocument* sdoc = GetSkeletonData();
        int parent_index = sdoc->bones[bone_index].parent_index;
        while (parent_index >= 0) {
            if (sdoc->bones[parent_index].selected)
                return true;
            parent_index = sdoc->bones[parent_index].parent_index;
        }

        return false;
    }

    static void SetBoneSelected(int bone_index, bool selected) {
        if (IsBoneSelected(bone_index) == selected)
            return;
        SkeletonDocument* sdoc = GetSkeletonData();
        sdoc->bones[bone_index].selected = selected;
        sdoc->selected_bone_count += selected ? 1 : -1;
    }

    static int GetFirstSelectedBoneIndex() {
        SkeletonDocument* sdoc = GetSkeletonData();
        for (int i=0; i<sdoc->bone_count; i++)
            if (IsBoneSelected(i))
                return i;
        return -1;
    }

    static void UpdateAllAnimationTransforms(SkeletonDocument* sdoc) {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            AnimationDocument* adoc = static_cast<AnimationDocument*>(GetDocument(i));
            if (adoc->def->type != ASSET_TYPE_ANIMATION)
                continue;

            if (sdoc != adoc->skeleton)
                continue;

            UpdateTransforms(adoc);
        }
    }

    static void UpdateAllAnimations(SkeletonDocument* s) {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            AnimationDocument* adoc = static_cast<AnimationDocument*>(GetDocument(i));
            if (adoc->def->type != ASSET_TYPE_ANIMATION) continue;
            if (s != adoc->skeleton) continue;

            RecordUndo(adoc);
            UpdateSkeleton(adoc);
            MarkModified(adoc);
        }
    }

    static void UpdateBoneNames() {
        if (!IsAltDown(g_skeleton_editor.input) && !g_workspace.show_names)
            return;

        SkeletonDocument* sdoc = GetSkeletonData();
        for (u16 i=0; i<sdoc->bone_count; i++) {
            BoneData* b = sdoc->bones + i;
            const Mat3& transform = b->local_to_world;
            Vec2 p = (TransformPoint(Translate(sdoc->position) * transform, Vec2{b->length * 0.5f, }));
            BeginCanvas({.type = CANVAS_TYPE_WORLD, .world_camera=g_workspace.camera, .world_position=p, .world_size={6,1}});
            BeginCenter();
            Label(b->name->value, {.font = FONT_SEGUISB, .font_size=12, .color=b->selected ? COLOR_VERTEX_SELECTED : COLOR_WHITE} );
            EndCenter();
            EndCanvas();
        }
    }

    static void UpdateSelectionCenter() {
        SkeletonDocument* sdoc = GetSkeletonData();
        Vec2 center = VEC2_ZERO;
        float center_count = 0.0f;
        for (int i=0; i<sdoc->bone_count; i++) {
            BoneData& eb = sdoc->bones[i];
            if (!IsBoneSelected(i))
                continue;
            center += TransformPoint(eb.local_to_world);
            center_count += 1.0f;
        }

        g_skeleton_editor.selection_center =
            center_count < F32_EPSILON
                ? center
                : center / center_count;
        g_skeleton_editor.selection_center_world = g_skeleton_editor.selection_center + sdoc->position;
    }

    static void SaveState() {
        SkeletonDocument* sdoc = GetSkeletonData();
        for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++)
            g_skeleton_editor.saved_bones[bone_index] = sdoc->bones[bone_index];
    }

    static void RevertToSavedState() {
        SkeletonDocument* sdoc = GetSkeletonData();
        for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++)
            sdoc->bones[bone_index] = g_skeleton_editor.saved_bones[bone_index];

        UpdateTransforms(sdoc);
        UpdateSelectionCenter();
    }

    static void ClearSelection() {
        SkeletonDocument* sdoc = GetSkeletonData();
        for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++)
            SetBoneSelected(bone_index, false);
    }

    static bool TrySelect() {
        SkeletonDocument* sdoc = GetSkeletonData();
        int bone_index = HitTestBone(sdoc, g_workspace.mouse_world_position);
        if (bone_index == -1)
            return false;

        BoneData* eb = &sdoc->bones[bone_index];
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

        SkeletonDocument* sdoc = GetSkeletonData();
        for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++) {
            BoneData* b = &sdoc->bones[bone_index];
            Mat3 collider_transform =
                Translate(sdoc->position) *
                b->local_to_world *
                Rotate(b->transform.rotation) *
                Scale(b->length);
            if (OverlapBounds(g_workspace.bone_collider, collider_transform, bounds))
                SetBoneSelected(bone_index, true);
        }
    }

    static void UpdateDefaultState() {
        if (!IsToolActive() && g_workspace.drag_started) {
            BeginBoxSelect(HandleBoxSelect);
            return;
        }

        if (!g_skeleton_editor.ignore_up && !g_workspace.drag && WasButtonReleased(g_skeleton_editor.input, MOUSE_LEFT)) {
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

    static void BuildSkeletonEditorMesh(MeshBuilder* builder, SkeletonDocument* sdoc, const Vec2& position) {
        float line_width = STYLE_SKELETON_BONE_WIDTH * g_workspace.zoom_ref_scale;
        float origin_size = STYLE_SKELETON_BONE_RADIUS * g_workspace.zoom_ref_scale;
        float dash_length = STYLE_SKELETON_PARENT_DASH * g_workspace.zoom_ref_scale;

        for (int bone_index = 0; bone_index < sdoc->bone_count; bone_index++) {
            BoneData* b = sdoc->bones + bone_index;
            bool selected = b->selected;
            Color bone_color = selected ? COLOR_BONE_SELECTED : STYLE_SKELETON_BONE_COLOR;

            Vec2 p0 = TransformPoint(b->local_to_world) + position;
            Vec2 p1 = TransformPoint(b->local_to_world, Vec2{b->length, 0}) + position;

            if (b->parent_index >= 0) {
                Mat3 parent_transform = GetParentLocalToWorld(sdoc, b, b->local_to_world);
                Vec2 pp = TransformPoint(parent_transform) + position;
                AddEditorDashedLine(builder, pp, p0, line_width, dash_length, bone_color);
            }

            AddEditorBone(builder, p0, p1, line_width, bone_color);
            AddEditorCircle(builder, p0, origin_size, bone_color);
        }
    }

    static void DrawSkeleton() {
        SkeletonDocument* sdoc = GetSkeletonData();

#if 0        
        BindIdentitySkeleton();
        BindColor(COLOR_WHITE);
        BindDepth(0.0);
        Mat3 local_to_world = Translate(sdoc->position);
        for (int i = 0; i < sdoc->skin_count; i++) {
            MeshDocument* skinned_mesh = sdoc->skins[i].mesh;
            if (!skinned_mesh)
                continue;
            DrawMesh(skinned_mesh, local_to_world, g_workspace.shaded_skinned_material);
        }
#endif        

        PushScratch();
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4096, 8192);
        BuildSkeletonEditorMesh(builder, sdoc, sdoc->position);

        if (!g_skeleton_editor.editor_mesh)
            g_skeleton_editor.editor_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        else
            UpdateMesh(builder, g_skeleton_editor.editor_mesh);
        PopScratch();

        BindDepth(0.0f);
        BindMaterial(g_workspace.editor_material);
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

    static void CounterActParentTransform(SkeletonDocument* sdoc, int parent_index) {
        BoneData& parent = sdoc->bones[parent_index];

        for (int child_index = 0; child_index < sdoc->bone_count; child_index++) {
            BoneData& child = sdoc->bones[child_index];
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
        SkeletonDocument* sdoc = GetSkeletonData();
        for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++) {
            if (!IsBoneSelected(bone_index) || IsAncestorSelected(bone_index))
                continue;

            BoneData& b = sdoc->bones[bone_index];
            BoneData& p = bone_index >= 0 ? sdoc->bones[b.parent_index] : sdoc->bones[0];
            BoneData& sb = g_skeleton_editor.saved_bones[bone_index];

            b.transform.position = TransformPoint(p.world_to_local, TransformPoint(sb.local_to_world) + delta);
        }

        UpdateTransforms(sdoc);

        // Counter-act the movement on unselected children
        for (int bone_index = 0; bone_index < sdoc->bone_count; bone_index++) {
            if (!IsBoneSelected(bone_index))
                continue;
            CounterActParentTransform(sdoc, bone_index);
        }

        UpdateTransforms(sdoc);
        UpdateAllAnimationTransforms(sdoc);
    }

    static void CommitMoveTool(const Vec2&) {
        MarkModified();
    }

    static void BeginMoveTool(bool record_undo) {
        if (GetSkeletonData()->selected_bone_count <= 0)
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
        SkeletonDocument* sdoc = GetSkeletonData();

        for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++) {
            if (!IsBoneSelected(bone_index))
                continue;

            BoneData& b = sdoc->bones[bone_index];
            BoneData& sb = g_skeleton_editor.saved_bones[bone_index];
            if (IsCtrlDown())
                b.transform.rotation = SnapAngle(sb.transform.rotation + angle);
            else
                b.transform.rotation = sb.transform.rotation + angle;
        }

        UpdateTransforms(sdoc);

        // Counter-act the rotation on unselected children
        for (int bone_index = 0; bone_index < sdoc->bone_count; bone_index++) {
            if (!IsBoneSelected(bone_index))
                continue;
            CounterActParentTransform(sdoc, bone_index);
        }

        UpdateTransforms(sdoc);
        UpdateAllAnimationTransforms(sdoc);
        MarkModified();
    }

    static void CommitRotateTool(float) {
    }

    static void BeginRotateTool() {
        SkeletonDocument* sdoc = GetSkeletonData();
        if (sdoc->selected_bone_count <= 0)
            return;

        UpdateSelectionCenter();
        SaveState();
        RecordUndo();
        BeginRotateTool({.origin=g_skeleton_editor.selection_center_world, .update=UpdateRotateTool, .commit=CommitRotateTool, .cancel=CancelSkeletonTool});
    }

    static void UpdateScaleTool(const Vec2& scale) {
        SkeletonDocument* sdoc = GetSkeletonData();
        for (i32 bone_index=0; bone_index<sdoc->bone_count; bone_index++) {
            if (!IsBoneSelected(bone_index))
                continue;

            BoneData& b = sdoc->bones[bone_index];
            BoneData& sb = g_skeleton_editor.saved_bones[bone_index];
            b.length = Clamp(sb.length * scale.x, 0.05f, 10.0f);
        }

        UpdateTransforms(sdoc);
        UpdateAllAnimationTransforms(sdoc);
    }

    static void CommitScaleTool(const Vec2&) {
        MarkModified();
    }

    static void BeginScaleTool() {
        SkeletonDocument* sdoc = GetSkeletonData();
        if (sdoc->selected_bone_count <= 0)
            return;

        UpdateSelectionCenter();
        SaveState();
        RecordUndo();
        BeginScaleTool({.origin=g_skeleton_editor.selection_center_world, .update=UpdateScaleTool, .commit=CommitScaleTool, .cancel=CancelSkeletonTool});
    }

    static void HandleRemove() {
        SkeletonDocument* sdoc = GetSkeletonData();
        if (sdoc->selected_bone_count <= 0)
            return;

        BeginUndoGroup();
        RecordUndo();

        for (int i=sdoc->bone_count - 1; i >=0; i--) {
            if (!IsBoneSelected(i))
                continue;

            RemoveBone(sdoc, i);
        }

        UpdateAllAnimations(sdoc);
        EndUndoGroup();
        ClearSelection();
        MarkModified();
    }

    static void CommitParentTool(const Vec2& position) {
        SkeletonDocument* sdoc = GetSkeletonData();
        int bone_index = HitTestBone(sdoc, position);
        if (bone_index != -1) {
            BeginUndoGroup();
            RecordUndo(sdoc);
            bone_index = ReparentBone(sdoc, GetFirstSelectedBoneIndex(), bone_index);
            ClearSelection();
            SetBoneSelected(bone_index, true);
            UpdateAllAnimations(sdoc);
            EndUndoGroup();
            return;
        }

        Document* hit_asset = HitTestAssets(position);
        if (!hit_asset)
            return;

        // RecordUndo();
        // sdoc->skins[sdoc->skin_count++] = {
        //     .asset_name = hit_asset->name,
        //     .mesh = static_cast<MeshDocument*>(hit_asset),
        // };
        // UpdateTransforms(sdoc);

        MarkModified();
    }

    static void BeginParentTool() {
        BeginSelectTool({.commit=CommitParentTool});
    }

    static void CommitUnparentTool(const Vec2& position) {
#if 0
        SkeletonDocument* sdoc = GetSkeletonData();
        for (int i=0; i<sdoc->skin_count; i++) {
            Skin& sm = sdoc->skins[i];
            if (!sm.mesh || !OverlapPoint(sm.mesh, sdoc->position, position))
                continue;

            RecordUndo(sdoc);
            for (int j=i; j<sdoc->skin_count-1; j++)
                sdoc->skins[j] = sdoc->skins[j+1];

            sdoc->skin_count--;

            MarkModified();
            return;
        }
#endif        
    }

    static void BeginUnparentTool() {
        BeginSelectTool({.commit=CommitUnparentTool});
    }

    static void BeginExtrudeTool() {
        SkeletonDocument* sdoc = GetSkeletonData();
        if (sdoc->selected_bone_count != 1)
            return;

        if (sdoc->bone_count >= MAX_BONES)
            return;

        int parent_bone_index = GetFirstSelectedBoneIndex();
        assert(parent_bone_index != -1);

        BoneData& parent_bone = sdoc->bones[parent_bone_index];

        RecordUndo();

        sdoc->bones[sdoc->bone_count] = {
            .name = GetUniqueBoneName(sdoc),
            .index = sdoc->bone_count,
            .parent_index = parent_bone_index,
            .transform = { .scale = VEC2_ONE },
            .length = parent_bone.length
        };
        sdoc->bone_count++;

        UpdateTransforms(sdoc);
        ClearSelection();
        SetBoneSelected(sdoc->bone_count-1, true);
        BeginMoveTool(false);
    }

    static void RenameBoneCommand(const Command& command) {
        if (command.arg_count != 0)
            return;

        SkeletonDocument* sdoc = GetSkeletonData();
        if (sdoc->selected_bone_count != 1) {
            LogError("can only rename a single selected bone");
            return;
        }

        MarkModified();
        BeginUndoGroup();
        RecordUndo();
        sdoc->bones[GetFirstSelectedBoneIndex()].name = command.name;
        UpdateAllAnimations(sdoc);
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
            .initial_text = GetSkeletonData()->bones[bone_index].name->value
        });
    }

    static void BeginSkeletonEditor(Document*) {
        PushInputSet(g_skeleton_editor.input);
        ClearSelection();
    }

    static void EndSkeletonEditor() {
        PopInputSet();
    }

    static void ResetRotation() {
        SkeletonDocument* sdoc = GetSkeletonData();
        RecordUndo(sdoc);
        for (int bone_index=1; bone_index<sdoc->bone_count; bone_index++) {
            if (!IsBoneSelected(bone_index))
                continue;

            BoneData& b = sdoc->bones[bone_index];
            b.transform.rotation = 0;
        }

        UpdateTransforms(sdoc);
        MarkModified(sdoc);
    }

    static void ResetTranslation() {
        SkeletonDocument* sdoc = GetSkeletonData();
        RecordUndo(sdoc);

        if (IsBoneSelected(0)) {
            BoneData& bone = sdoc->bones[0];
            bone.transform.position = VEC2_ZERO;
        }

        for (int bone_index=1; bone_index<sdoc->bone_count; bone_index++) {
            if (!IsBoneSelected(bone_index))
                continue;

            BoneData& b = sdoc->bones[bone_index];
            BoneData& p = sdoc->bones[b.parent_index];
            b.transform.position = Vec2{p.length, 0};
        }

        UpdateTransforms(sdoc);
        MarkModified(sdoc);
    }

    void InitSkeletonEditor(SkeletonDocument* sdoc) {
        sdoc->vtable.editor_begin = BeginSkeletonEditor;
        sdoc->vtable.editor_end = EndSkeletonEditor;
        sdoc->vtable.editor_draw = DrawSkeletonData;
        sdoc->vtable.editor_update = UpdateSkeletonEditor;
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
}
