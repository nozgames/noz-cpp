//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <complex>

extern Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, Stream* stream);
static void InitAnimationData(AnimationDocument* a);
extern void InitAnimationEditor(AnimationDocument* a);

inline SkeletonDocument* GetSkeletonData(AnimationDocument* n) { return n->impl->skeleton; }

int GetRealFrameIndex(AnimationDocument* n, int frame_index) {
    AnimationDataImpl* impl = n->impl;
    for (int i=0; i<impl->frame_count; i++)
        for (int h=0; h<=impl->frames[i].hold; h++, frame_index--)
            if (frame_index == 0) return i;

    return impl->frame_count;
}

int GetFrameIndexWithHolds(AnimationDocument* n, int frame_index) {
    AnimationDataImpl* impl = n->impl;
    int frame_index_with_holds = 0;
    for (int i=0; i<frame_index; i++) {
        frame_index_with_holds++;
        frame_index_with_holds += impl->frames[i].hold;
    }
    return frame_index_with_holds;
}

int GetFrameCountWithHolds(AnimationDocument* n) {
    AnimationDataImpl* impl = n->impl;
    int frame_count = 0;
    for (int frame_index=0; frame_index<impl->frame_count; frame_index++) {
        frame_count++;
        frame_count += impl->frames[frame_index].hold;
    }
    return frame_count;
}

void UpdateTransforms(AnimationDocument* n, int frame_index) {
    AnimationDataImpl* impl = n->impl;
    if (frame_index == -1)
        frame_index = impl->current_frame;

    SkeletonDocument* s = GetSkeletonData(n);
    SkeletonDataImpl* skelimpl = s->impl;
    for (int bone_index=0; bone_index<skelimpl->bone_count; bone_index++) {
        BoneData* b = &skelimpl->bones[bone_index];
        Transform& frame = GetFrameTransform(n, bone_index, frame_index);

        impl->animator.bones[bone_index] = TRS(
            b->transform.position + frame.position,
            b->transform.rotation + frame.rotation,
            b->transform.scale);
    }

    for (int bone_index=1; bone_index<skelimpl->bone_count; bone_index++)
        impl->animator.bones[bone_index] =
            impl->animator.bones[skelimpl->bones[bone_index].parent_index] * impl->animator.bones[bone_index];
}

void DrawAnimationData(Document* a) {
    assert(a->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    AnimationDataImpl* impl = n->impl;
    SkeletonDocument* s = GetSkeletonData(n);
    if (!s)
        return;

    SkeletonDataImpl* skelimpl = s->impl;
    Mat3 transform = Translate(a->position);

    if (!a->editing)
        transform = Translate(-GetFrameTransform(n, 0, impl->current_frame).position) * transform;

    BindColor(COLOR_WHITE);
    BindSkeleton(&skelimpl->bones[0].world_to_local, sizeof(BoneData), impl->animator.bones, 0, skelimpl->bone_count);
    for (int i=0; i<skelimpl->skin_count; i++) {
        MeshDocument* skinned_mesh = skelimpl->skins[i].mesh;
        if (!skinned_mesh || skinned_mesh->type != ASSET_TYPE_MESH)
            continue;

        DrawMesh(skinned_mesh, transform, g_view.shaded_skinned_material);
    }
}

static void ParseSkeletonBone(Tokenizer& tk, SkeletonDocument* es, int bone_index, int* bone_map) {
    if (!ExpectQuotedString(tk))
        throw std::exception("missing quoted bone name");

    bone_map[bone_index] = FindBoneIndex(es, GetName(tk));
}

void UpdateSkeleton(AnimationDocument* n) {
    AnimationDataImpl* impl = n->impl;
    SkeletonDocument* s = GetSkeletonData(n);

    int bone_map[MAX_BONES];
    for (int i=0; i<MAX_BONES; i++)
        bone_map[i] = -1;

    for (int i=0; i<impl->bone_count; i++) {
        int new_bone_index = FindBoneIndex(s, impl->bones[i].name);
        if (new_bone_index == -1)
            continue;
        bone_map[new_bone_index] = i;
    }

    AnimationFrameData new_frames[MAX_ANIMATION_FRAMES];
    for (int frame_index=0; frame_index<impl->frame_count; frame_index++)
        new_frames[frame_index] = impl->frames[frame_index];

    memcpy(impl->frames, new_frames, sizeof(new_frames));

    SkeletonDataImpl* skelimpl = s->impl;
    for (int i=0; i<skelimpl->bone_count; i++) {
        AnimationBoneData& ab = impl->bones[i];
        ab.index = i;
        ab.name = skelimpl->bones[i].name;
    }

    impl->bone_count = skelimpl->bone_count;

    UpdateBounds(n);
    UpdateTransforms(n);
}

static void ParseSkeleton(AnimationDocument* n, Tokenizer& tk, int* bone_map) {
    AnimationDataImpl* impl = n->impl;
    if (!ExpectQuotedString(tk))
        throw std::exception("missing quoted skeleton name");

    impl->skeleton_name = GetName(tk);

    SkeletonDocument* s = static_cast<SkeletonDocument*>(GetDocument(ASSET_TYPE_SKELETON, impl->skeleton_name));
    if (!s)
        return;
    assert(s);

    if (!s->loaded)
        LoadAssetData(s);

    SkeletonDataImpl* skelimpl = s->impl;
    for (int i=0; i<skelimpl->bone_count; i++) {
        AnimationBoneData& enb = impl->bones[i];
        BoneData& eb = skelimpl->bones[i];
        enb.name = eb.name;
        enb.index = i;
    }

    impl->bone_count = skelimpl->bone_count;

    for (int frame_index=0; frame_index<MAX_ANIMATION_FRAMES; frame_index++)
        for (int bone_index=0; bone_index<MAX_BONES; bone_index++)
            SetIdentity(impl->frames[frame_index].transforms[bone_index]);

    int bone_index = 0;
    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "b"))
            ParseSkeletonBone(tk, s, bone_index++, bone_map);
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

static void ParseFramePosition(AnimationDocument* n, Tokenizer& tk, int bone_index, int frame_index) {
    float x;
    if (!ExpectFloat(tk, &x))
        ThrowError("expected position 'x' value");
    float y;
    if (!ExpectFloat(tk, &y))
        ThrowError("expected position 'y' value");

    if (bone_index == -1)
        return;

    SetPosition(GetFrameTransform(n, bone_index, frame_index), {x,y});
}

static void ParseFrameHold(AnimationDocument* n, Tokenizer& tk, int frame_index) {
    AnimationDataImpl* impl = n->impl;
    int hold;
    if (!ExpectInt(tk, &hold))
        ThrowError("expected hold value");

    impl->frames[frame_index].hold = Max(0, hold);
}

static void ParseFrameRotation(AnimationDocument* n, Tokenizer& tk, int bone_index, int frame_index) {
    float r;
    if (!ExpectFloat(tk, &r))
        ThrowError("expected rotation value");

    if (bone_index == -1)
        return;

    SetRotation(GetFrameTransform(n, bone_index, frame_index), r);
}

static void ParseFrameScale(AnimationDocument* n, Tokenizer& tk, int bone_index, int frame_index) {
    float s;
    if (!ExpectFloat(tk, &s))
        ThrowError("expected scale value");

    if (bone_index == -1)
        return;

    SetScale(GetFrameTransform(n, bone_index, frame_index), s);
}

static void ParseFrameEvent(AnimationDocument* n, Tokenizer& tk, int frame_index) {
    AnimationDataImpl* impl = n->impl;
    if (!ExpectQuotedString(tk))
        ThrowError("expected event name");

    impl->frames[frame_index].event_name = GetName(tk);
}

static void ParseFrame(AnimationDocument* n, Tokenizer& tk, int* bone_map) {
    AnimationDataImpl* impl = n->impl;
    int bone_index = -1;
    impl->frame_count++;
    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "b"))
            bone_index = ParseFrameBone(n, tk, bone_map);
        else if (ExpectIdentifier(tk, "e"))
            ParseFrameEvent(n, tk, impl->frame_count - 1);
        else if (ExpectIdentifier(tk, "r"))
            ParseFrameRotation(n, tk, bone_index, impl->frame_count - 1);
        else if (ExpectIdentifier(tk, "s"))
            ParseFrameScale(n, tk, bone_index, impl->frame_count - 1);
        else if (ExpectIdentifier(tk, "p"))
            ParseFramePosition(n, tk, bone_index, impl->frame_count - 1);
        else if (ExpectIdentifier(tk, "h"))
            ParseFrameHold(n, tk, impl->frame_count - 1);
        else
            break;
    }
}

void UpdateBounds(AnimationDocument* n) {
    AnimationDataImpl* impl = n->impl;
    n->bounds = GetSkeletonData(n)->bounds;

    SkeletonDocument* s = GetSkeletonData(n);
    SkeletonDataImpl* skelimpl = s->impl;
    Vec2 root_position = TransformPoint(impl->animator.bones[0]);
    Bounds2 bounds = Bounds2 { root_position, root_position };
    for (int bone_index=0; bone_index<impl->bone_count; bone_index++) {
        BoneData* b = skelimpl->bones + bone_index;
        float bone_width = b->length * BONE_WIDTH;
        const Mat3& bone_transform = impl->animator.bones[bone_index];
        bounds = Union(bounds, TransformPoint(bone_transform));
        bounds = Union(bounds, TransformPoint(bone_transform, Vec2{b->length, 0}));
        bounds = Union(bounds, TransformPoint(bone_transform, Vec2{bone_width, bone_width}));
        bounds = Union(bounds, TransformPoint(bone_transform, Vec2{bone_width, -bone_width}));
    }

    for (int i=0; i<skelimpl->skin_count; i++) {
        MeshDocument* skinned_mesh = skelimpl->skins[i].mesh;
        if (!skinned_mesh || skinned_mesh->type != ASSET_TYPE_MESH)
            continue;

        // todo: this needs to account for the skinned mesh transform
        bounds = Union(bounds, GetBounds(skinned_mesh));
    }

    s->bounds = Expand(bounds, BOUNDS_PADDING);
}

static void PostLoadAnimationData(Document* a) {
    assert(a->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    AnimationDataImpl* impl = n->impl;

    impl->skeleton = static_cast<SkeletonDocument*>(GetDocument(ASSET_TYPE_SKELETON, impl->skeleton_name));
    if (!impl->skeleton)
        return;

    for (int frame_index=0; frame_index<impl->frame_count; frame_index++) {
        AnimationFrameData& f = impl->frames[frame_index];
        if (!f.event_name) continue;
        f.event = static_cast<EventDocument*>(GetDocument(ASSET_TYPE_EVENT, f.event_name));
    }

    PostLoadAssetData(impl->skeleton);
    UpdateTransforms(impl->skeleton);
    UpdateTransforms(n);
    UpdateBounds(n);
}

static void LoadAnimationData(Document* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    AnimationDataImpl* impl = n->impl;
    impl->frame_count = 0;

    std::filesystem::path path = a->path.value;
    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
    Tokenizer tk;
    Init(tk, contents.c_str());

    int bone_map[MAX_BONES];
    for (int i=0; i<MAX_BONES; i++)
        bone_map[i] = -1;

    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "s"))
            ParseSkeleton(n, tk, bone_map);
        else if (ExpectIdentifier(tk, "f"))
            ParseFrame(n, tk, bone_map);
        else {
            char error[1024];
            GetString(tk, error, sizeof(error) - 1);
            return;
            //ThrowError("invalid token '%s' in animation", error);
        }
    }

    if (impl->frame_count == 0) {
        AnimationFrameData& enf = impl->frames[0];
        for (int i=0; i<MAX_BONES; i++)
            enf.transforms[i] = {
                .position = VEC2_ZERO,
                .scale = VEC2_ONE,
                .rotation = 0,
                .local_to_world = MAT3_IDENTITY,
                .world_to_local = MAT3_IDENTITY
            };
        impl->frame_count = 1;
    }

    n->bounds = { VEC2_NEGATIVE_ONE, VEC2_ONE };
}

static AnimationDocument* LoadAnimationData(const std::filesystem::path& path) {
    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
    Tokenizer tk;
    Init(tk, contents.c_str());

    AnimationDocument* n = static_cast<AnimationDocument*>(CreateAssetData(path));
    assert(n);
    InitAnimationData(n);
    LoadAssetData(n);
    MarkModified(n);
    return n;
}

static void SerializeTransform(Stream* stream, const Transform& transform) {
    BoneTransform bone_transform = {
        .position = transform.position,
        .scale = transform.scale,
        .rotation = transform.rotation,
    };
    WriteStruct(stream, bone_transform);
}

void Serialize(AnimationDocument* n, Stream* stream, SkeletonDocument* s) {
    assert(s);
    AnimationDataImpl* impl = n->impl;
    SkeletonDataImpl* skelimpl = s->impl;

    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE;
    header.type = ASSET_TYPE_ANIMATION;
    header.version = 1;
    WriteAssetHeader(stream, &header);

    bool looping = (impl->flags & ANIMATION_FLAG_LOOPING) != 0;
    int real_frame_count = GetFrameCountWithHolds(n);

    WriteU8(stream, (u8)skelimpl->bone_count);
    WriteU8(stream, (u8)impl->frame_count);
    WriteU8(stream, (u8)real_frame_count);
    WriteU8(stream, (u8)g_config->GetInt("animation", "frame_rate", ANIMATION_FRAME_RATE));
    WriteU8(stream, (u8)impl->flags);

    // todo: do we need this?
    for (int i=0; i<skelimpl->bone_count; i++)
        WriteU8(stream, (u8)impl->bones[i].index);

    // frame transforms (write absolute: bind pose + frame delta)
    for (int frame_index=0; frame_index<impl->frame_count; frame_index++) {
        AnimationFrameData& f = impl->frames[frame_index];

        // Bone 0 (root) - position is zeroed for root motion handling
        Transform transform = f.transforms[0];
        transform.position = VEC2_ZERO;
        transform.rotation += skelimpl->bones[0].transform.rotation;
        SerializeTransform(stream, transform);

        for (int bone_index=1; bone_index<skelimpl->bone_count; bone_index++) {
            BoneTransform& bind = skelimpl->bones[bone_index].transform;
            Transform abs_transform = f.transforms[bone_index];
            abs_transform.position = bind.position + abs_transform.position;
            abs_transform.rotation = bind.rotation + abs_transform.rotation;
            SerializeTransform(stream, abs_transform);
        }
    }

    float base_root_motion = impl->frames[0].transforms[0].position.x;

    // frames
    for (int frame_index=0; frame_index<impl->frame_count; frame_index++) {
        AnimationFrameData& fd = impl->frames[frame_index];
        AnimationFrame f = {};
        EventDocument* e = fd.event_name
            ? static_cast<EventDocument*>(GetDocument(ASSET_TYPE_EVENT, fd.event_name))
            : nullptr;

        f.event = e ? e->id : 0;
        f.transform0 = frame_index;
        f.transform1 = looping
            ? (frame_index + 1) % impl->frame_count
            : Min(frame_index + 1, impl->frame_count - 1);

        float root_motion0 = impl->frames[f.transform0].transforms[0].position.x - base_root_motion;
        float root_motion1 = impl->frames[f.transform1].transforms[0].position.x - base_root_motion;

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

Animation* ToAnimation(Allocator* allocator, AnimationDocument* n) {
    SkeletonDocument* es = GetSkeletonData(n);
    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 8192);
    if (!stream)
        return nullptr;

    Serialize(n, stream, es);
    SeekBegin(stream, 0);

    Animation* animation = static_cast<Animation*>(LoadAssetInternal(allocator, n->name, ASSET_TYPE_ANIMATION, LoadAnimation, stream));
    Free(stream);

    return animation;
}

static void SaveAnimationData(Document* ea, const std::filesystem::path& path) {
    assert(ea->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* n = (AnimationDocument*)ea;
    AnimationDataImpl* impl = n->impl;
    SkeletonDocument* es = GetSkeletonData(n);
    SkeletonDataImpl* skelimpl = es->impl;

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);

    WriteCSTR(stream, "s \"%s\"\n", impl->skeleton_name->value);

    for (int i=0; i<skelimpl->bone_count; i++) {
        const AnimationBoneData& eab = impl->bones[i];
        WriteCSTR(stream, "b \"%s\"\n", eab.name->value);
    }

    for (int frame_index=0; frame_index<impl->frame_count; frame_index++) {
        AnimationFrameData& f = impl->frames[frame_index];

        WriteCSTR(stream, "f");

        if (f.hold > 0)
            WriteCSTR(stream, " h %d", f.hold);
        if (f.event_name)
            WriteCSTR(stream, " e \"%s\"", f.event_name->value);

        WriteCSTR(stream, "\n");

        for (int bone_index=0; bone_index<skelimpl->bone_count; bone_index++) {
            Transform& bt = GetFrameTransform(n, bone_index, frame_index);

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

int InsertFrame(AnimationDocument* n, int insert_at) {
    AnimationDataImpl* impl = n->impl;
    if (impl->frame_count >= MAX_ANIMATION_FRAMES)
        return -1;

    impl->frame_count++;

    int copy_frame = Max(0,insert_at - 1);

    SkeletonDocument* s = GetSkeletonData(n);
    SkeletonDataImpl* skelimpl = s->impl;
    for (int frame_index=impl->frame_count-1; frame_index>insert_at; frame_index--)
        impl->frames[frame_index] = impl->frames[frame_index - 1];

    if (copy_frame >= 0)
        for (int j=0; j<skelimpl->bone_count; j++)
            GetFrameTransform(n, j, insert_at) = GetFrameTransform(n, j, copy_frame);

    impl->frames[insert_at].hold = 0;

    return insert_at;
}

int DeleteFrame(AnimationDocument* n, int frame_index) {
    AnimationDataImpl* impl = n->impl;
    if (impl->frame_count <= 1)
        return frame_index;

    for (int i=frame_index; i<impl->frame_count - 1; i++)
        impl->frames[i] = impl->frames[i + 1];

    impl->frame_count--;

    return Min(frame_index, impl->frame_count - 1);
}

Transform& GetFrameTransform(AnimationDocument* n, int bone_index, int frame_index) {
    AnimationDataImpl* impl = n->impl;
    assert(bone_index >= 0 && bone_index < MAX_BONES);
    assert(frame_index >= 0 && frame_index < impl->frame_count);
    return impl->frames[frame_index].transforms[bone_index];
}

Document* NewAnimationDocument(const std::filesystem::path& path) {
    if (g_view.selected_asset_count != 1) {
        LogError("no skeleton selected");
        return nullptr;
    }

    std::filesystem::path full_path = path.is_relative() ?  std::filesystem::path(g_editor.project_path) / "assets" / "animations" / path : path;
    full_path += ".anim";

    Document* skeleton_asset = GetFirstSelectedAsset();
    if (!skeleton_asset || skeleton_asset->type != ASSET_TYPE_SKELETON)
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
    return LoadAnimationData(full_path);
}

static void HandleAnimationUndoRedo(Document* a) {
    assert(a->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    UpdateSkeleton(n);
    UpdateTransforms(n);
}

static void LoadAnimationMetadata(Document* a, Props* meta) {
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    AnimationDataImpl* impl = n->impl;
    impl->flags = ANIMATION_FLAG_NONE;
    if (meta->GetBool("animation", "loop", true))
        impl->flags |= ANIMATION_FLAG_LOOPING;
    if (meta->GetBool("animation", "root_motion", false))
        impl->flags |= ANIMATION_FLAG_ROOT_MOTION;
}

static void SaveAnimationMetadata(Document* a, Props* meta) {
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    AnimationDataImpl* impl = n->impl;
    meta->SetBool("animation", "loop", IsLooping(impl->flags));
    meta->SetBool("animation", "root_motion", IsRootMotion(n));
}

static void AllocAnimationImpl(Document* a) {
    assert(a->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    n->impl = static_cast<AnimationDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(AnimationDataImpl)));
    memset(n->impl, 0, sizeof(AnimationDataImpl));
}

static void CloneAnimationData(Document* a) {
    assert(a->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    AnimationDataImpl* old_impl = n->impl;
    AllocAnimationImpl(a);
    memcpy(n->impl, old_impl, sizeof(AnimationDataImpl));
    n->impl->animation = nullptr;
    n->impl->animator = {};
    if (n->impl->skeleton) {
        UpdateTransforms(n);
        UpdateBounds(n);
    }
}

static void DestroyAnimationData(Document* a) {
    AnimationDocument* n = static_cast<AnimationDocument*>(a);
    Free(n->impl);
    n->impl = nullptr;
}

int HitTestBones(AnimationDocument* n, const Mat3& transform, const Vec2& position, int* bones, int max_bones) {
    AnimationDataImpl* impl = n->impl;
    SkeletonDocument* s = impl->skeleton;
    SkeletonDataImpl* skelimpl = s->impl;
    int bone_count = 0;
    for (int bone_index=impl->bone_count-1; bone_index>=0 && max_bones > 0; bone_index--) {
        BoneData* sb = &skelimpl->bones[bone_index];
        Mat3 local_to_world = transform * impl->animator.bones[bone_index] * Scale(sb->length);
        if (OverlapPoint(g_view.bone_collider, local_to_world, position)) {
            bones[bone_count++] = bone_index;
            max_bones--;
        }
    }
    return bone_count;
}

int HitTestBone(AnimationDocument* n, const Mat3& transform, const Vec2& position) {
    int bones[1];
    if (0 == HitTestBones(n, transform, position, bones, 1))
        return -1;
    return bones[0];
}

void SetLooping(AnimationDocument* n, bool looping) {
    AnimationDataImpl* impl = n->impl;
    if (looping)
        impl->flags |= ANIMATION_FLAG_LOOPING;
    else
        impl->flags &= ~ANIMATION_FLAG_LOOPING;

    MarkMetaModified(n);
}

static void InitAnimationData(AnimationDocument* a) {
    AllocAnimationImpl(a);

    a->vtable = {
        .destructor = DestroyAnimationData,
        .load = LoadAnimationData,
        .post_load = PostLoadAnimationData,
        .save = SaveAnimationData,
        .load_metadata = LoadAnimationMetadata,
        .save_metadata = SaveAnimationMetadata,
        .draw = DrawAnimationData,
        .clone = CloneAnimationData,
        .undo_redo = HandleAnimationUndoRedo
    };

    InitAnimationEditor(a);
}

void InitAnimationData(Document* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ANIMATION);
    InitAnimationData(static_cast<AnimationDocument*>(a));
}
