//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct SdfEditor {
    SdfData* data;
    Shortcut* shortcuts;
};

static SdfEditor g_sdf_editor = {};

inline SdfData* GetSdfData() {
    AssetData* a = g_sdf_editor.data;
    assert(a->type == ASSET_TYPE_SDF);
    return static_cast<SdfData*>(a);
}

inline MeshData* GetSdfMeshData() {
    return &GetSdfData()->mesh;
}

extern void DrawMeshEditorOverlays(MeshData* m);

static void DrawSdfEditor() {
    SdfData* s = GetSdfData();
    MeshData* m = GetSdfMeshData();

    // Start SDF preview generation if needed (runs in background)
    if (s->preview_dirty && !s->preview_task) {
        GenerateSdfPreview(s);
        s->preview_dirty = false;  // Mark as no longer dirty (task is running)
    }

    // Draw SDF preview if available
    if (s->runtime && !s->runtime->preview_faces.empty() && s->preview_material) {
        BindMaterial(s->preview_material);
        BindColor(COLOR_WHITE);
        for (const auto& face : s->runtime->preview_faces) {
            DrawMesh(face.mesh, Translate(s->position));
        }
    }

    // Draw mesh editor overlays (vertices, edges, handles) without the mesh fill
    DrawMeshEditorOverlays(m);
}

static void UpdateSdfEditor() {
    SdfData* s = GetSdfData();
    MeshData* m = GetSdfMeshData();

    // Sync modified state from mesh to SDF
    if (m->modified) {
        MarkModified(s);
        MarkSdfPreviewDirty(s);  // Regenerate SDF preview
        m->modified = false;
    }

    // Check SDF-specific shortcuts first
    CheckShortcuts(g_sdf_editor.shortcuts, GetInputSet());

    // Delegate to mesh editor update
    m->vtable.editor_update();
}

static void BeginSdfEditor(AssetData* a) {
    SdfData* s = static_cast<SdfData*>(a);
    g_sdf_editor.data = s;

    // Sync position and start mesh editor
    s->mesh.position = s->position;
    s->mesh.vtable.editor_begin(&s->mesh);
}

static void EndSdfEditor() {
    //SdfData* s = GetSdfData();
    MeshData* m = GetSdfMeshData();

    if (m->vtable.editor_end)
        m->vtable.editor_end();

    g_sdf_editor.data = nullptr;
}

static Bounds2 GetSdfEditorBounds() {
    MeshData* m = GetSdfMeshData();
    if (m->vtable.editor_bounds)
        return m->vtable.editor_bounds();
    return GetBounds(m);
}

void ShutdownSdfEditor() {
    g_sdf_editor = {};
}

void InitSdfEditor() {
    // SDF-specific shortcuts (in addition to mesh editor shortcuts)
    static Shortcut shortcuts[] = {
        // Add SDF-specific shortcuts here as needed
        { INPUT_CODE_NONE }
    };

    g_sdf_editor = {};
    g_sdf_editor.shortcuts = shortcuts;
}

void InitSdfEditor(SdfData* s) {
    s->vtable.editor_begin = BeginSdfEditor;
    s->vtable.editor_end = EndSdfEditor;
    s->vtable.editor_update = UpdateSdfEditor;
    s->vtable.editor_draw = DrawSdfEditor;
    s->vtable.editor_bounds = GetSdfEditorBounds;
}
