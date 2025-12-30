//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

extern Mesh* MESH_ASSET_ICON_SOUND;

static void AllocSoundImpl(AssetData* a) {
    assert(a->type == ASSET_TYPE_SOUND);
    SoundData* s = static_cast<SoundData*>(a);
    s->impl = static_cast<SoundDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(SoundDataImpl)));
    memset(s->impl, 0, sizeof(SoundDataImpl));
}

static void CloneSoundData(AssetData* a) {
    assert(a->type == ASSET_TYPE_SOUND);
    SoundData* s = static_cast<SoundData*>(a);
    SoundDataImpl* old_impl = s->impl;
    AllocSoundImpl(a);
    memcpy(s->impl, old_impl, sizeof(SoundDataImpl));
}

static void DestroySoundData(AssetData* a) {
    SoundData* s = static_cast<SoundData*>(a);
    Free(s->impl);
    s->impl = nullptr;
}

static void DrawSoundData(AssetData* a) {
    BindMaterial(g_view.shaded_material);
    BindColor(COLOR_WHITE);
    DrawMesh(MESH_ASSET_ICON_SOUND, Translate(a->position));
}

static void PlaySoundData(AssetData* a) {
    SoundData* s = static_cast<SoundData*>(a);
    SoundDataImpl* impl = s->impl;
    if (!impl->sound)
        impl->sound = static_cast<Sound*>(LoadAssetInternal(ALLOCATOR_DEFAULT, s->name, ASSET_TYPE_SOUND, LoadSound));

    Play(impl->sound, 1.0f, 1.0f);
}

static void InitSoundData(SoundData* s) {
    AllocSoundImpl(s);

    s->vtable = {
        .destructor = DestroySoundData,
        .draw = DrawSoundData,
        .play = PlaySoundData,
        .clone = CloneSoundData,
    };
}

void InitSoundData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_SOUND);
    InitSoundData(static_cast<SoundData*>(a));
}
