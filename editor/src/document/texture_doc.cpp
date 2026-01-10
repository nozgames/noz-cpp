//
//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"

namespace noz::editor {
    extern void InitTextureEditor(TextureDocument*);
    extern void ImportTexture(Document* doc, const std::filesystem::path& path, Props* config, Props* meta);

    static void CloneTextureDocument(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_TEXTURE);
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
        assert(doc->def->type == ASSET_TYPE_TEXTURE);

        TextureDocument* impl = static_cast<TextureDocument*>(doc);
        if (!impl->texture) return;

        BindDepth(-0.1f);
        BindColor(COLOR_WHITE);
        BindShader(SHADER_EDITOR_TEXTURE);
        BindTexture(impl->texture);
        DrawMesh(g_workspace.quad_mesh, Translate(doc->position) * Scale(Vec2{GetSize(impl->bounds).x, -GetSize(impl->bounds).y}));
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
        assert(doc->def->type == ASSET_TYPE_TEXTURE);
        TextureDocument* d = static_cast<TextureDocument*>(doc);
        d->editor_only = meta->GetBool("texture", "reference", false) || Contains(doc->path, "reference", true);
        d->scale = meta->GetFloat("editor", "scale", 1.0f);
        InitTextureEditor(d);
        UpdateBounds(d);
    }

    static void SaveTextureDocumentMeta(Document* doc, Props* meta) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_TEXTURE);
        TextureDocument* d = static_cast<TextureDocument*>(doc);
        meta->SetString("editor", "scale", std::to_string(d->scale).c_str());
        meta->SetBool("texture", "reference", d->editor_only);
    }

    void PostLoadTextureDocument(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_TEXTURE);
        TextureDocument* d = static_cast<TextureDocument*>(doc);
        d->texture = static_cast<Texture*>(LoadAssetInternal(ALLOCATOR_DEFAULT, doc->name, ASSET_TYPE_TEXTURE, LoadTexture));
        UpdateBounds(d);
    }

    static void ReloadTextureDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_TEXTURE);

        TextureDocument* d = static_cast<TextureDocument*>(doc);
        if (!d->texture) {
            LoadDocument(doc);
            PostLoadDocument(doc);
        } else {
            ReloadAsset(doc->name, ASSET_TYPE_TEXTURE, d->texture, ReloadTexture);
        }
    }

        static void WriteTextureData(
        Stream* stream,
        const uint8_t* data,
        int width,
        int height,
        int channels,
        const std::string& filter,
        const std::string& clamp) {

        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_TEXTURE;
        header.version = 1;
        header.flags = ASSET_FLAG_NONE;
        WriteAssetHeader(stream, &header);

        TextureFilter filter_value = filter == "nearest" || filter == "point"
            ? TEXTURE_FILTER_NEAREST
            : TEXTURE_FILTER_LINEAR;

        TextureClamp clamp_value = clamp == "repeat" ?
            TEXTURE_CLAMP_REPEAT :
            TEXTURE_CLAMP_CLAMP;

        TextureFormat format = TEXTURE_FORMAT_RGBA8;
        WriteU8(stream, (u8)format);
        WriteU8(stream, (u8)filter_value);
        WriteU8(stream, (u8)clamp_value);
        WriteU32(stream, width);
        WriteU32(stream, height);
        WriteBytes(stream, data, width * height * channels);
    }

    static void InitTextureDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_TEXTURE);

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

    void ImportTexture(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        (void)config;

        std::filesystem::path src_path = doc->path.value;

        int width;
        int height;
        int channels;
        unsigned char* image_data = stbi_load(src_path.string().c_str(), &width, &height, &channels, 0);

        if (!image_data)
            throw std::runtime_error("Failed to load texture file");

        std::string filter = meta->GetString("texture", "filter", "linear");
        std::string clamp = meta->GetString("texture", "clamp", "clamp");
        //bool convert_from_srgb = meta->GetBool("texture", "srgb", false);

        std::vector<uint8_t> rgba_data;
        if (channels != 4) {
            rgba_data.resize(width * height * 4);
            for (int i = 0; i < width * height; ++i) {
                for (int c = 0; c < 3; ++c)
                    rgba_data[i * 4 + c] = (c < channels) ? image_data[i * channels + c] : 0;
                rgba_data[i * 4 + 3] = (channels == 4) ? image_data[i * channels + 3] : 255; // Alpha
            }
            channels = 4;
        } else {
            rgba_data.assign(image_data, image_data + (width * height * channels));
        }

        stbi_image_free(image_data);

        // if (convert_from_srgb)
        //     ConvertSRGBToLinear(rgba_data.data(), width, height, channels);

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        WriteTextureData(
            stream,
            rgba_data.data(),
            width,
            height,
            channels,
            filter,
            clamp
        );
        SaveStream(stream, path);
        Free(stream);


        if (Contains(doc->path, "reference", true)) {
            // Only mark meta_modified if the value is actually changing
            // to avoid a save->reimport->save loop via file watcher
            if (!doc->editor_only) {
                doc->editor_only = true;
                MarkMetaModified(doc);
            }
            InitTextureDocument(static_cast<TextureDocument*>(doc));
        }
    }

    void InitTextureDocumentDef() {
        InitDocumentDef({
            .type=ASSET_TYPE_TEXTURE,
            .size=sizeof(TextureDocument),
            .ext=".png",
            .init_func = InitTextureDocument,
            .import_func = ImportTexture
        });
    }
}
