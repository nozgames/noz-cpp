//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

extern void LoadMeshData(MeshData* m, Tokenizer& tk, bool multiple_mesh);
extern void SaveMeshData(MeshData* m, Stream* stream);
extern void InitSdfEditor(SdfData* s);
extern void DrawMesh(MeshData* m, const Mat3& transform, Material* material);

static Shader* g_sdf_shader = nullptr;

static void DrawSdf(AssetData* a) {
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);

    // Start preview generation if needed (runs in background)
    if (s->preview_dirty && !s->preview_task) {
        GenerateSdfPreview(s);
        s->preview_dirty = false;  // Mark as no longer dirty (task is running)
    }

    // Draw using SDF preview if available, otherwise fall back to mesh
    if (s->runtime && !s->runtime->preview_faces.empty() && s->preview_material) {
        BindMaterial(s->preview_material);
        BindColor(COLOR_WHITE);
        for (const auto& face : s->runtime->preview_faces) {
            DrawMesh(face.mesh, Translate(a->position));
        }
    } else {
        DrawMesh(&s->mesh, Translate(a->position), nullptr);
    }
}

static void LoadSdfData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);

    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, a->path);
    Tokenizer tk;
    Init(tk, contents.c_str());

    // Parse SDF-specific settings first
    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "sdf")) {
            // Parse sdf settings: r = range, m = min_resolution, p = pixels_per_unit
            while (!IsEOF(tk) && !Peek(tk, "\n")) {
                if (ExpectIdentifier(tk, "r")) {
                    s->sdf_range = ExpectFloat(tk);
                } else if (ExpectIdentifier(tk, "m")) {
                    s->min_resolution = ExpectInt(tk);
                } else if (ExpectIdentifier(tk, "p")) {
                    s->pixels_per_unit = ExpectFloat(tk);
                } else {
                    break;
                }
            }
        } else {
            break;
        }
    }

    // Load the mesh data
    LoadMeshData(&s->mesh, tk, false);

    // Copy bounds from mesh
    a->bounds = s->mesh.bounds;

    // Mark preview as needing generation
    s->preview_dirty = true;
}

static void SaveSdfData(AssetData* a, const std::filesystem::path& path) {
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

    // Write SDF settings
    WriteCSTR(stream, "sdf r %f m %d p %f\n\n", s->sdf_range, s->min_resolution, s->pixels_per_unit);

    // Write mesh data
    SaveMeshData(&s->mesh, stream);

    SaveStream(stream, path);
    Free(stream);
}

static void LoadSdfMetaData(AssetData* a, Props* meta) {
    assert(a);
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);

    // Load mesh metadata
    if (s->mesh.vtable.load_metadata)
        s->mesh.vtable.load_metadata(&s->mesh, meta);
}

static void SaveSdfMetaData(AssetData* a, Props* meta) {
    assert(a);
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);

    // Save mesh metadata
    if (s->mesh.vtable.save_metadata)
        s->mesh.vtable.save_metadata(&s->mesh, meta);
}

static void PostLoadSdfData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);

    if (s->mesh.vtable.post_load)
        s->mesh.vtable.post_load(&s->mesh);

    // Use dedicated SDF shader, fall back to text shader if not available
    if (!g_sdf_shader) {
        g_sdf_shader = SHADER_SDF ? SHADER_SDF : SHADER_TEXT;
    }
}

AssetData* NewSdfData(const std::filesystem::path& path) {
    constexpr const char* default_sdf =
        "sdf r 0.25 m 32 p 32.0\n"
        "\n"
        "d 0.5\n"
        "\n"
        "v -1 -1 e 1 h 0\n"
        "v 1 -1 e 1 h 0\n"
        "v 1 1 e 1 h 0\n"
        "v -1 1 e 1 h 0\n"
        "\n"
        "f 0 1 2 3 c 0 n 0 0 1\n";

    std::filesystem::path full_path = path.is_relative()
        ? std::filesystem::path(g_editor.project_path) / g_editor.save_dir / "sdf" / path
        : path;
    full_path += ".sdf";

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
    WriteCSTR(stream, default_sdf);
    SaveStream(stream, full_path);
    Free(stream);

    AssetData* a = CreateAssetData(full_path);
    if (a && a->vtable.load)
        a->vtable.load(a);
    return a;
}

static void DestroySdfData(AssetData* a) {
    SdfData* s = static_cast<SdfData*>(a);

    // Cancel any running preview task
    if (s->preview_task) {
        Cancel(s->preview_task);
        s->preview_task = noz::TASK_NULL;
    }

    // Free preview resources
    if (s->runtime) {
        for (auto& face : s->runtime->preview_faces)
            Free(face.mesh);
        delete s->runtime;
        s->runtime = nullptr;
    }
    Free(s->preview_atlas);
    Free(s->preview_material);
    s->preview_atlas = nullptr;
    s->preview_material = nullptr;

    if (s->mesh.vtable.destructor)
        s->mesh.vtable.destructor(&s->mesh);
}

static void SdfUndoRedo(AssetData* a) {
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);
    if (s->mesh.vtable.undo_redo)
        s->mesh.vtable.undo_redo(&s->mesh);

    // Mark preview as dirty after undo/redo
    s->preview_dirty = true;
}

static void CloneSdfData(AssetData* a) {
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);
    if (s->mesh.vtable.clone)
        s->mesh.vtable.clone(&s->mesh);

    // Reset preview for clone (will regenerate)
    s->runtime = new SdfRuntimeData();
    s->preview_atlas = nullptr;
    s->preview_material = nullptr;
    s->preview_dirty = true;
    s->preview_task = noz::TASK_NULL;
}

void MarkSdfPreviewDirty(SdfData* s) {
    s->preview_dirty = true;
}

Shader* GetSdfShader() {
    return g_sdf_shader;
}

void InitSdfData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* s = static_cast<SdfData*>(a);

    // Initialize defaults
    // sdf_range: distance field range in world units (smaller = tighter edge)
    // For good AA, sdf_range * pixels_per_unit should be ~4-8 pixels
    s->sdf_range = 0.25f;
    s->min_resolution = 32;
    s->pixels_per_unit = 32.0f;

    // Initialize runtime and preview
    s->runtime = new SdfRuntimeData();
    s->preview_atlas = nullptr;
    s->preview_material = nullptr;
    s->preview_dirty = true;
    s->preview_task = noz::TASK_NULL;

    // Initialize the embedded mesh
    InitMeshData(&s->mesh);

    // Set up SDF vtable (non-editor callbacks)
    s->vtable = {
        .destructor = DestroySdfData,
        .load = LoadSdfData,
        .post_load = PostLoadSdfData,
        .save = SaveSdfData,
        .load_metadata = LoadSdfMetaData,
        .save_metadata = SaveSdfMetaData,
        .draw = DrawSdf,
        .clone = CloneSdfData,
        .undo_redo = SdfUndoRedo,
    };

    // Set up editor callbacks from sdf_editor.cpp
    InitSdfEditor(s);
}
