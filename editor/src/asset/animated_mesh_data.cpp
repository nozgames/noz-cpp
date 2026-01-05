//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

static void InitAnimatedMeshData(AnimatedMeshData* m);
extern void InitAnimatedMeshEditor(AnimatedMeshData* m);
extern void LoadMeshData(MeshData* m, Tokenizer& tk, bool multiple_mesh);
extern void SaveMeshData(MeshData* m, Stream* stream);

static void DrawAnimatedMeshData(AssetData* a) {
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    AnimatedMeshDataImpl* impl = m->impl;

    if (impl->playing) {
        impl->play_time = Update(impl->playing, impl->play_time);
        BindColor(COLOR_WHITE, Vec2Int(0,0));
        BindMaterial(g_view.shaded_material);
        DrawMesh(impl->playing, Translate(a->position), impl->play_time);
    } else if (impl->frame_count > 0) {
        DrawMesh(&impl->frames[0], Translate(a->position));
    }
}

static void SaveAnimatedMeshData(AssetData* a, const std::filesystem::path& path) {
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    AnimatedMeshDataImpl* impl = m->impl;

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

    for (int i=0; i<impl->frame_count; i++) {
        WriteCSTR(stream, "m\n");
        SaveMeshData(&impl->frames[i], stream);
    }
    SaveStream(stream, path);
    Free(stream);
}

AnimatedMesh* ToAnimatedMesh(AnimatedMeshData* m) {
    AnimatedMeshDataImpl* impl = m->impl;
    Mesh* frames[ANIMATED_MESH_MAX_FRAMES];
    int frame_count = 0;
    for (int i=0; i<impl->frame_count; i++) {
        Mesh* frame = ToMesh(&impl->frames[i], true, false);
        if (!frame)
            continue;

        frames[frame_count++] = frame;
    }

    return CreateAnimatedMesh(ALLOCATOR_DEFAULT, m->name, frame_count, frames);
}

static void ParseMesh(AnimatedMeshData* m, Tokenizer& tk) {
    AnimatedMeshDataImpl* impl = m->impl;
    if (impl->frame_count >= ANIMATED_MESH_MAX_FRAMES)
        ThrowError("too many frames in animated mesh");

    MeshData& frame = impl->frames[impl->frame_count++];

    InitMeshData(&frame);
    LoadMeshData(&frame, tk, true);
}

static void LoadAnimatedMeshData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    AnimatedMeshDataImpl* impl = m->impl;

    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, a->path.value);
    Tokenizer tk;
    Init(tk, contents.c_str());

    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "m")) {
            ParseMesh(m, tk);
        } else {
            char error[1024];
            GetString(tk, error, sizeof(error) - 1);
            ThrowError("invalid token '%s' in mesh", error);
        }
    }

    Bounds2 bounds = impl->frames->bounds;
    for (int i=1; i<impl->frame_count; i++)
        bounds = Union(bounds, impl->frames[i].bounds);

    a->bounds = bounds;
}

AnimatedMeshData* LoadAnimatedMeshData(const std::filesystem::path& path) {
    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
    Tokenizer tk;
    Init(tk, contents.c_str());

    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(CreateAssetData(path));
    assert(m);
    LoadAnimatedMeshData(m);
    return m;
}

AssetData* NewAnimatedMeshData(const std::filesystem::path& path) {
    const char* default_mesh =
        "m\n"
        "v -1 -1 e 1 h 0\n"
        "v 1 -1 e 1 h 0\n"
        "v 1 1 e 1 h 0\n"
        "v -1 1 e 1 h 0\n"
        "f 0 1 2 3 c 1 0\n";

    std::string text = default_mesh;

    if (g_view.selected_asset_count == 1) {
        AssetData* selected = GetFirstSelectedAsset();
        if (selected && selected->type == ASSET_TYPE_MESH) {
            text = "m\n";
            text += ReadAllText(ALLOCATOR_DEFAULT, selected->path.value);
        }
    }

    std::filesystem::path full_path = path.is_relative() ?  std::filesystem::path(g_editor.project_path) / "assets" / "animated_meshes" / path : path;
    full_path += ".amesh";

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
    WriteCSTR(stream, text.c_str());
    SaveStream(stream, full_path);
    Free(stream);

    return LoadAnimatedMeshData(full_path);
}

static void AllocateAnimatedMeshImpl(AssetData* a) {
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    m->impl = static_cast<AnimatedMeshDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(AnimatedMeshDataImpl)));
    memset(m->impl, 0, sizeof(AnimatedMeshDataImpl));
}

static void CloneAnimatedMeshData(AssetData* a) {
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    AnimatedMeshDataImpl* old_impl = m->impl;
    AllocateAnimatedMeshImpl(m);
    memcpy(m->impl, old_impl, sizeof(AnimatedMeshDataImpl));
}

static void DestroyAnimatedMeshData(AssetData* a) {
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    Free(m->impl);
    m->impl = nullptr;
}

static void PlayAnimatedMeshData(AssetData* a) {
    AnimatedMeshData* m = static_cast<AnimatedMeshData*>(a);
    AnimatedMeshDataImpl* impl = m->impl;
    assert(m);

    if (impl->playing) {
        Free(impl->playing);
        impl->playing = nullptr;
    } else {
        impl->playing = ToAnimatedMesh(m);
    }

    impl->play_time = 0.0f;
}

static void InitAnimatedMeshData(AnimatedMeshData* m) {
    m->impl = static_cast<AnimatedMeshDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(AnimatedMeshDataImpl)));
    m->vtable = {
        .destructor = DestroyAnimatedMeshData,
        .load = LoadAnimatedMeshData,
        .save = SaveAnimatedMeshData,
        .draw = DrawAnimatedMeshData,
        .play = PlayAnimatedMeshData,
        .clone = CloneAnimatedMeshData,
    };

    InitAnimatedMeshEditor(m);
}

void InitAnimatedMeshData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ANIMATED_MESH);
    InitAnimatedMeshData(static_cast<AnimatedMeshData*>(a));
}
