//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "document/atlas_manager.h"

namespace fs = std::filesystem;

namespace noz::editor {

    const Name* MakeCanonicalAssetName(const fs::path& path) {
        return MakeCanonicalAssetName(fs::path(path).replace_extension("").filename().string().c_str());
    }

    const Name* MakeCanonicalAssetName(const char* name) {
        std::string result = name;
        Lower(result.data(), (u32)result.size());
        Replace(result.data(), (u32)result.size(), '/', '_');
        Replace(result.data(), (u32)result.size(), '.', '_');
        Replace(result.data(), (u32)result.size(), ' ', '_');
        Replace(result.data(), (u32)result.size(), '-', '_');
        return GetName(result.c_str());
    }

    bool IsFile(Document* a) {
        return a->path.length > 0;
    }

    static void DestroyDocument(void* p) {
        Document* a = static_cast<Document*>(p);
        if (a->vtable.destructor) {
            a->vtable.destructor(a);
        }
    }

    Document* CreateDocument(const std::filesystem::path& path) {
        const DocumentDef* doc_def = FindDocumentDef(path.extension().string().c_str());
        if (!doc_def)
            return nullptr;

        Document* doc = static_cast<Document*>(Alloc(ALLOCATOR_DEFAULT, doc_def->size, DestroyDocument));
        Set(doc->path, canonical(path).string().c_str());
        Lower(doc->path);
        doc->def = doc_def;
        doc->name = MakeCanonicalAssetName(path);
        doc->bounds = Bounds2{{-0.5f, -0.5f}, {0.5f, 0.5f}};
        doc->asset_path_index = -1;

        for (int i=0; i<g_editor.source_path_count; i++) {
            if (Equals(g_editor.source_paths[i].value, doc->path, g_editor.source_paths[i].length, true)) {
                doc->asset_path_index = i;
                break;
            }
        }

        assert(doc->asset_path_index != -1);

        if (doc_def->init_func)
            doc_def->init_func(doc);

        return doc;
    }

    static void LoadAssetMetadata(Document* ea, const std::filesystem::path& path) {
        Props* props = LoadProps(std::filesystem::path(path.string() + ".meta"));
        if (!props)
            return;

        ea->position = props->GetVec2("editor", "position", VEC2_ZERO);

        if (ea->vtable.load_metadata)
            ea->vtable.load_metadata(ea, props);
    }

    static void SaveAssetMetadata(Document* a) {
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
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (!a->modified && !a->meta_modified)
                continue;

            SaveAssetMetadata(a);

            a->meta_modified= false;
        }
    }

    void SetPosition(Document* a, const Vec2& position) {
        a->position = position;
        a->meta_modified = true;
    }

    void DrawFaceCenters(MeshDocument* m, const Vec2& position) {
        BindMaterial(g_workspace.vertex_material);
        MeshFrameData* frame = GetCurrentFrame(m);
        for (u16 fi=0; fi<frame->geom.face_count; fi++) {
            const FaceData* f = GetFace(frame, fi);
            BindColor(IsSelected(f) ? COLOR_VERTEX_SELECTED : COLOR_VERTEX);
            DrawVertex(position + GetFaceCenter(m, fi));
        }
    }

    void SaveDocuments() {
        // Update atlases for any dirty meshes before saving
        UpdateDirtyMeshAtlases();

        SaveAssetMetadata();

        u32 count = 0;
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);;
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

    bool OverlapPoint(Document* a, const Vec2& overlap_point) {
        return Contains(a->bounds + a->position, overlap_point);
    }

    bool OverlapPoint(Document* a, const Vec2& position, const Vec2& overlap_point) {
        return Contains(a->bounds + position, overlap_point);
    }

    bool OverlapBounds(Document* a, const Bounds2& bounds) {
        return Intersects(a->bounds + a->position, bounds);
    }

    Document* HitTestAssets(const Vec2& overlap_point) {
        Document* first_hit = nullptr;
        for (u32 i=GetDocumentCount(); i>0; i--) {
            Document* a = GetDocument(i-1);
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

    Document* HitTestAssets(const Bounds2& hit_bounds) {
        Document* first_hit = nullptr;
        for (u32 i=GetDocumentCount(); i>0; i--) {
            Document* a = GetDocument(i-1);
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

    void DrawAsset(Document* a) {
        BindDepth(0.0f);
        if (a->vtable.draw)
            a->vtable.draw(a);
    }

    Document* GetFirstSelectedAsset() {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (a->selected)
                return a;
        }

        return nullptr;
    }

    void ClearAssetSelection() {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            a->selected = false;
        }

        g_workspace.selected_asset_count = 0;
    }

    void SetSelected(Document* a, bool selected) {
        assert(a);
        if (a->selected == selected)
            return;
        a->selected = true;
        g_workspace.selected_asset_count++;
    }

    void ToggleSelected(Document* a) {
        assert(a);
        a->selected = !a->selected;
        if (a->selected)
            g_workspace.selected_asset_count++;
        else
            g_workspace.selected_asset_count--;
    }

    Document* FindDocument(AssetType type, const Name* name) {
        for (int doc_index = 0; doc_index < GetDocumentCount(); doc_index++) {
            Document* doc = GetDocument(doc_index);
            if (doc && (type == ASSET_TYPE_UNKNOWN || doc->def->type == type) && doc->name == name)
                return doc;
        }

        return nullptr;
    }

    Document* Clone(Document* doc) {
        Document* clone = CreateDocument(doc->path.value);
        memcpy(clone, doc, doc->def->size);

        if (clone->vtable.clone)
            clone->vtable.clone(doc);

        return clone;
    }

    void CloneInto(Document* dst, Document* src) {
        bool editing = dst->editing;
        memcpy(dst, src, src->def->size);
        dst->editing = editing;

        if (dst->vtable.clone)
            dst->vtable.clone(dst);
    }

    Document* CreateAssetDataForImport(const std::filesystem::path& path) {
        Document* a = CreateDocument(path);
        if (!a)
            return nullptr;

        LoadAssetMetadata(a, path);
        // LoadAssetData(a);
        // PostLoadAssetData(a);
        //SortAssets();

        return a;
    }

    void InitDocument() {
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
                Document* existing = FindDocument(ASSET_TYPE_UNKNOWN, asset_name);
                if (existing) {
                    if (strcmp(asset_name->value, "palette") == 0)
                        LogInfo("[AssetData] SKIPPING duplicate palette: %s (source_path_index=%d)", asset_path.string().c_str(), i);
                    continue;
                }

                Document* a = nullptr;
                for (int asset_type=0; !a && asset_type<ASSET_TYPE_COUNT; asset_type++)
                    a = CreateDocument(asset_path);

                if (a)
                    LoadAssetMetadata(a, asset_path);
            }
        }
    }

    void LoadDocument(Document* a) {
        assert(a);

        if (a->loaded)
            return;

        a->loaded = true;

        if (a->vtable.load)
            a->vtable.load(a);
    }

    void PostLoadDocument(Document* a) {
        assert(a);
        assert(a->loaded);

        if (a->post_loaded)
            return;

        if (a->vtable.post_load)
            a->vtable.post_load(a);

        a->post_loaded = true;
    }

    void LoadDocuments() {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            Document* a = GetDocument(i);
            assert(a);
            LoadDocument(a);
        }
    }

    void PostLoadDocuments() {
        for (u32 i=0, c=GetDocumentCount(); i<c; i++) {
            PostLoadDocument(GetDocument(i));
        }
    }

    void HotloadEditorAsset(AssetType type, const Name* name){
        Document* doc = FindDocument(type, name);
        if (doc != nullptr && doc->vtable.reload)
            doc->vtable.reload(doc);
    }

    void MarkModified(Document* doc) {
        doc->modified = true;
    }

    void MarkMetaModified(Document* doc) {
        doc->meta_modified = true;
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

    void DeleteAsset(Document* a) {
        if (fs::exists(a->path.value))
            fs::remove(a->path.value);

        fs::path meta_path = fs::path(std::string(a->path) + ".meta");
        if (fs::exists(meta_path))
            fs::remove(meta_path);

        Free(a);
    }

    fs::path GetTargetPath(Document* doc) {
        std::string type_name_lower = ToString(doc->def->type);
        Lower(type_name_lower.data(), (u32)type_name_lower.size());
        fs::path source_relative_path = fs::relative(doc->path.value, g_editor.source_paths[doc->asset_path_index].value);
        fs::path target_short_path = type_name_lower / GetSafeFilename(source_relative_path.filename().string().c_str());
        fs::path target_path = g_editor.output_path / target_short_path;
        target_path.replace_extension("");
        return target_path;
    }

    bool Rename(Document* doc, const Name* new_name) {
        assert(doc);
        assert(new_name);

        if (doc->name == new_name)
            return true;

        // Check if another asset already exists with this name
        Document* existing = FindDocument(ASSET_TYPE_UNKNOWN, new_name);
        if (existing && existing != doc)
            return false;

        fs::path new_path = fs::path(doc->path.value).parent_path() / (std::string(new_name->value) + fs::path(doc->path.value).extension().string());
        if (fs::exists(new_path))
            return false;

        // Save old meta path BEFORE updating a->path
        fs::path old_meta_path = fs::path(std::string(doc->path) + ".meta");

        // Let asset type handle pre-rename logic (e.g., mesh updating its atlas)
        if (doc->vtable.editor_rename)
            doc->vtable.editor_rename(doc, new_name);

        try {
            fs::rename(doc->path.value, new_path);
        } catch (...) {
            return false;
        }

        Set(doc->path, new_path.string().c_str());
        doc->name = new_name;

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
        Document* a = nullptr;

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
            a = NewSkeletonDocument(asset_name->value);
        else if (asset_type == ASSET_TYPE_ANIMATION)
            a = NewAnimationDocument(asset_name->value);
        else if (asset_type == ASSET_TYPE_VFX)
            a = NewVfxDocument(asset_name->value);
        else if (asset_type == ASSET_TYPE_EVENT)
            a = NewEventDocument(asset_name->value);
        else if (asset_type == ASSET_TYPE_ATLAS)
            a = NewAtlasDocument(asset_name->value);

        if (a == nullptr)
            return;

        a->position = position ? *position : GetCenter(GetWorldBounds(g_workspace.camera));
        MarkModified(a);
        MarkMetaModified(a);

        if (a->vtable.post_load)
            a->vtable.post_load(a);

        SaveDocuments();

        ClearAssetSelection();
        SetSelected(a, true);
    }

    Document* Duplicate(Document* adoc) {
        fs::path new_path = GetUniqueAssetPath(adoc->path.value);

        Document* d = Clone(adoc);
        Set(d->path, new_path.string().c_str());
        d->name = MakeCanonicalAssetName(new_path);
        d->selected = false;
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
            if (FindDocument(ASSET_TYPE_UNKNOWN, canonical_name))
                continue;

            return candidate;
        }
    }

    int GetSelectedAssets(Document** out_assets, int max_assets) {
        int selected_count = 0;
        for (u32 i=0, c=GetDocumentCount(); i<c && selected_count < max_assets; i++) {
            Document* a = GetDocument(i);
            assert(a);
            if (!a->selected) continue;
            out_assets[selected_count++] = a;
        }

        return selected_count;
    }
}
