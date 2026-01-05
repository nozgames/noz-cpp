//
//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

extern Shader* SHADER_TEXTURED_MESH;

extern void InitTextureEditor(TextureData*);

static void AllocTextureDataImpl(AssetData* a) {
    assert(a->type == ASSET_TYPE_TEXTURE);
    TextureData* t = static_cast<TextureData*>(a);
    t->impl = static_cast<TextureDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(TextureDataImpl)));
    memset(t->impl, 0, sizeof(TextureDataImpl));
}

static void CloneTextureData(AssetData* a) {
    assert(a->type == ASSET_TYPE_TEXTURE);
    TextureData* t = static_cast<TextureData*>(a);
    TextureDataImpl* old_impl = t->impl;
    AllocTextureDataImpl(a);
    memcpy(t->impl, old_impl, sizeof(TextureDataImpl));
}

static void DestroyTextureData(AssetData* a) {
    TextureData* t = static_cast<TextureData*>(a);
    Free(t->impl);
    t->impl = nullptr;
}

void DrawTextureData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_TEXTURE);

    TextureData* t = static_cast<TextureData*>(a);
    TextureDataImpl* impl = t->impl;
    if (!t || !impl->material)
        return;

    BindDepth(-0.1f);
    BindColor(COLOR_WHITE);
    BindMaterial(impl->material);
    DrawMesh(g_view.quad_mesh, Translate(a->position) * Scale(Vec2{GetSize(t->bounds).x, -GetSize(t->bounds).y}));
    BindDepth(0.1f);
}

void UpdateBounds(TextureData* t) {
    TextureDataImpl* impl = t->impl;
    if (impl->texture) {
        Vec2 tsize = ToVec2(GetSize(impl->texture)) / 72.0f;
        t->bounds = Bounds2{-tsize.x*0.5f, -tsize.y*0.5f, tsize.x*0.5f, tsize.y*0.5f};
    } else {
        t->bounds = Bounds2{
            Vec2{-0.5f, -0.5f} * impl->scale,
            Vec2{0.5f, 0.5f} * impl->scale
        };
    }

    t->bounds = { t->bounds.min * impl->scale, t->bounds.max * impl->scale };
}

static void LoadTextureMetaData(AssetData* a, Props* meta) {
    assert(a);
    assert(a->type == ASSET_TYPE_TEXTURE);
    TextureData* t = static_cast<TextureData*>(a);
    TextureDataImpl* impl = t->impl;
    t->editor_only = meta->GetBool("texture", "reference", false) || Contains(a->path, "reference", true);
    impl->scale = meta->GetFloat("editor", "scale", 1.0f);
    InitTextureEditor(static_cast<TextureData*>(a));
    UpdateBounds(t);
}


static void SaveTextureMetaData(AssetData* a, Props* meta) {
    assert(a);
    assert(a->type == ASSET_TYPE_TEXTURE);
    TextureData* t = static_cast<TextureData*>(a);
    TextureDataImpl* impl = t->impl;
    meta->SetString("editor", "scale", std::to_string(impl->scale).c_str());
    meta->SetBool("texture", "reference", t->editor_only);
}

void PostLoadTextureData(AssetData* a) {
    assert(a->type == ASSET_TYPE_TEXTURE);
    TextureData* t = static_cast<TextureData*>(a);
    TextureDataImpl* impl = t->impl;
    impl->texture = (Texture*)LoadAssetInternal(ALLOCATOR_DEFAULT, a->name, ASSET_TYPE_TEXTURE, LoadTexture);
    impl->material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
    SetTexture(impl->material, impl->texture, 0);
    UpdateBounds(t);
}

static void ReloadTextureData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_TEXTURE);

    TextureData* t = static_cast<TextureData*>(a);
    TextureDataImpl* impl = t->impl;
    if (!impl->texture) {
        LoadAssetData(a);
        PostLoadAssetData(a);
    } else {
        ReloadAsset(a->name, ASSET_TYPE_TEXTURE, impl->texture, ReloadTexture);
    }
}

void InitTextureData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_TEXTURE);

    TextureData* t = static_cast<TextureData*>(a);
    AllocTextureDataImpl(a);
    TextureDataImpl* impl = t->impl;

    t->bounds = Bounds2{Vec2{-0.5f, -0.5f}, Vec2{0.5f, 0.5f}};
    impl->scale = 1.0f;
    t->vtable = {
        .destructor = DestroyTextureData,
        .reload = ReloadTextureData,
        .post_load = PostLoadTextureData,
        .load_metadata = LoadTextureMetaData,
        .save_metadata = SaveTextureMetaData,
        .draw = DrawTextureData,
        .clone = CloneTextureData,
    };
}
