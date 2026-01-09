//
//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {
    extern void InitTextureEditor(TextureDocument*);
    extern void ImportTextureDocument(Document* doc, const std::filesystem::path& path, Props* config, Props* meta);

    static void CloneTextureDocument(Document* doc) {
        assert(doc->type == ASSET_TYPE_TEXTURE);
        TextureDocument* impl = static_cast<TextureDocument*>(doc);
        impl->texture = nullptr;
    }

    static void DestroyTextureDocument(Document* doc) {
        TextureDocument* impl = static_cast<TextureDocument*>(doc);
        Free(impl->texture);
        impl->texture = nullptr;
    }

    void DrawTextureDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_TEXTURE);

        TextureDocument* impl = static_cast<TextureDocument*>(doc);
        if (!impl->texture) return;

        BindDepth(-0.1f);
        BindColor(COLOR_WHITE);
        BindShader(SHADER_EDITOR_TEXTURE);
        BindTexture(impl->texture);
        DrawMesh(g_view.quad_mesh, Translate(doc->position) * Scale(Vec2{GetSize(impl->bounds).x, -GetSize(impl->bounds).y}));
        BindDepth(0.1f);
    }

    void UpdateBounds(TextureDocument* doc) {
        // 512 pixels = 10 units for grid alignment with power-of-2 textures
        constexpr float PIXELS_PER_UNIT = 51.2f;
        if (doc->texture) {
            Vec2 tsize = ToVec2(GetSize(doc->texture)) / PIXELS_PER_UNIT;
            doc->bounds = Bounds2{-tsize.x*0.5f, -tsize.y*0.5f, tsize.x*0.5f, tsize.y*0.5f};
        } else {
            doc->bounds = Bounds2{
                Vec2{-0.5f, -0.5f} * doc->scale,
                Vec2{0.5f, 0.5f} * doc->scale
            };
        }

        doc->bounds = { doc->bounds.min * doc->scale, doc->bounds.max * doc->scale };
    }

    static void LoadTextureDocument(Document* doc, Props* meta) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_TEXTURE);
        TextureDocument* d = static_cast<TextureDocument*>(doc);
        d->editor_only = meta->GetBool("texture", "reference", false) || Contains(doc->path, "reference", true);
        d->scale = meta->GetFloat("editor", "scale", 1.0f);
        InitTextureEditor(d);
        UpdateBounds(d);
    }

    static void SaveTextureDocumentMeta(Document* doc, Props* meta) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_TEXTURE);
        TextureDocument* d = static_cast<TextureDocument*>(doc);
        meta->SetString("editor", "scale", std::to_string(d->scale).c_str());
        meta->SetBool("texture", "reference", d->editor_only);
    }

    void PostLoadTextureDocument(Document* doc) {
        assert(doc->type == ASSET_TYPE_TEXTURE);
        TextureDocument* d = static_cast<TextureDocument*>(doc);
        d->texture = static_cast<Texture*>(LoadAssetInternal(ALLOCATOR_DEFAULT, doc->name, ASSET_TYPE_TEXTURE, LoadTexture));
        UpdateBounds(d);
    }

    static void ReloadTextureDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_TEXTURE);

        TextureDocument* d = static_cast<TextureDocument*>(doc);
        if (!d->texture) {
            LoadDocument(doc);
            PostLoadDocument(doc);
        } else {
            ReloadAsset(doc->name, ASSET_TYPE_TEXTURE, d->texture, ReloadTexture);
        }
    }

    static void InitTextureDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_TEXTURE);

        TextureDocument* impl = static_cast<TextureDocument*>(doc);

        impl->bounds = Bounds2{Vec2{-0.5f, -0.5f}, Vec2{0.5f, 0.5f}};
        impl->scale = 1.0f;
        impl->vtable = {
            .destructor = DestroyTextureDocument,
            .reload = ReloadTextureDocument,
            .post_load = PostLoadTextureDocument,
            .load_metadata = LoadTextureDocument,
            .save_metadata = SaveTextureDocumentMeta,
            .draw = DrawTextureDocument,
            .clone = CloneTextureDocument,
        };
    }

    void InitTextureDocumentDef() {
        InitDocumentDef({
            .type=ASSET_TYPE_TEXTURE,
            .size=sizeof(TextureDocument),
            .ext=".png",
            .init_func = InitTextureDocument,
            .import_func = ImportTextureDocument
        });
    }
}
