//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <cstring>

#include "asset_registry.h"
#include "import/asset_importer.h"
#include "asset/asset_data.h"

// Forward declarations for Init*Data functions
extern void InitMeshData(AssetData* a);
extern void InitTextureData(AssetData* a);
extern void InitVfxData(AssetData* a);
extern void InitAnimationData(AssetData* a);
extern void InitSkeletonData(AssetData* a);
extern void InitShaderData(AssetData* a);
extern void InitSoundData(AssetData* a);
extern void InitFontData(AssetData* a);
extern void InitAnimatedMeshData(AssetData* a);
extern void InitEventData(AssetData* a);
extern void InitBinData(AssetData* a);
extern void InitLuaData(AssetData* a);
extern void InitAtlasData(AssetData* a);

// Forward declarations for Get*Importer functions
extern AssetImporter GetMeshImporter();
extern AssetImporter GetTextureImporter();
extern AssetImporter GetVfxImporter();
extern AssetImporter GetAnimationImporter();
extern AssetImporter GetSkeletonImporter();
extern AssetImporter GetShaderImporter();
extern AssetImporter GetSoundImporter();
extern AssetImporter GetFontImporter();
extern AssetImporter GetAnimatedMeshImporter();
extern AssetImporter GetEventImporter();
extern AssetImporter GetBinImporter();
extern AssetImporter GetLuaImporter();
extern AssetImporter GetAtlasImporter();

// @editor_asset_registry
static EditorAssetTypeInfo g_editor_asset_types[MAX_EDITOR_ASSET_TYPES] = {};
static int g_editor_asset_type_count = 0;

void RegisterEditorAssetType(const EditorAssetTypeInfo& info) {
    assert(g_editor_asset_type_count < MAX_EDITOR_ASSET_TYPES);

    // Check for duplicate type_id
    for (int i = 0; i < g_editor_asset_type_count; i++) {
        if (g_editor_asset_types[i].type_id == info.type_id) {
            assert(false && "Duplicate editor asset type_id registered");
            return;
        }
    }

    g_editor_asset_types[g_editor_asset_type_count++] = info;
}

const EditorAssetTypeInfo* GetEditorAssetTypeInfo(int type_id) {
    for (int i = 0; i < g_editor_asset_type_count; i++) {
        if (g_editor_asset_types[i].type_id == type_id)
            return &g_editor_asset_types[i];
    }
    return nullptr;
}

const EditorAssetTypeInfo* FindEditorAssetTypeByExtension(const char* ext) {
    if (!ext) return nullptr;
    for (int i = 0; i < g_editor_asset_type_count; i++) {
        if (g_editor_asset_types[i].importer.ext && strcmp(g_editor_asset_types[i].importer.ext, ext) == 0)
            return &g_editor_asset_types[i];
    }
    return nullptr;
}

int GetRegisteredEditorAssetTypeCount() {
    return g_editor_asset_type_count;
}

void InitEditorAssets() {
    // Register all built-in editor asset types
    RegisterEditorAssetType({ASSET_TYPE_MESH, InitMeshData, GetMeshImporter()});
    RegisterEditorAssetType({ASSET_TYPE_VFX, InitVfxData, GetVfxImporter()});
    RegisterEditorAssetType({ASSET_TYPE_SKELETON, InitSkeletonData, GetSkeletonImporter()});
    RegisterEditorAssetType({ASSET_TYPE_ANIMATION, InitAnimationData, GetAnimationImporter()});
    RegisterEditorAssetType({ASSET_TYPE_SOUND, InitSoundData, GetSoundImporter()});
    RegisterEditorAssetType({ASSET_TYPE_TEXTURE, InitTextureData, GetTextureImporter()});
    RegisterEditorAssetType({ASSET_TYPE_FONT, InitFontData, GetFontImporter()});
    RegisterEditorAssetType({ASSET_TYPE_SHADER, InitShaderData, GetShaderImporter()});
    RegisterEditorAssetType({ASSET_TYPE_ANIMATED_MESH, InitAnimatedMeshData, GetAnimatedMeshImporter()});
    RegisterEditorAssetType({ASSET_TYPE_EVENT, InitEventData, GetEventImporter()});
    RegisterEditorAssetType({ASSET_TYPE_BIN, InitBinData, GetBinImporter()});
    RegisterEditorAssetType({ASSET_TYPE_LUA, InitLuaData, GetLuaImporter()});
    RegisterEditorAssetType({ASSET_TYPE_ATLAS, InitAtlasData, GetAtlasImporter()});
}
