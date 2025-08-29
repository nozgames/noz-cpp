//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <filesystem>
#include "props.h"

struct AssetImporterTraits
{
    const char* type_name;           // e.g. "shader", "texture", "mesh"
    type_t type;                     // e.g. TYPE_SHADER, TYPE_TEXTURE, TYPE_MESH
    asset_signature_t signature;     // e.g. ASSET_SIGNATURE_SHADER, ASSET_SIGNATURE_TEXTURE
    const char** file_extensions;    // NULL-terminated array of supported extensions (e.g. {".png", ".jpg", NULL})
    void (*import_func) (const std::filesystem::path& source_path, Stream* output_stream, Props* config, Props* meta_props);
    bool (*does_depend_on) (const std::filesystem::path& source_path, const std::filesystem::path& dependency_path);
};
