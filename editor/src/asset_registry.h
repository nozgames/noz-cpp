//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct AssetData;
struct AssetImporter;

constexpr int MAX_EDITOR_ASSET_TYPES = 128;

struct EditorAssetTypeInfo {
    int type_id;
    void (*init)(AssetData* a);
    AssetImporter importer;
};

extern void RegisterEditorAssetType(const EditorAssetTypeInfo& info);
extern const EditorAssetTypeInfo* GetEditorAssetTypeInfo(int type_id);
extern const EditorAssetTypeInfo* FindEditorAssetTypeByExtension(const char* ext);
extern int GetRegisteredEditorAssetTypeCount();
extern void InitEditorAssets();
