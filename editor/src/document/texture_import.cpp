//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"

namespace fs = std::filesystem;

namespace noz::editor {

    extern void InitTextureDocument(noz::editor::TextureDocument*);

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

    static void ImportTextureDocument(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        (void)config;

        fs::path src_path = doc->path.value;

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
}
