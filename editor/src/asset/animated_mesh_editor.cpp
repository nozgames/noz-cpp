//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

constexpr float FRAME_SIZE_X = 20;
constexpr float FRAME_SIZE_Y = 40;
constexpr float FRAME_BORDER_SIZE = 1;
constexpr Color FRAME_BORDER_COLOR = Color24ToColor(32,32,32);
constexpr float DOPESHEET_FRAME_DOT_SIZE = 5;
constexpr float DOPESHEET_FRAME_DOT_OFFSET_X = FRAME_SIZE_X * 0.5f - DOPESHEET_FRAME_DOT_SIZE * 0.5f;
constexpr float DOPESHEET_FRAME_DOT_OFFSET_Y = 5;
constexpr Color DOPESHEET_FRAME_DOT_COLOR = FRAME_BORDER_COLOR;
constexpr Color FRAME_COLOR = Color32ToColor(100, 100, 100, 255);
constexpr Color DOPESHEET_SELECTED_FRAME_COLOR = COLOR_VERTEX_SELECTED;

extern void RefreshMeshEditorSelection();

struct AnimatedMeshEditor {
    AnimatedMeshData* data;
    Shortcut* shortcuts;
    float playback_time;
    AnimatedMesh* playing;
    MeshData clipboard;
    bool has_clipboard;
    Mesh* prev_frame_mesh;
    Mesh* next_frame_mesh;
    int cached_frame;
};

static AnimatedMeshEditor g_animated_mesh_editor = {};

inline AnimatedMeshData* GetAnimatedMeshData() {
    AssetData* a = g_animated_mesh_editor.data;
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    return static_cast<AnimatedMeshData*>(a);
}

inline MeshData* GetAnimatedMeshFrameData(int frame_index) {
    AnimatedMeshData* m = GetAnimatedMeshData();
    assert(frame_index >= 0 && frame_index < m->frame_count);
    return &m->frames[frame_index];
}

inline MeshData* GetAnimatedMeshFrameData() {
    AnimatedMeshData* m = GetAnimatedMeshData();
    return &m->frames[m->current_frame];
}

static Mesh* BuildEdgeMesh(MeshData* m, Mesh* existing) {
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, MAX_VERTICES * 4, MAX_EDGES * 6);

    float line_width = 0.02f * g_view.zoom_ref_scale;

    for (int i = 0; i < m->edge_count; i++) {
        const EdgeData& e = m->edges[i];
        Vec2 v0 = m->vertices[e.v0].position;
        Vec2 v1 = m->vertices[e.v1].position;
        Vec2 dir = Normalize(v1 - v0);
        Vec2 n = Perpendicular(dir) * line_width;

        u16 base = GetVertexCount(builder);
        AddVertex(builder, v0 - n);
        AddVertex(builder, v0 + n);
        AddVertex(builder, v1 + n);
        AddVertex(builder, v1 - n);
        AddTriangle(builder, base, base + 1, base + 2);
        AddTriangle(builder, base, base + 2, base + 3);
    }

    Mesh* result;
    if (!existing)
        result = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
    else {
        UpdateMeshFromBuilder(existing, builder);
        result = existing;
    }

    PopScratch();
    return result;
}

static void DrawAnimatedMeshEditor() {
    AnimatedMeshData* m = GetAnimatedMeshData();
    MeshData* f = GetAnimatedMeshFrameData();

    if (g_animated_mesh_editor.playing) {
        BindColor(COLOR_WHITE);
        BindMaterial(g_view.shaded_material);
        DrawMesh(g_animated_mesh_editor.playing, Translate(m->position), g_animated_mesh_editor.playback_time);
    } else {
        f->vtable.editor_draw();
    }

    // Rebuild edge meshes if frame changed
    if (g_animated_mesh_editor.cached_frame != m->current_frame) {
        g_animated_mesh_editor.cached_frame = m->current_frame;

        int prev_frame = (m->current_frame - 1 + m->frame_count) % m->frame_count;
        if (prev_frame != m->current_frame) {
            MeshData* pf = GetAnimatedMeshFrameData(prev_frame);
            g_animated_mesh_editor.prev_frame_mesh = BuildEdgeMesh(pf, g_animated_mesh_editor.prev_frame_mesh);
        }

        int next_frame = (m->current_frame + 1) % m->frame_count;
        if (next_frame != m->current_frame) {
            MeshData* nf = GetAnimatedMeshFrameData(next_frame);
            g_animated_mesh_editor.next_frame_mesh = BuildEdgeMesh(nf, g_animated_mesh_editor.next_frame_mesh);
        }
    }

    // Draw prev frame edges
    int prev_frame = (m->current_frame - 1 + m->frame_count) % m->frame_count;
    if (prev_frame != m->current_frame && g_animated_mesh_editor.prev_frame_mesh) {
        BindColor(COLOR_RED);
        BindMaterial(g_view.shaded_material);
        DrawMesh(g_animated_mesh_editor.prev_frame_mesh, Translate(m->position));
    }

    // Draw next frame edges
    int next_frame = (m->current_frame + 1) % m->frame_count;
    if (next_frame != m->current_frame && g_animated_mesh_editor.next_frame_mesh) {
        BindColor(COLOR_GREEN);
        BindMaterial(g_view.shaded_material);
        DrawMesh(g_animated_mesh_editor.next_frame_mesh, Translate(m->position));
    }
}

static void UpdateAnimatedMeshEditor() {
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(GetAssetData());
    MeshData* current = GetAnimatedMeshFrameData();
    if (current && current->modified) {
        MarkModified(m);
        current->modified = false;
    }

    if (g_animated_mesh_editor.playing) {
        AnimatedMesh* am = ToAnimatedMesh(m);
        g_animated_mesh_editor.playback_time = Update(am, g_animated_mesh_editor.playback_time, 1.0f, true);
        assert(FloorToInt(g_animated_mesh_editor.playback_time * ANIMATION_FRAME_RATE) < GetFrameCount(am));
    }

    CheckShortcuts(g_animated_mesh_editor.shortcuts, GetInputSet());

    current->vtable.editor_update();

    BeginCanvas();
    BeginContainer({.align=ALIGN_BOTTOM_CENTER, .margin=EdgeInsetsBottom(60)});
    BeginRow();

    int current_frame = m->current_frame;
    for (int frame_index=0; frame_index<m->frame_count; frame_index++) {
        MeshData* f = &m->frames[frame_index];
        BeginContainer({
            .width=FRAME_SIZE_X * (1 + f->hold) + FRAME_BORDER_SIZE * 2,
            .height=FRAME_SIZE_Y + FRAME_BORDER_SIZE * 2,
            .margin=EdgeInsetsLeft(-2),
            .color = frame_index == current_frame
                ? DOPESHEET_SELECTED_FRAME_COLOR
                : FRAME_COLOR,
            .border = {.width=FRAME_BORDER_SIZE, .color=FRAME_BORDER_COLOR}
        });
        BeginContainer({.align=ALIGN_BOTTOM_LEFT, .margin=EdgeInsetsBottomLeft(DOPESHEET_FRAME_DOT_OFFSET_Y, DOPESHEET_FRAME_DOT_OFFSET_X)});
            Container({.width=DOPESHEET_FRAME_DOT_SIZE, .height=DOPESHEET_FRAME_DOT_SIZE, .color=DOPESHEET_FRAME_DOT_COLOR});
        EndContainer();
        EndContainer();
    }

    EndRow();
    EndContainer();
    EndCanvas();
}

static void SetFrame(int frame_count) {
    AnimatedMeshData* m = GetAnimatedMeshData();
    if (m->current_frame != -1) {
        MeshData* f = &m->frames[m->current_frame];
        if (f->modified) {
            MarkModified(m);
            f->modified = false;
        }
        f->vtable.editor_end();
    }

    m->current_frame = Clamp(frame_count, 0, m->frame_count - 1);

    MeshData* f = &m->frames[m->current_frame];
    f->position = m->position;
    f->vtable.editor_begin(f);
}

static void BeginAnimatedMeshEditor(AssetData* a) {
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    m->current_frame = -1;
    g_animated_mesh_editor.data = m;
    g_animated_mesh_editor.cached_frame = -1;
    SetFrame(0);
}

static void EndAnimatedMeshEditor() {
    AnimatedMeshData* m = GetAnimatedMeshData();
    if (m->current_frame != -1) {
        MeshData* f = &m->frames[m->current_frame];
        f->vtable.editor_end();
    }

    Free(g_animated_mesh_editor.prev_frame_mesh);
    Free(g_animated_mesh_editor.next_frame_mesh);
    g_animated_mesh_editor.prev_frame_mesh = nullptr;
    g_animated_mesh_editor.next_frame_mesh = nullptr;
    g_animated_mesh_editor.playing = nullptr;
}

void ShutdownAnimatedMeshEditor() {
    Free(g_animated_mesh_editor.prev_frame_mesh);
    Free(g_animated_mesh_editor.next_frame_mesh);
    g_animated_mesh_editor.prev_frame_mesh = nullptr;
    g_animated_mesh_editor.next_frame_mesh = nullptr;
    g_animated_mesh_editor.has_clipboard = false;
}

static void SetPrevFrame() {
    SetFrame((GetAnimatedMeshData()->current_frame - 1 + GetAnimatedMeshData()->frame_count) % GetAnimatedMeshData()->frame_count);
}

static void SetNextFrame() {
    SetFrame((GetAnimatedMeshData()->current_frame + 1) % GetAnimatedMeshData()->frame_count);
}

static void InsertFrameAfter() {
    AnimatedMeshData* m = GetAnimatedMeshData();
    MeshData* cf = GetAnimatedMeshFrameData(m->current_frame);
    m->frame_count++;

    for (int frame_index=m->frame_count - 1; frame_index > m->current_frame; frame_index--)
        m->frames[frame_index] = m->frames[frame_index - 1];

    MeshData* nf = GetAnimatedMeshFrameData(m->current_frame+1);
    InitMeshData(nf);
    *nf = *cf;
    nf->vtable.clone(nf);
    SetFrame(m->current_frame + 1);
    MarkModified(m);
}

static void TogglePlayAnimation() {
    if (g_animated_mesh_editor.playing) {
        Free(g_animated_mesh_editor.playing);
        g_animated_mesh_editor.playing = nullptr;
    } else {
        g_animated_mesh_editor.playing = ToAnimatedMesh(GetAnimatedMeshData());
        g_animated_mesh_editor.playback_time = 0.0f;
    }
}

static void IncHoldFrame() {
    GetAnimatedMeshFrameData()->hold++;
}

static void DecHoldFrame() {
    MeshData* f = GetAnimatedMeshFrameData();
    f->hold = Max(f->hold - 1, 0);
}

static void DeleteFrame() {
    AnimatedMeshData* m = GetAnimatedMeshData();
    if (m->frame_count <= 1)
        return;

    MeshData* cf = GetAnimatedMeshFrameData(m->current_frame);
    cf->vtable.editor_end();

    int deleted_frame = m->current_frame;
    for (int frame_index = deleted_frame; frame_index < m->frame_count - 1; frame_index++)
        m->frames[frame_index] = m->frames[frame_index + 1];

    m->frame_count--;
    m->current_frame = -1;
    SetFrame(Min(deleted_frame, m->frame_count - 1));
    MarkModified(m);
}

static void CopyFrame() {
    MeshData* f = GetAnimatedMeshFrameData();
    MeshData* clip = &g_animated_mesh_editor.clipboard;

    // Initialize clipboard
    if (!clip->data)
        InitMeshData(clip);

    clip->vertex_count = 0;
    clip->face_count = 0;
    clip->tag_count = 0;

    // Build vertex index mapping (old index -> new index in clipboard)
    int vertex_map[MAX_VERTICES];
    for (int i = 0; i < MAX_VERTICES; i++)
        vertex_map[i] = -1;

    // First pass: collect vertices used by selected faces
    for (int face_index = 0; face_index < f->face_count; face_index++) {
        FaceData& face = f->faces[face_index];
        if (!face.selected)
            continue;

        for (int vi = 0; vi < face.vertex_count; vi++) {
            int old_idx = face.vertices[vi];
            if (vertex_map[old_idx] == -1) {
                vertex_map[old_idx] = clip->vertex_count;
                clip->vertices[clip->vertex_count++] = f->vertices[old_idx];
            }
        }
    }

    // Second pass: copy selected faces with remapped vertex indices
    for (int face_index = 0; face_index < f->face_count; face_index++) {
        FaceData& face = f->faces[face_index];
        if (!face.selected)
            continue;

        FaceData& new_face = clip->faces[clip->face_count++];
        new_face = face;
        new_face.selected = false;

        for (int vi = 0; vi < new_face.vertex_count; vi++)
            new_face.vertices[vi] = vertex_map[face.vertices[vi]];
    }

    g_animated_mesh_editor.has_clipboard = clip->face_count > 0;
}

static void PasteFrame() {
    if (!g_animated_mesh_editor.has_clipboard)
        return;

    AnimatedMeshData* m = GetAnimatedMeshData();
    MeshData* f = GetAnimatedMeshFrameData();
    MeshData* src = &g_animated_mesh_editor.clipboard;

    // Check if we have room
    if (f->vertex_count + src->vertex_count > MAX_VERTICES ||
        f->face_count + src->face_count > MAX_FACES)
        return;

    RecordUndo(m);

    // Deselect everything first
    for (int i = 0; i < f->vertex_count; i++)
        f->vertices[i].selected = false;
    for (int i = 0; i < f->face_count; i++)
        f->faces[i].selected = false;

    // Base index for new vertices
    int vertex_base = f->vertex_count;

    // Add vertices from clipboard (selected)
    for (int i = 0; i < src->vertex_count; i++) {
        f->vertices[f->vertex_count] = src->vertices[i];
        f->vertices[f->vertex_count].selected = true;
        f->vertex_count++;
    }

    // Add faces from clipboard with offset vertex indices
    for (int i = 0; i < src->face_count; i++) {
        FaceData& new_face = f->faces[f->face_count++];
        new_face = src->faces[i];
        new_face.selected = true;

        for (int vi = 0; vi < new_face.vertex_count; vi++)
            new_face.vertices[vi] += vertex_base;
    }

    // Rebuild edges and mark dirty
    UpdateEdges(f);
    MarkDirty(f);

    // Refresh mesh editor selection center
    RefreshMeshEditorSelection();

    MarkModified(m);
}

void InitAnimatedMeshEditor() {
    static Shortcut shortcuts[] = {
        { KEY_Q, false, false, false, SetPrevFrame },
        { KEY_E, false, false, false, SetNextFrame },
        { KEY_O, false, false, false, InsertFrameAfter },
        { KEY_SPACE, false, false, false, TogglePlayAnimation },
        { KEY_H, false, false, false, IncHoldFrame },
        { KEY_H, false, true, false, DecHoldFrame },
        { KEY_X, false, false, true, DeleteFrame },
        { KEY_C, false, true, false, CopyFrame },
        { KEY_V, false, true, false, PasteFrame },
        { INPUT_CODE_NONE }
    };

    g_animated_mesh_editor = {};
    g_animated_mesh_editor.shortcuts = shortcuts;
}

void InitAnimatedMeshEditor(AnimatedMeshData* m) {
    m->vtable.editor_begin = BeginAnimatedMeshEditor;
    m->vtable.editor_end = EndAnimatedMeshEditor;
    m->vtable.editor_update = UpdateAnimatedMeshEditor;
    m->vtable.editor_draw = DrawAnimatedMeshEditor;
}
