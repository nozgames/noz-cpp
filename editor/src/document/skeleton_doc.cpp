//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    extern void InitSkeletonEditor(SkeletonDocument* sdoc);

    static void BuildSkeletonDisplayMesh(SkeletonDocument* sdoc, const Vec2& position) {
        PushScratch();
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4096, 8192);

        float line_width =  STYLE_SKELETON_BONE_WIDTH * g_workspace.zoom_ref_scale;
        float bone_raius = STYLE_SKELETON_BONE_RADIUS * g_workspace.zoom_ref_scale;
        float dash_length = STYLE_SKELETON_PARENT_DASH * g_workspace.zoom_ref_scale;

        for (int bone_index = 0; bone_index < sdoc->bone_count; bone_index++) {
            BoneData* b = sdoc->bones + bone_index;
            Vec2 p0 = TransformPoint(b->local_to_world) + position;
            Vec2 p1 = TransformPoint(b->local_to_world, Vec2{b->length, 0}) + position;

            if (b->parent_index >= 0) {
                Mat3 parent_transform = GetParentLocalToWorld(sdoc, b, b->local_to_world);
                Vec2 pp = TransformPoint(parent_transform) + position;
                AddEditorDashedLine(builder, pp, p0, line_width, dash_length, STYLE_SKELETON_BONE_COLOR);
            }

            AddEditorBone(builder, p0, p1, line_width, STYLE_SKELETON_BONE_COLOR);
            AddEditorCircle(builder, p0, bone_raius, STYLE_SKELETON_BONE_COLOR);
        }

        if (!sdoc->display_mesh)
            sdoc->display_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        else
            UpdateMesh(builder, sdoc->display_mesh);

        PopScratch();
        sdoc->display_mesh_dirty = false;
        sdoc->display_mesh_zoom_version = g_workspace.zoom_version;
        sdoc->display_mesh_position = position;
    }

    void DrawSkeletonDocument(SkeletonDocument* sdoc, const Vec2& position) {
        BindIdentitySkeleton();
        BindColor(COLOR_WHITE);
        BindDepth(0.0);
        Mat3 local_to_world = Translate(sdoc->position);
        for (int i=0; i<sdoc->skin_count; i++) {
            MeshDocument* skinned_mesh = sdoc->skins[i].mesh;
            if (!skinned_mesh)
                continue;
            DrawMesh(skinned_mesh, local_to_world, g_workspace.shaded_skinned_material);
        }

        bool zoom_changed = sdoc->display_mesh_zoom_version != g_workspace.zoom_version;
        bool position_changed = sdoc->display_mesh_position != position;
        if (sdoc->display_mesh_dirty || !sdoc->display_mesh || zoom_changed || position_changed)
            BuildSkeletonDisplayMesh(sdoc, position);

        BindDepth(0.0f);
        BindMaterial(g_workspace.editor_material);
        BindTransform(MAT3_IDENTITY);
        DrawMesh(sdoc->display_mesh);
    }

    static void DrawSkeletonData(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
        DrawSkeletonDocument(sdoc, sdoc->position);
    }

    int HitTestBones(SkeletonDocument* sdoc, const Mat3& transform, const Vec2& position, int* bones, int max_bones) {
        int hit_count = 0;
        for (int bone_index=sdoc->bone_count-1; bone_index>=0 && max_bones > 0; bone_index--) {
            BoneData* b = sdoc->bones + bone_index;
            Mat3 local_to_world = transform * b->local_to_world;
            if (!OverlapPoint(g_workspace.bone_collider, local_to_world * Scale(b->length), position))
                continue;

            bones[hit_count++] = bone_index;
            max_bones--;
        }

        return hit_count;
    }

    int HitTestBone(SkeletonDocument* sdoc, const Mat3& transform, const Vec2& position) {
        int bones[MAX_BONES];
        int bone_count = HitTestBones(sdoc, transform, position, bones, MAX_BONES);
        if (bone_count == 0)
            return -1;

        float best_dist = F32_MAX;
        int best_bone_index = -1;
        for (int bone_index=0; bone_index<bone_count; bone_index++) {
            BoneData* b = sdoc->bones + bones[bone_index];
            Mat3 local_to_world = transform * b->local_to_world;
            Vec2 b0 = TransformPoint(local_to_world);
            Vec2 b1 = TransformPoint(local_to_world, {b->length, 0});
            float dist = DistanceFromLine(b0, b1, position);
            if (dist < best_dist) {
                best_dist = dist;
                best_bone_index = bones[bone_index];
            }
        }

        return best_bone_index;
    }

    int HitTestBone(SkeletonDocument* sdoc, const Vec2& world_pos) {
        return HitTestBone(sdoc, Translate(sdoc->position), world_pos);
    }

    static void ParseBonePosition(BoneData& eb, Tokenizer& tk)
    {
        float x;
        if (!ExpectFloat(tk, &x))
            ThrowError("misssing 'x' in bone position");
        float y;
        if (!ExpectFloat(tk, &y))
            ThrowError("misssing 'y' in bone position");

        eb.transform.position = {x,y};
    }

    static void ParseBoneRotation(BoneData& eb, Tokenizer& tk) {
        float r;
        if (!ExpectFloat(tk, &r))
            ThrowError("misssing bone rotation value");

        eb.transform.rotation = r;
    }

    static void ParseBoneLength(BoneData& eb, Tokenizer& tk) {
        float l;
        if (!ExpectFloat(tk, &l))
            ThrowError("misssing bone length value");

        eb.length = l;
    }

    static void ParseBone(SkeletonDocument* sdoc, Tokenizer& tk) {
        if (!ExpectQuotedString(tk))
            ThrowError("expected bone name as quoted string");

        const Name* bone_name = GetName(tk);

        int parent_index = -1;
        if (!ExpectInt(tk, &parent_index))
            ThrowError("expected parent index");

        BoneData& bone = sdoc->bones[sdoc->bone_count++];
        bone.name = bone_name;
        bone.parent_index = parent_index;
        bone.index = sdoc->bone_count - 1;
        bone.transform.scale = VEC2_ONE;
        bone.length = 0.25f;

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "p"))
                ParseBonePosition(bone, tk);
            else if (ExpectIdentifier(tk, "r"))
                ParseBoneRotation(bone, tk);
            else if (ExpectIdentifier(tk, "l"))
                ParseBoneLength(bone, tk);
            else
                break;
        }
    }

    void LoadSkeletonDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);

        std::filesystem::path path = sdoc->path.value;
        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
        Tokenizer tk;
        Init(tk, contents.c_str());

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "b")) {
                ParseBone(sdoc, tk);
            } else {
                char error[1024];
                GetString(tk, error, sizeof(error) - 1);
                ThrowError("unknown identifier '%s' in skeleton", error);
            }
        }

        UpdateTransforms(sdoc);
    }

    static void SaveSkeletonData(Document* doc, const std::filesystem::path& path) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

        for (int i=0; i<sdoc->bone_count; i++) {
            const BoneData& eb = sdoc->bones[i];
            WriteCSTR(stream, "b \"%s\" %d p %f %f r %f l %f\n",
                eb.name->value,
                eb.parent_index,
                eb.transform.position.x,
                eb.transform.position.y,
                eb.transform.rotation,
                eb.length);
        }

        SaveStream(stream, path);
        Free(stream);
    }

    Document* NewSkeletonDocument(const std::filesystem::path& path) {
        const char* default_skel = "b \"root\" -1 p 0 0 l 1\n";

        std::filesystem::path full_path = path.is_relative() ? std::filesystem::path(g_editor.project_path) / "assets" / "skeletons" / path : path;
        full_path += ".skel";

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        WriteCSTR(stream, default_skel);
        SaveStream(stream, full_path);
        Free(stream);

        Document* doc = CreateDocument(full_path);
        if (doc && doc->vtable.load)
            doc->vtable.load(doc);
        return doc;
    }

    void UpdateTransforms(SkeletonDocument* sdoc) {
        if (sdoc->bone_count <= 0)
            return;

        BoneData& root = sdoc->bones[0];
        root.local_to_world = TRS(root.transform.position, root.transform.rotation, VEC2_ONE);
        root.world_to_local = Inverse(root.local_to_world);

        for (int bone_index=1; bone_index<sdoc->bone_count; bone_index++) {
            BoneData& bone = sdoc->bones[bone_index];
            BoneData& parent = sdoc->bones[bone.parent_index];
            bone.local_to_world = parent.local_to_world * TRS(bone.transform.position, bone.transform.rotation, VEC2_ONE);
            bone.world_to_local = Inverse(bone.local_to_world);
        }

        Vec2 root_position = TransformPoint(sdoc->bones[0].local_to_world);
        Bounds2 bounds = Bounds2 { root_position, root_position };
        for (int i=0; i<sdoc->bone_count; i++) {
            BoneData* b = sdoc->bones + i;
            float bone_width = b->length * BONE_WIDTH;
            const Mat3& bone_transform = b->local_to_world;
            bounds = Union(bounds, TransformPoint(bone_transform));
            bounds = Union(bounds, TransformPoint(bone_transform, Vec2{b->length, 0}));
            bounds = Union(bounds, TransformPoint(bone_transform, Vec2{bone_width, bone_width}));
            bounds = Union(bounds, TransformPoint(bone_transform, Vec2{bone_width, -bone_width}));
        }

        for (int i=0; i<sdoc->skin_count; i++) {
            MeshDocument* skinned_mesh = sdoc->skins[i].mesh;
            if (!skinned_mesh || skinned_mesh->def->type != ASSET_TYPE_MESH)
                continue;

            bounds = Union(bounds, GetBounds(skinned_mesh));
        }

        sdoc->bounds = Expand(bounds, BOUNDS_PADDING);
        sdoc->display_mesh_dirty = true;
    }

    static void LoadSkeletonDocumentMeta(Document* doc, Props* meta) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);

        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
        for (auto& key : meta->GetKeys("skin")) {
            sdoc->skins[sdoc->skin_count++] = {.asset_name = GetName(key.c_str())};
        }
    }

    static void PostLoadSkeletonDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);

        int loaded_skinned_mesh = 0;
        for (int i=0; i<sdoc->skin_count; i++) {
            Skin& sm = sdoc->skins[i];
            sm.mesh = static_cast<MeshDocument*>(FindDocument(ASSET_TYPE_MESH, sm.asset_name));
            if (!sm.mesh)
                continue;
            sdoc->skins[loaded_skinned_mesh++] = sm;
            PostLoadDocument(sm.mesh);
        }

        sdoc->skin_count = loaded_skinned_mesh;

        UpdateTransforms(sdoc);
    }

    int FindBoneIndex(SkeletonDocument* sdoc, const Name* name) {
        for (int i=0; i<sdoc->bone_count; i++)
            if (sdoc->bones[i].name == name)
                return i;

        return -1;
    }

    static int CompareBoneParentIndex(void const* p, void const* arg) {
        BoneData* a = (BoneData*)p;
        BoneData* b = (BoneData*)arg;
        return a->parent_index - b->parent_index;
    }

    static void ReparentBoneTransform(BoneData& b, BoneData& p) {
        Mat3 new_local = p.world_to_local * b.local_to_world;

        b.transform.position.x = new_local.m[6];
        b.transform.position.y = new_local.m[7];

        // Scale (magnitude of basis vectors)
        f32 scale_x = Sqrt(
            new_local.m[0] * new_local.m[0] +
            new_local.m[1] * new_local.m[1]);
        f32 scale_y = Sqrt(
            new_local.m[3] * new_local.m[3] +
            new_local.m[4] * new_local.m[4]);

        b.transform.scale = Vec2(scale_x, scale_y);
        b.transform.rotation = Degrees(
            Atan2(
                new_local.m[1] / scale_x,
                new_local.m[0] / scale_x));
    }

    int ReparentBone(SkeletonDocument* sdoc, int bone_index, int parent_index) {
        BoneData& eb = sdoc->bones[bone_index];

        eb.parent_index = parent_index;

        qsort(sdoc->bones, sdoc->bone_count, sizeof(BoneData), CompareBoneParentIndex);

        int bone_map[MAX_BONES];
        for (int i=0; i<sdoc->bone_count; i++)
            bone_map[sdoc->bones[i].index] = i;

        for (int i=1; i<sdoc->bone_count; i++) {
            sdoc->bones[i].parent_index = bone_map[sdoc->bones[i].parent_index];
            sdoc->bones[i].index = i;
        }

        ReparentBoneTransform(sdoc->bones[bone_map[bone_index]], sdoc->bones[bone_map[parent_index]]);
        UpdateTransforms(sdoc);

        return bone_map[bone_index];
    }

    // Returns: -1 = left, 1 = right, 0 = center
    // Sets is_prefix to true if l_/r_ prefix, false if _l/_r suffix
    static int GetBoneSideInternal(const char* str, size_t len, bool* is_prefix) {
        // Check prefix first (l_ or r_)
        if (len >= 2) {
            if (strncmp(str, "l_", 2) == 0) {
                *is_prefix = true;
                return -1;
            }
            if (strncmp(str, "r_", 2) == 0) {
                *is_prefix = true;
                return 1;
            }
        }

        // Check suffix (_l or _r)
        if (len >= 2) {
            const char* suffix = &str[len - 2];
            if (strcmp(suffix, "_l") == 0) {
                *is_prefix = false;
                return -1;
            }
            if (strcmp(suffix, "_r") == 0) {
                *is_prefix = false;
                return 1;
            }
        }

        *is_prefix = false;
        return 0;
    }

    int GetBoneSide(SkeletonDocument* sdoc, int bone_index) {
        const char* str = sdoc->bones[bone_index].name->value;
        bool is_prefix;
        return GetBoneSideInternal(str, strlen(str), &is_prefix);
    }

    int GetMirrorBone(SkeletonDocument* sdoc, int bone_index) {
        const char* name_a = sdoc->bones[bone_index].name->value;
        size_t len_a = strlen(name_a);
        bool is_prefix_a;
        int side = GetBoneSideInternal(name_a, len_a, &is_prefix_a);

        if (side == 0)
            return -1;

        for (int i = 0; i < sdoc->bone_count; i++) {
            if (i == bone_index)
                continue;

            const char* name_b = sdoc->bones[i].name->value;
            size_t len_b = strlen(name_b);
            bool is_prefix_b;
            int other_side = GetBoneSideInternal(name_b, len_b, &is_prefix_b);

            if (other_side != -side)
                continue;

            // Both must use same convention (both prefix or both suffix)
            if (is_prefix_a != is_prefix_b)
                continue;

            if (len_a != len_b)
                continue;

            // Compare base name (skip the 2-char prefix or suffix)
            if (is_prefix_a) {
                if (strcmp(name_a + 2, name_b + 2) == 0)
                    return i;
            } else {
                if (strncmp(name_a, name_b, len_a - 2) == 0)
                    return i;
            }
        }

        return -1;
    }

    void RemoveBone(SkeletonDocument* sdoc, int bone_index) {
        if (bone_index <= 0 || bone_index >= sdoc->bone_count)
            return;

        BoneData& b = sdoc->bones[bone_index];
        int parent_index = b.parent_index;

        // Reparent children to parent
        for (int child_index=0; child_index < sdoc->bone_count; child_index++) {
            BoneData& c = sdoc->bones[child_index];
            if (c.parent_index == bone_index) {
                c.parent_index = parent_index;
                ReparentBoneTransform(c, sdoc->bones[parent_index]);
            }
        }

        sdoc->bone_count--;

        for (int i=bone_index; i<sdoc->bone_count; i++) {
            BoneData& enb = sdoc->bones[i];
            enb = sdoc->bones[i + 1];
            enb.index = i;
            if (enb.parent_index == bone_index)
                enb.parent_index = parent_index;
            else if (enb.parent_index > bone_index)
                enb.parent_index--;
        }

        UpdateTransforms(sdoc);
    }

    const Name* GetUniqueBoneName(SkeletonDocument* sdoc) {
        const Name* bone_name = GetName("Bone");

        int bone_postfix = 2;
        while (FindBoneIndex(sdoc, bone_name) != -1)
        {
            char name[64];
            Format(name, sizeof(name), "Bone%d", bone_postfix++);
            bone_name = GetName(name);
        }

        return bone_name;
    }

    void Serialize(SkeletonDocument* sdoc, Stream* stream) {
        const Name* bone_names[MAX_BONES];
        for (int i=0; i<sdoc->bone_count; i++)
            bone_names[i] = sdoc->bones[i].name;

        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_SKELETON;
        header.version = 1;
        header.flags = 0;
        header.names = sdoc->bone_count;
        WriteAssetHeader(stream, &header, bone_names);

        WriteU8(stream, (u8)sdoc->bone_count);

        for (int i=0; i<sdoc->bone_count; i++) {
            BoneData& eb = sdoc->bones[i];
            WriteI8(stream, (char)eb.parent_index);
            WriteStruct(stream, eb.transform.position);
            WriteFloat(stream, eb.transform.rotation);
            WriteStruct(stream, eb.transform.scale);
            WriteStruct(stream, eb.world_to_local);
        }
    }

    Skeleton* ToSkeleton(Allocator* allocator, SkeletonDocument* sdoc) {
        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 8192);
        if (!stream)
            return nullptr;
        Serialize(sdoc, stream);
        SeekBegin(stream, 0);

        Skeleton* skeleton = static_cast<Skeleton*>(LoadAssetInternal(allocator, sdoc->name, ASSET_TYPE_SKELETON, LoadSkeleton, stream));
        Free(stream);

        return skeleton;
    }

    static void SaveSkeletonMetadata(Document* doc, Props* meta) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
        meta->ClearGroup("skin");

        for (int i=0; i<sdoc->skin_count; i++) {
            if (sdoc->skins[i].mesh == nullptr)
                continue;

            meta->AddKey("skin", sdoc->skins[i].asset_name->value);
        }
    }

    static void SkeletonUndoRedo(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
        UpdateTransforms(sdoc);

        extern void UpdateTransforms(AnimationDocument* docdoc, int frame_index=-1);
        for (u32 i = 0, c = GetDocumentCount(); i < c; i++) {
            AnimationDocument* adoc = static_cast<AnimationDocument*>(GetDocument(i));
            if (adoc->def->type != ASSET_TYPE_ANIMATION) continue;
            if (adoc->skeleton != sdoc) continue;
            UpdateTransforms(adoc);
        }
    }

    static void CloneSkeletonDocument(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
        sdoc->display_mesh = nullptr;
    }

    static void DestroySkeletonDocument(Document* doc) {
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);
        assert(sdoc);
        Free(sdoc->display_mesh);
        sdoc->display_mesh = nullptr;
    }

    static void ImportSkeleton(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        (void)config;
        (void)meta;

        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(doc);

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        Serialize(sdoc, stream);
        SaveStream(stream, path);
        Free(stream);
    }

    static void InitSkeletonData(SkeletonDocument* sdoc) {
        assert(sdoc);
        sdoc->vtable = {
            .destructor = DestroySkeletonDocument,
            .load = LoadSkeletonDocument,
            .post_load = PostLoadSkeletonDocument,
            .save = SaveSkeletonData,
            .load_metadata = LoadSkeletonDocumentMeta,
            .save_metadata = SaveSkeletonMetadata,
            .draw = DrawSkeletonData,
            .clone = CloneSkeletonDocument,
            .undo_redo = SkeletonUndoRedo
        };

        InitSkeletonEditor(sdoc);
    }

    static void InitSkeletonDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SKELETON);
        InitSkeletonData(static_cast<SkeletonDocument*>(doc));
    }

    void InitSkeletonDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_SKELETON,
            .size = sizeof(SkeletonDocument),
            .ext = ".skel",
            .init_func = InitSkeletonDocument,
            .import_func = ImportSkeleton
        });
    }
}
