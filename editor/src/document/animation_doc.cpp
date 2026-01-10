//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//


namespace noz::editor {

    static void InitAnimationDocument(AnimationDocument* adoc);
    extern void InitAnimationEditor(AnimationDocument* adoc);
    extern void LoadSkeletonDocument(Document* sdoc);

    inline SkeletonDocument* GetSkeletonDocument(AnimationDocument* adoc) { return adoc->skeleton; }

    int GetRealFrameIndex(AnimationDocument* adoc, int frame_index) {
        for (int i=0; i<adoc->frame_count; i++)
            for (int h=0; h<=adoc->frames[i].hold; h++, frame_index--)
                if (frame_index == 0) return i;

        return adoc->frame_count;
    }

    int GetFrameIndexWithHolds(AnimationDocument* adoc, int frame_index) {
        int frame_index_with_holds = 0;
        for (int i=0; i<frame_index; i++) {
            frame_index_with_holds++;
            frame_index_with_holds += adoc->frames[i].hold;
        }
        return frame_index_with_holds;
    }

    int GetFrameCountWithHolds(AnimationDocument* adoc) {
        int frame_count = 0;
        for (int frame_index=0; frame_index<adoc->frame_count; frame_index++) {
            frame_count++;
            frame_count += adoc->frames[frame_index].hold;
        }
        return frame_count;
    }

    void UpdateTransforms(AnimationDocument* adoc, int frame_index) {
        if (frame_index == -1)
            frame_index = adoc->current_frame;

        SkeletonDocument* sdoc = GetSkeletonDocument(adoc);
        for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++) {
            BoneData* b = &sdoc->bones[bone_index];
            Transform& frame = GetFrameTransform(adoc, bone_index, frame_index);

            adoc->animator.bones[bone_index] = TRS(
                b->transform.position + frame.position,
                b->transform.rotation + frame.rotation,
                b->transform.scale);
        }

        for (int bone_index=1; bone_index<sdoc->bone_count; bone_index++)
            adoc->animator.bones[bone_index] =
                adoc->animator.bones[sdoc->bones[bone_index].parent_index] * adoc->animator.bones[bone_index];
    }

    void DrawAnimationData(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        SkeletonDocument* sdoc = GetSkeletonDocument(adoc);
        if (!sdoc)
            return;

        Mat3 transform = Translate(doc->position);

        if (!doc->editing)
            transform = Translate(-GetFrameTransform(adoc, 0, adoc->current_frame).position) * transform;

        BindColor(COLOR_WHITE);
        BindSkeleton(&sdoc->bones[0].world_to_local, sizeof(BoneData), adoc->animator.bones, 0, sdoc->bone_count);
        for (int i=0; i<sdoc->skin_count; i++) {
            MeshDocument* skinned_mesh = sdoc->skins[i].mesh;
            if (!skinned_mesh || skinned_mesh->def->type != ASSET_TYPE_MESH)
                continue;

            DrawMesh(skinned_mesh, transform, g_workspace.shaded_skinned_material);
        }
    }

    static void ParseSkeletonBone(Tokenizer& tk, SkeletonDocument* es, int bone_index, int* bone_map) {
        if (!ExpectQuotedString(tk))
            throw std::exception("missing quoted bone name");

        bone_map[bone_index] = FindBoneIndex(es, GetName(tk));
    }

    void UpdateSkeleton(AnimationDocument* adoc) {
        SkeletonDocument* s = GetSkeletonDocument(adoc);

        int bone_map[MAX_BONES];
        for (int i=0; i<MAX_BONES; i++)
            bone_map[i] = -1;

        for (int i=0; i<adoc->bone_count; i++) {
            int new_bone_index = FindBoneIndex(s, adoc->bones[i].name);
            if (new_bone_index == -1)
                continue;
            bone_map[new_bone_index] = i;
        }

        AnimationFrameData new_frames[MAX_ANIMATION_FRAMES];
        for (int frame_index=0; frame_index<adoc->frame_count; frame_index++)
            new_frames[frame_index] = adoc->frames[frame_index];

        memcpy(adoc->frames, new_frames, sizeof(new_frames));

        for (int i=0; i<adoc->bone_count; i++) {
            AnimationBoneData& ab = adoc->bones[i];
            ab.index = i;
            ab.name = adoc->bones[i].name;
        }

        adoc->bone_count = adoc->bone_count;

        UpdateBounds(adoc);
        UpdateTransforms(adoc);
    }

    static void ParseSkeleton(AnimationDocument* adoc, Tokenizer& tk, int* bone_map) {
        if (!ExpectQuotedString(tk))
            throw std::exception("missing quoted skeleton name");

        adoc->skeleton_name = GetName(tk);

        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(FindDocument(ASSET_TYPE_SKELETON, adoc->skeleton_name));
        if (!sdoc) return;

        if (!sdoc->loaded)
            LoadSkeletonDocument(sdoc);

        for (int i=0; i<adoc->bone_count; i++) {
            AnimationBoneData& enb = adoc->bones[i];
            BoneData& eb = sdoc->bones[i];
            enb.name = eb.name;
            enb.index = i;
        }

        adoc->bone_count = sdoc->bone_count;

        for (int frame_index=0; frame_index<MAX_ANIMATION_FRAMES; frame_index++)
            for (int bone_index=0; bone_index<MAX_BONES; bone_index++)
                SetIdentity(adoc->frames[frame_index].transforms[bone_index]);

        int bone_index = 0;
        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "b"))
                ParseSkeletonBone(tk, sdoc, bone_index++, bone_map);
            else
                break;
        }
    }

    static int ParseFrameBone(AnimationDocument* a, Tokenizer& tk, int* bone_map) {
        (void)a;
        int bone_index;
        if (!ExpectInt(tk, &bone_index))
            ThrowError("expected bone index");

        return bone_map[bone_index];
    }

    static void ParseFramePosition(AnimationDocument* adoc, Tokenizer& tk, int bone_index, int frame_index) {
        float x;
        if (!ExpectFloat(tk, &x))
            ThrowError("expected position 'x' value");
        float y;
        if (!ExpectFloat(tk, &y))
            ThrowError("expected position 'y' value");

        if (bone_index == -1)
            return;

        SetPosition(GetFrameTransform(adoc, bone_index, frame_index), {x,y});
    }

    static void ParseFrameHold(AnimationDocument* adoc, Tokenizer& tk, int frame_index) {
        int hold;
        if (!ExpectInt(tk, &hold))
            ThrowError("expected hold value");

        adoc->frames[frame_index].hold = Max(0, hold);
    }

    static void ParseFrameRotation(AnimationDocument* adoc, Tokenizer& tk, int bone_index, int frame_index) {
        float r;
        if (!ExpectFloat(tk, &r))
            ThrowError("expected rotation value");

        if (bone_index == -1)
            return;

        SetRotation(GetFrameTransform(adoc, bone_index, frame_index), r);
    }

    static void ParseFrameScale(AnimationDocument* adoc, Tokenizer& tk, int bone_index, int frame_index) {
        float s;
        if (!ExpectFloat(tk, &s))
            ThrowError("expected scale value");

        if (bone_index == -1)
            return;

        SetScale(GetFrameTransform(adoc, bone_index, frame_index), s);
    }

    static void ParseFrameEvent(AnimationDocument* adoc, Tokenizer& tk, int frame_index) {
        if (!ExpectQuotedString(tk))
            ThrowError("expected event name");

        adoc->frames[frame_index].event_name = GetName(tk);
    }

    static void ParseFrame(AnimationDocument* adoc, Tokenizer& tk, int* bone_map) {
        int bone_index = -1;
        adoc->frame_count++;
        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "b"))
                bone_index = ParseFrameBone(adoc, tk, bone_map);
            else if (ExpectIdentifier(tk, "e"))
                ParseFrameEvent(adoc, tk, adoc->frame_count - 1);
            else if (ExpectIdentifier(tk, "r"))
                ParseFrameRotation(adoc, tk, bone_index, adoc->frame_count - 1);
            else if (ExpectIdentifier(tk, "s"))
                ParseFrameScale(adoc, tk, bone_index, adoc->frame_count - 1);
            else if (ExpectIdentifier(tk, "p"))
                ParseFramePosition(adoc, tk, bone_index, adoc->frame_count - 1);
            else if (ExpectIdentifier(tk, "h"))
                ParseFrameHold(adoc, tk, adoc->frame_count - 1);
            else
                break;
        }
    }

    void UpdateBounds(AnimationDocument* adoc) {
        adoc->bounds = GetSkeletonDocument(adoc)->bounds;

        SkeletonDocument* sdoc = GetSkeletonDocument(adoc);
        Vec2 root_position = TransformPoint(adoc->animator.bones[0]);
        Bounds2 bounds = Bounds2 { root_position, root_position };
        for (int bone_index=0; bone_index<adoc->bone_count; bone_index++) {
            BoneData* b = sdoc->bones + bone_index;
            float bone_width = b->length * BONE_WIDTH;
            const Mat3& bone_transform = adoc->animator.bones[bone_index];
            bounds = Union(bounds, TransformPoint(bone_transform));
            bounds = Union(bounds, TransformPoint(bone_transform, Vec2{b->length, 0}));
            bounds = Union(bounds, TransformPoint(bone_transform, Vec2{bone_width, bone_width}));
            bounds = Union(bounds, TransformPoint(bone_transform, Vec2{bone_width, -bone_width}));
        }

        for (int i=0; i<sdoc->skin_count; i++) {
            MeshDocument* skinned_mesh = sdoc->skins[i].mesh;
            if (!skinned_mesh || skinned_mesh->def->type != ASSET_TYPE_MESH)
                continue;

            // todo: this needs to account for the skinned mesh transform
            bounds = Union(bounds, GetBounds(skinned_mesh));
        }

        sdoc->bounds = Expand(bounds, BOUNDS_PADDING);
    }

    static void PostLoadAnimationData(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);

        adoc->skeleton = static_cast<SkeletonDocument*>(FindDocument(ASSET_TYPE_SKELETON, adoc->skeleton_name));
        if (!adoc->skeleton)
            return;

        for (int frame_index=0; frame_index<adoc->frame_count; frame_index++) {
            AnimationFrameData& f = adoc->frames[frame_index];
            if (!f.event_name) continue;
            f.event = static_cast<EventDocument*>(FindDocument(ASSET_TYPE_EVENT, f.event_name));
        }

        PostLoadDocument(adoc->skeleton);
        UpdateTransforms(adoc->skeleton);
        UpdateTransforms(adoc);
        UpdateBounds(adoc);
    }

    static void LoadAnimationDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        adoc->frame_count = 0;

        std::filesystem::path path = doc->path.value;
        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
        Tokenizer tk;
        Init(tk, contents.c_str());

        int bone_map[MAX_BONES];
        for (int i=0; i<MAX_BONES; i++)
            bone_map[i] = -1;

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "s"))
                ParseSkeleton(adoc, tk, bone_map);
            else if (ExpectIdentifier(tk, "f"))
                ParseFrame(adoc, tk, bone_map);
            else {
                char error[1024];
                GetString(tk, error, sizeof(error) - 1);
                return;
                //ThrowError("invalid token '%s' in animation", error);
            }
        }

        if (adoc->frame_count == 0) {
            AnimationFrameData& enf = adoc->frames[0];
            for (int i=0; i<MAX_BONES; i++)
                enf.transforms[i] = {
                    .position = VEC2_ZERO,
                    .scale = VEC2_ONE,
                    .rotation = 0,
                    .local_to_world = MAT3_IDENTITY,
                    .world_to_local = MAT3_IDENTITY
                };
            adoc->frame_count = 1;
        }

        adoc->bounds = { VEC2_NEGATIVE_ONE, VEC2_ONE };
    }

    static AnimationDocument* LoadAnimationDocument(const std::filesystem::path& path) {
        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
        Tokenizer tk;
        Init(tk, contents.c_str());

        AnimationDocument* adoc = static_cast<AnimationDocument*>(CreateDocument(path));
        InitAnimationDocument(adoc);
        LoadDocument(adoc);
        MarkModified(adoc);
        return adoc;
    }

    static void SerializeTransform(Stream* stream, const Transform& transform) {
        BoneTransform bone_transform = {
            .position = transform.position,
            .scale = transform.scale,
            .rotation = transform.rotation,
        };
        WriteStruct(stream, bone_transform);
    }

    void Serialize(AnimationDocument* adoc, Stream* stream, SkeletonDocument* s) {
        assert(s);

        SkeletonDocument* sdoc = adoc->skeleton;

        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_ANIMATION;
        header.version = 1;
        WriteAssetHeader(stream, &header);

        bool looping = (adoc->flags & ANIMATION_FLAG_LOOPING) != 0;
        int real_frame_count = GetFrameCountWithHolds(adoc);

        WriteU8(stream, (u8)sdoc->bone_count);
        WriteU8(stream, (u8)adoc->frame_count);
        WriteU8(stream, (u8)real_frame_count);
        WriteU8(stream, (u8)g_editor.config->GetInt("animation", "frame_rate", ANIMATION_FRAME_RATE));
        WriteU8(stream, (u8)adoc->flags);

        // todo: do we need this?
        for (int i=0; i<sdoc->bone_count; i++)
            WriteU8(stream, (u8)adoc->bones[i].index);

        // frame transforms (write absolute: bind pose + frame delta)
        for (int frame_index=0; frame_index<adoc->frame_count; frame_index++) {
            AnimationFrameData& f = adoc->frames[frame_index];

            // Bone 0 (root) - position is zeroed for root motion handling
            Transform transform = f.transforms[0];
            transform.position = VEC2_ZERO;
            transform.rotation += sdoc->bones[0].transform.rotation;
            SerializeTransform(stream, transform);

            for (int bone_index=1; bone_index<sdoc->bone_count; bone_index++) {
                BoneTransform& bind = sdoc->bones[bone_index].transform;
                Transform abs_transform = f.transforms[bone_index];
                abs_transform.position = bind.position + abs_transform.position;
                abs_transform.rotation = bind.rotation + abs_transform.rotation;
                SerializeTransform(stream, abs_transform);
            }
        }

        float base_root_motion = adoc->frames[0].transforms[0].position.x;

        // frames
        for (int frame_index=0; frame_index<adoc->frame_count; frame_index++) {
            AnimationFrameData& fd = adoc->frames[frame_index];
            AnimationFrame f = {};
            EventDocument* e = fd.event_name
                ? static_cast<EventDocument*>(FindDocument(ASSET_TYPE_EVENT, fd.event_name))
                : nullptr;

            f.event = e ? e->id : 0;
            f.transform0 = frame_index;
            f.transform1 = looping
                ? (frame_index + 1) % adoc->frame_count
                : Min(frame_index + 1, adoc->frame_count - 1);

            float root_motion0 = adoc->frames[f.transform0].transforms[0].position.x - base_root_motion;
            float root_motion1 = adoc->frames[f.transform1].transforms[0].position.x - base_root_motion;

            if (f.transform1 < f.transform0) {
                root_motion1 += root_motion0 + base_root_motion;
            }

            if (fd.hold == 0) {
                f.fraction0 = 0.0f;
                f.fraction1 = 1.0f;
                f.root_motion0 = root_motion0;
                f.root_motion1 = root_motion1;
                WriteStruct(stream, f);
                continue;
            }

            int hold_count = fd.hold + 1;
            for (int hold_index=0; hold_index<hold_count; hold_index++) {
                f.fraction1 = (float)(hold_index + 1) / (float)hold_count;
                f.root_motion1 = root_motion0 + (root_motion1 - root_motion0) * f.fraction1;
                WriteStruct(stream, f);
                f.fraction0 = f.fraction1;
                f.event = 0;
            }
        }

        // // Alwasy write an extra frame at the end to simplify sampling logic
        // AnimationFrame f = {};
        // f.transform0 = n->frame_count - 1;
        // f.transform1 = n->frame_count - 1;
        // f.fraction0 = 0.0f;
        // WriteStruct(stream, f);
    }

    Animation* ToAnimation(Allocator* allocator, AnimationDocument* adoc) {
        SkeletonDocument* es = GetSkeletonDocument(adoc);
        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 8192);
        if (!stream)
            return nullptr;

        Serialize(adoc, stream, es);
        SeekBegin(stream, 0);

        Animation* animation = static_cast<Animation*>(LoadAssetInternal(
            allocator,
            adoc->name,
            ASSET_TYPE_ANIMATION,
            LoadAnimation,
            stream));
        Free(stream);

        return animation;
    }

    static void SaveAnimationData(Document* doc, const std::filesystem::path& path) {
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        SkeletonDocument* sdoc = GetSkeletonDocument(adoc);

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

        WriteCSTR(stream, "s \"%s\"\n", adoc->skeleton_name->value);

        for (int i=0; i<sdoc->bone_count; i++) {
            const AnimationBoneData& eab = adoc->bones[i];
            WriteCSTR(stream, "b \"%s\"\n", eab.name->value);
        }

        for (int frame_index=0; frame_index<adoc->frame_count; frame_index++) {
            AnimationFrameData& f = adoc->frames[frame_index];

            WriteCSTR(stream, "f");

            if (f.hold > 0)
                WriteCSTR(stream, " h %d", f.hold);
            if (f.event_name)
                WriteCSTR(stream, " e \"%s\"", f.event_name->value);

            WriteCSTR(stream, "\n");

            for (int bone_index=0; bone_index<sdoc->bone_count; bone_index++) {
                Transform& bt = GetFrameTransform(adoc, bone_index, frame_index);

                bool has_pos = bt.position != VEC2_ZERO;
                bool has_rot = bt.rotation != 0.0f;

                if (!has_pos && !has_rot)
                    continue;

                WriteCSTR(stream, "b %d", bone_index);

                if (has_pos)
                    WriteCSTR(stream, " p %f %f", bt.position.x, bt.position.y);

                if (has_rot)
                    WriteCSTR(stream, " r %f", bt.rotation);

                WriteCSTR(stream, "\n");
            }
        }

        SaveStream(stream, path);
        Free(stream);
    }

    int InsertFrame(AnimationDocument* adoc, int insert_at) {
        if (adoc->frame_count >= MAX_ANIMATION_FRAMES)
            return -1;

        adoc->frame_count++;

        int copy_frame = Max(0,insert_at - 1);

        SkeletonDocument* sdoc = GetSkeletonDocument(adoc);
        for (int frame_index=adoc->frame_count-1; frame_index>insert_at; frame_index--)
            adoc->frames[frame_index] = adoc->frames[frame_index - 1];

        if (copy_frame >= 0)
            for (int j=0; j<sdoc->bone_count; j++)
                GetFrameTransform(adoc, j, insert_at) = GetFrameTransform(adoc, j, copy_frame);

        adoc->frames[insert_at].hold = 0;

        return insert_at;
    }

    int DeleteFrame(AnimationDocument* adoc, int frame_index) {
        if (adoc->frame_count <= 1)
            return frame_index;

        for (int i=frame_index; i<adoc->frame_count - 1; i++)
            adoc->frames[i] = adoc->frames[i + 1];

        adoc->frame_count--;

        return Min(frame_index, adoc->frame_count - 1);
    }

    Transform& GetFrameTransform(AnimationDocument* adoc, int bone_index, int frame_index) {
        assert(bone_index >= 0 && bone_index < MAX_BONES);
        assert(frame_index >= 0 && frame_index < adoc->frame_count);
        return adoc->frames[frame_index].transforms[bone_index];
    }

    AnimationDocument* NewAnimationDocument(const std::filesystem::path& path) {
        if (g_workspace.selected_asset_count != 1) {
            LogError("no skeleton selected");
            return nullptr;
        }

        std::filesystem::path full_path = path.is_relative() ?  std::filesystem::path(g_editor.project_path) / "assets" / "animations" / path : path;
        full_path += ".anim";

        Document* skeleton_asset = GetFirstSelectedAsset();
        if (!skeleton_asset || skeleton_asset->def->type != ASSET_TYPE_SKELETON)
        {
            LogError("no skeleton selected");
            return nullptr;
        }

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        WriteCSTR(stream, "s \"%s\"\n", skeleton_asset->name->value);
        SaveStream(stream, full_path);
        Free(stream);

        QueueImport(full_path);
        WaitForImportTasks();
        return LoadAnimationDocument(full_path);
    }

    static void HandleAnimationUndoRedo(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        UpdateSkeleton(adoc);
        UpdateTransforms(adoc);
    }

    static void LoadAnimationMetadata(Document* doc, Props* meta) {
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        adoc->flags = ANIMATION_FLAG_NONE;
        if (meta->GetBool("animation", "loop", true))
            adoc->flags |= ANIMATION_FLAG_LOOPING;
        if (meta->GetBool("animation", "root_motion", false))
            adoc->flags |= ANIMATION_FLAG_ROOT_MOTION;
    }

    static void SaveAnimationDocumentMeta(Document* doc, Props* meta) {
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        meta->SetBool("animation", "loop", IsLooping(adoc));
        meta->SetBool("animation", "root_motion", IsRootMotion(adoc));
    }

    static void CloneAnimationData(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        adoc->animation = nullptr;
        adoc->animator = {};
        if (adoc->skeleton) {
            UpdateTransforms(adoc);
            UpdateBounds(adoc);
        }
    }

    static void DestroyAnimationData(Document* doc) {
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        Free(adoc->animation);
        adoc->animation = nullptr;
    }

    int HitTestBones(AnimationDocument* adoc, const Mat3& transform, const Vec2& position, int* bones, int max_bones) {
        SkeletonDocument* sdoc = adoc->skeleton;
        int bone_count = 0;
        for (int bone_index=sdoc->bone_count-1; bone_index>=0 && max_bones > 0; bone_index--) {
            BoneData* sb = &sdoc->bones[bone_index];
            Mat3 local_to_world = transform * adoc->animator.bones[bone_index] * Scale(sb->length);
            if (OverlapPoint(g_workspace.bone_collider, local_to_world, position)) {
                bones[bone_count++] = bone_index;
                max_bones--;
            }
        }
        return bone_count;
    }

    int HitTestBone(AnimationDocument* adoc, const Mat3& transform, const Vec2& position) {
        int bones[1];
        if (0 == HitTestBones(adoc, transform, position, bones, 1))
            return -1;
        return bones[0];
    }

    void SetLooping(AnimationDocument* adoc, bool looping) {
        if (looping)
            adoc->flags |= ANIMATION_FLAG_LOOPING;
        else
            adoc->flags &= ~ANIMATION_FLAG_LOOPING;

        MarkMetaModified(adoc);
    }

    static void InitAnimationDocument(AnimationDocument* adoc) {
        adoc->vtable = {
            .destructor = DestroyAnimationData,
            .load = LoadAnimationDocument,
            .post_load = PostLoadAnimationData,
            .save = SaveAnimationData,
            .load_metadata = LoadAnimationMetadata,
            .save_metadata = SaveAnimationDocumentMeta,
            .draw = DrawAnimationData,
            .clone = CloneAnimationData,
            .undo_redo = HandleAnimationUndoRedo
        };

        InitAnimationEditor(adoc);
    }

    void InitAnimationDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        InitAnimationDocument(static_cast<AnimationDocument*>(doc));
    }


    static void ImportAnimation(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        (void)config;
        (void)meta;

        assert(doc);
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
        SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(FindDocument(ASSET_TYPE_SKELETON, adoc->skeleton_name));
        if (!sdoc)
            ThrowError("invalid skeleton");

        Stream* stream = CreateStream(nullptr, 4096);
        Serialize(adoc, stream, sdoc);
        SaveStream(stream, path);
        Free(stream);
    }

    static bool CheckAnimationDependency(Document* doc, Document* dependency) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_ANIMATION);
        assert(dependency);

        if (dependency->def->type != ASSET_TYPE_SKELETON)
            return false;

#if 0
        AnimationDocument* en = LoadEditorAnimation(doc->path);
        if (!en)
            return false;

        // does the path end with en->skeleton_name + ".skel"?
        std::string expected_path = en->skeleton_name->value;
        std::string path_str = dependency_path.string();
        expected_path += ".skel";

        CleanPath(expected_path.data());
        CleanPath(path_str.data());

        bool result = path_str.ends_with(expected_path);

        Free(en);

        return result;
#else
        return false;
#endif
    }

    void InitAnimationDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_ANIMATION,
            .size = sizeof(AnimationDocument),
            .ext = ".anim",
            .init_func = InitAnimationDocument,
            .import_func = ImportAnimation,
            .check_dependency_func = CheckAnimationDependency,
        });
    }
}
