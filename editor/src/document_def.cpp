//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "document/document_def.h"

namespace noz::editor {

    extern void InitBinDocumentDef();
    extern void InitEventDocumentDef();
    extern void InitFontDocumentDef();
    extern void InitShaderDocument();
    extern void InitTextureDocumentDef();
    extern void InitLuaDocumentDef();

    static DocumentDef g_document_defs[ASSET_TYPE_COUNT] = {};

    void InitDocumentDef(const DocumentDef& info) {
        assert(info.type >= 0 && info.type < ASSET_TYPE_COUNT);
        assert(g_document_defs[info.type].type == ASSET_TYPE_UNKNOWN);
        g_document_defs[info.type] = info;
    }

    const DocumentDef* GetDocumentDef(AssetType type) {
        return &g_document_defs[static_cast<int>(type)];
    }

    const DocumentDef* FindDocumentDef(const char* ext) {
        if (!ext) return nullptr;
        for (int i = 0; i < ASSET_TYPE_COUNT; i++) {
            if (g_document_defs[i].ext && Equals(g_document_defs[i].ext, ext))
                return &g_document_defs[i];
        }
        return nullptr;
    }

    void InitDocumentDefs() {
        RegisterDocument({ASSET_TYPE_MESH, InitMeshData, GetMeshImporter()});
        RegisterDocument({ASSET_TYPE_VFX, InitVfxDocument, GetVfxImporter()});
        RegisterDocument({ASSET_TYPE_SKELETON, InitSkeletonData, GetSkeletonImporter()});
        RegisterDocument({ASSET_TYPE_ANIMATION, InitAnimationData, GetAnimationImporter()});
        RegisterDocument({ASSET_TYPE_SPRITE, InitSpriteDocument, GetSpriteImporter()});
        RegisterDocument({ASSET_TYPE_SOUND, InitSoundDocument, GetSoundImporter()});
        RegisterDocument({ASSET_TYPE_TEXTURE, InitTextureData, GetTextureImporter()});
        RegisterDocument({ASSET_TYPE_ATLAS, InitAtlasDocument, GetAtlasImporter()});

        InitBinDocumentDef();
        InitEventDocumentDef();
        InitFontDocumentDef();
        InitLuaDocumentDef();
        InitShaderDocument();
        InitTextureDocumentDef();
    }
}
