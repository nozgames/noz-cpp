//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace fs = std::filesystem;

const Name* MakeCanonicalAssetName(const fs::path& path)
{
    return MakeCanonicalAssetName(fs::path(path).replace_extension("").filename().string().c_str());
}

const Name* MakeCanonicalAssetName(const char* name)
{
    std::string result = name;
    Lower(result.data(), (u32)result.size());
    Replace(result.data(), (u32)result.size(), '/', '_');
    Replace(result.data(), (u32)result.size(), '.', '_');
    Replace(result.data(), (u32)result.size(), ' ', '_');
    Replace(result.data(), (u32)result.size(), '-', '_');
    return GetName(result.c_str());
}

bool IsFile(AssetData* a) {
    return a->path.length > 0;
}

static void DestroyAssetData(void* p) {
    AssetData* a = static_cast<AssetData*>(p);
    if (a->vtable.destructor) {
        a->vtable.destructor(a);
    }
}

AssetData* CreateAssetData(const std::filesystem::path& path) {
    // Find the asset type info from extension
    const EditorAssetTypeInfo* type_info = FindEditorAssetTypeByExtension(path.extension().string().c_str());
    if (!type_info)
        return nullptr;

    // Allocate GenericAssetData (AssetData + void* data pointer)
    AssetData* a = static_cast<AssetData*>(Alloc(g_editor.asset_allocator, sizeof(GenericAssetData), DestroyAssetData));
    Set(a->path, canonical(path).string().c_str());
    Lower(a->path);
    a->name = MakeCanonicalAssetName(path);
    a->bounds = Bounds2{{-0.5f, -0.5f}, {0.5f, 0.5f}};
    a->asset_path_index = -1;

    for (int i=0; i<g_editor.source_path_count; i++) {
        if (Equals(g_editor.source_paths[i].value, a->path, g_editor.source_paths[i].length, true)) {
            a->asset_path_index = i;
            break;
        }
    }

    assert(a->asset_path_index != -1);

    // Set type from registry
    a->type = static_cast<AssetType>(type_info->type_id);

    // Initialize via registry
    if (type_info->init)
        type_info->init(a);

    return a;
}

static void LoadAssetMetadata(AssetData* ea, const std::filesystem::path& path) {
    Props* props = LoadProps(std::filesystem::path(path.string() + ".meta"));
    if (!props)
        return;

    ea->position = props->GetVec2("editor", "position", VEC2_ZERO);

    if (ea->vtable.load_metadata)
        ea->vtable.load_metadata(ea, props);
}

static void SaveAssetMetadata(AssetData* a) {
    std::filesystem::path meta_path = std::filesystem::path(std::string(a->path) + ".meta");
    Props* props = LoadProps(meta_path);
    if (!props)
        props = new Props{};
    props->SetVec2("editor", "position", a->position);

    if (a->vtable.save_metadata)
        a->vtable.save_metadata(a, props);

    SaveProps(props, meta_path);
}

static void SaveAssetMetadata() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (!a->modified && !a->meta_modified)
            continue;

        SaveAssetMetadata(a);

        a->meta_modified= false;
    }
}

void SetPosition(AssetData* a, const Vec2& position) {
    a->position = position;
    a->meta_modified = true;
}

void DrawSelectedEdges(MeshData* m, const Vec2& position) {
    BindMaterial(g_view.vertex_material);

    MeshFrameData* frame = GetCurrentFrame(m);
    for (i32 edge_index=0; edge_index < frame->edge_count; edge_index++) {
        const EdgeData& ee = frame->edges[edge_index];
        if (!ee.selected)
            continue;

        const Vec2& v0 = frame->vertices[ee.v0].position;
        const Vec2& v1 = frame->vertices[ee.v1].position;
        DrawLine(v0 + position, v1 + position);
    }
}

void DrawEdges(MeshData* m, const Vec2& position) {
    BindMaterial(g_view.vertex_material);

    MeshFrameData* frame = GetCurrentFrame(m);
    for (i32 edge_index=0; edge_index < frame->edge_count; edge_index++) {
        const EdgeData& ee = frame->edges[edge_index];
        DrawLine(frame->vertices[ee.v0].position + position, frame->vertices[ee.v1].position + position);
    }
}

void DrawEdges(MeshData* m, const Mat3& transform) {
    BindMaterial(g_view.vertex_material);

    MeshFrameData* frame = GetCurrentFrame(m);
    for (i32 edge_index=0; edge_index < frame->edge_count; edge_index++) {
        const EdgeData& ee = frame->edges[edge_index];
        Vec2 p1 = TransformPoint(transform, frame->vertices[ee.v0].position);
        Vec2 p2 = TransformPoint(transform, frame->vertices[ee.v1].position);
        DrawLine(p1, p2);
    }
}

void DrawSelectedFaces(MeshData* m, const Vec2& position) {
    BindMaterial(g_view.vertex_material);

    MeshFrameData* frame = GetCurrentFrame(m);
    for (i32 face_index=0; face_index < frame->face_count; face_index++) {
        const FaceData& f = frame->faces[face_index];
        if (!f.selected)
            continue;

        for (int vertex_index=0; vertex_index<f.vertex_count; vertex_index++) {
            int v0 = f.vertices[vertex_index];
            int v1 = f.vertices[(vertex_index + 1) % f.vertex_count];
            DrawLine(frame->vertices[v0].position + position, frame->vertices[v1].position + position);
        }
    }
}

void DrawFaceCenters(MeshData* m, const Vec2& position) {
    BindMaterial(g_view.vertex_material);
    MeshFrameData* frame = GetCurrentFrame(m);
    for (int i=0; i<frame->face_count; i++) {
        FaceData& ef = frame->faces[i];
        BindColor(ef.selected ? COLOR_VERTEX_SELECTED : COLOR_VERTEX);
        DrawVertex(position + GetFaceCenter(m, i));
    }
}

void SaveAssetData() {
    SaveAssetMetadata();

    u32 count = 0;
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);;
        if (!a || !a->modified)
            continue;

        a->modified = false;

        if (a->vtable.save)
            a->vtable.save(a, a->path.value);
        else
            continue;

        count++;
    }

    if (count > 0)
        AddNotification(NOTIFICATION_TYPE_INFO, "Saved %d asset(s)", count);
}

bool OverlapPoint(AssetData* a, const Vec2& overlap_point) {
    return Contains(a->bounds + a->position, overlap_point);
}

bool OverlapPoint(AssetData* a, const Vec2& position, const Vec2& overlap_point) {
    return Contains(a->bounds + position, overlap_point);
}

bool OverlapBounds(AssetData* a, const Bounds2& bounds) {
    return Intersects(a->bounds + a->position, bounds);
}

AssetData* HitTestAssets(const Vec2& overlap_point) {
    AssetData* first_hit = nullptr;
    for (u32 i=GetAssetCount(); i>0; i--) {
        AssetData* a = GetAssetData(i-1);
        if (!a)
            continue;

        if (OverlapPoint(a, overlap_point)) {
            if (!first_hit)
                first_hit = a;
            if (!a->selected)
                return a;
        }
    }

    return first_hit;
}

AssetData* HitTestAssets(const Bounds2& hit_bounds) {
    AssetData* first_hit = nullptr;
    for (u32 i=GetAssetCount(); i>0; i--) {
        AssetData* a = GetAssetData(i-1);
        if (!a)
            continue;

        if (OverlapBounds(a, hit_bounds)) {
            if (!first_hit)
                first_hit = a;
            if (!a->selected)
                return a;
        }
    }

    return first_hit;
}

void DrawAsset(AssetData* a) {
    BindDepth(0.0f);
    if (a->vtable.draw)
        a->vtable.draw(a);
}

AssetData* GetFirstSelectedAsset() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (a->selected)
            return a;
    }

    return nullptr;
}

void ClearAssetSelection() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        a->selected = false;
    }

    g_view.selected_asset_count = 0;
}

void SetSelected(AssetData* a, bool selected) {
    assert(a);
    if (a->selected == selected)
        return;
    a->selected = true;
    g_view.selected_asset_count++;
}

void ToggleSelected(AssetData* a) {
    assert(a);
    a->selected = !a->selected;
    if (a->selected)
        g_view.selected_asset_count++;
    else
        g_view.selected_asset_count--;
}

AssetData* GetAssetData(AssetType type, const Name* name) {
    // Iterate allocator directly (not sorted array) so this works during InitAssetData
    for (u32 i = 0; i < MAX_ASSETS; i++) {
        AssetData* a = GetAssetDataInternal(i);
        if (a && (type == ASSET_TYPE_UNKNOWN || a->type == type) && a->name == name)
            return a;
    }

    return nullptr;
}

void Clone(AssetData* dst, AssetData* src) {
    bool editing = dst->editing;
    *static_cast<GenericAssetData*>(dst) = *static_cast<GenericAssetData*>(src);
    dst->editing = editing;

    if (dst->vtable.clone)
        dst->vtable.clone(dst);
}

AssetData* CreateAssetDataForImport(const std::filesystem::path& path) {
    AssetData* a = CreateAssetData(path);
    if (!a)
        return nullptr;

    LoadAssetMetadata(a, path);
    // LoadAssetData(a);
    // PostLoadAssetData(a);
    //SortAssets();

    return a;
}

void InitAssetData() {
    for (int i=0; i<g_editor.source_path_count; i++) {
        std::vector<fs::path> asset_paths;
        GetFilesInDirectory(g_editor.source_paths[i].value, asset_paths);

        for (auto& asset_path : asset_paths) {
            std::filesystem::path ext = asset_path.extension();
            if (ext == ".meta")
                continue;

            // Skip Luau definition files
            std::string filename = asset_path.filename().string();
            if (filename.ends_with(".d.luau") || filename.ends_with(".d.lua"))
                continue;

            // Skip if asset with same name already exists (from earlier source path)
            const Name* asset_name = MakeCanonicalAssetName(asset_path);
            AssetData* existing = GetAssetData(ASSET_TYPE_UNKNOWN, asset_name);
            if (existing) {
                if (strcmp(asset_name->value, "palette") == 0)
                    LogInfo("[AssetData] SKIPPING duplicate palette: %s (source_path_index=%d)", asset_path.string().c_str(), i);
                continue;
            }

            AssetData* a = nullptr;
            for (int asset_type=0; !a && asset_type<ASSET_TYPE_COUNT; asset_type++)
                a = CreateAssetData(asset_path);

            if (a)
                LoadAssetMetadata(a, asset_path);
        }
    }

    SortAssets();
}

void LoadAssetData(AssetData* a) {
    assert(a);

    if (a->loaded)
        return;

    a->loaded = true;

    if (a->vtable.load)
        a->vtable.load(a);
}

void PostLoadAssetData(AssetData* a) {
    assert(a);
    assert(a->loaded);

    if (a->post_loaded)
        return;

    if (a->vtable.post_load)
        a->vtable.post_load(a);

    a->post_loaded = true;
}

void LoadAssetData() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        LoadAssetData(a);
    }
}

void PostLoadAssetData() {
    for (u32 i=0, c=GetAssetCount(); i<c; i++) {
        PostLoadAssetData(GetAssetData(i));
    }
}

void HotloadEditorAsset(AssetType type, const Name* name){
    AssetData* a = GetAssetData(type, name);
    if (a != nullptr && a->vtable.reload)
        a->vtable.reload(a);
}

void MarkModified(AssetData* a) {
    a->modified = true;
}

void MarkMetaModified(AssetData* a) {
    a->meta_modified = true;
}

std::filesystem::path GetEditorAssetPath(const Name* name, const char* ext) {
    if (g_editor.source_path_count == 0)
        return "";

    std::filesystem::path path;
    for (int p = 0; p<g_editor.source_path_count; p++) {
        path = std::filesystem::current_path() / g_editor.source_paths[p].value / name->value;
        path += ext;
        if (std::filesystem::exists(path))
            break;
    }

    return path;
}

void DeleteAsset(AssetData* a) {
    if (fs::exists(a->path.value))
        fs::remove(a->path.value);

    fs::path meta_path = fs::path(std::string(a->path) + ".meta");
    if (fs::exists(meta_path))
        fs::remove(meta_path);

    Free(a);
}

void SortAssets() {
    u32 asset_index = 0;
    for (u32 i=0; i<MAX_ASSETS; i++) {
        AssetData* a = GetAssetDataInternal(i);
        if (!a) continue;
        g_editor.assets[asset_index++] = i;
    }

    assert(asset_index == GetAssetCount());
}

fs::path GetTargetPath(AssetData* a) {
    std::string type_name_lower = ToString(a->type);
    Lower(type_name_lower.data(), (u32)type_name_lower.size());
    fs::path source_relative_path = fs::relative(a->path.value, g_editor.source_paths[a->asset_path_index].value);
    fs::path target_short_path = type_name_lower / GetSafeFilename(source_relative_path.filename().string().c_str());
    fs::path target_path = g_editor.output_path / target_short_path;
    target_path.replace_extension("");
    return target_path;
}

bool Rename(AssetData* a, const Name* new_name) {
    assert(a);
    assert(new_name);

    if (a->name == new_name)
        return true;

    // Check if another asset already exists with this name
    AssetData* existing = GetAssetData(ASSET_TYPE_UNKNOWN, new_name);
    if (existing && existing != a)
        return false;

    fs::path new_path = fs::path(a->path.value).parent_path() / (std::string(new_name->value) + fs::path(a->path.value).extension().string());
    if (fs::exists(new_path))
        return false;

    // Save old meta path BEFORE updating a->path
    fs::path old_meta_path = fs::path(std::string(a->path) + ".meta");

    try {
        fs::rename(a->path.value, new_path);
    } catch (...) {
        return false;
    }

    Set(a->path, new_path.string().c_str());
    a->name = new_name;

    fs::path new_meta_path = fs::path(new_path.string() + ".meta");
    if (fs::exists(old_meta_path)) {
        try {
            fs::rename(old_meta_path, new_meta_path);
        } catch (...) {
            // Meta file rename failed, but asset file was already renamed
            // Continue anyway since the main rename succeeded
        }
    }

    return true;
}

void NewAsset(AssetType asset_type, const Name* asset_name, const Vec2* position) {
    AssetData* a = nullptr;

    if (!asset_name) {
        Text default_name;
        Format(default_name, "new %s", GetAssetTypeInfo(asset_type)->short_name);
        Lower(default_name);
        std::filesystem::path new_path = GetUniqueAssetPath(std::filesystem::path(default_name.value));
        asset_name = GetName(new_path.stem().string().c_str());
    }

    if (asset_type == ASSET_TYPE_MESH)
        a = NewMeshData(asset_name->value);
    else if (asset_type == ASSET_TYPE_SKELETON)
        a = NewEditorSkeleton(asset_name->value);
    else if (asset_type == ASSET_TYPE_ANIMATION)
        a = NewAnimationData(asset_name->value);
    else if (asset_type == ASSET_TYPE_VFX)
        a = NewVfxData(asset_name->value);
    else if (asset_type == ASSET_TYPE_EVENT)
        a = NewEventData(asset_name->value);
    else if (asset_type == ASSET_TYPE_ATLAS)
        a = NewAtlasData(asset_name->value);

    if (a == nullptr)
        return;

    a->position = position ? *position : GetCenter(GetWorldBounds(g_view.camera));
    MarkModified(a);
    MarkMetaModified(a);

    if (a->vtable.post_load)
        a->vtable.post_load(a);

    SortAssets();
    SaveAssetData();

    ClearAssetSelection();
    SetSelected(a, true);
}

AssetData* Duplicate(AssetData* a) {
    fs::path new_path = GetUniqueAssetPath(a->path.value);

    AssetData* d = static_cast<AssetData*>(Alloc(g_editor.asset_allocator, sizeof(GenericAssetData)));
    Clone(d, a);
    Set(d->path, new_path.string().c_str());
    d->name = MakeCanonicalAssetName(new_path);
    d->selected = false;
    SortAssets();
    QueueImport(new_path);
    WaitForImportTasks();
    MarkModified(d);
    MarkMetaModified(d);
    return d;
}

std::filesystem::path GetUniqueAssetPath(const std::filesystem::path& path) {
    fs::path parent_path = path.parent_path();
    fs::path file_name = path.filename();
    fs::path ext = path.extension();
    file_name.replace_extension("");

    // Canonicalize the base name (lowercase, spaces to underscores, etc.)
    const Name* canonical_base = MakeCanonicalAssetName(file_name.string().c_str());
    fs::path candidate = parent_path / (std::string(canonical_base->value) + ext.string());

    for (int i = 0; ; i++) {
        if (i > 0) {
            candidate = parent_path / (std::string(canonical_base->value) + "_" + std::to_string(i) + ext.string());
        }

        // Check if file exists on disk
        if (!candidate.empty() && fs::exists(candidate))
            continue;

        // Check if asset with this canonical name already exists in memory
        const Name* canonical_name = MakeCanonicalAssetName(candidate);
        if (GetAssetData(ASSET_TYPE_UNKNOWN, canonical_name))
            continue;

        return candidate;
    }
}

int GetSelectedAssets(AssetData** out_assets, int max_assets) {
    int selected_count = 0;
    for (u32 i=0, c=GetAssetCount(); i<c && selected_count < max_assets; i++) {
        AssetData* a = GetAssetData(i);
        assert(a);
        if (!a->selected) continue;
        out_assets[selected_count++] = a;
    }

    return selected_count;
}