//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "document_def.h"

namespace noz::editor {

    extern void InitAnimationDocumentDef();
    extern void InitAtasDocumentDef();
    extern void InitBinDocumentDef();
    extern void InitEventDocumentDef();
    extern void InitFontDocumentDef();
    extern void InitLuaDocumentDef();
    extern void InitMeshDocumentDef();
    extern void InitShaderDocumentDef();
    extern void InitSkeletonDocumentDef();
    extern void InitSoundDocumentDef();
    extern void InitTextureDocumentDef();
    extern void InitVfxDocumentDef();

    DocumentDef g_document_defs[ASSET_TYPE_COUNT] = {};

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
        InitAnimationDocumentDef();
        InitAtasDocumentDef();
        InitBinDocumentDef();
        InitEventDocumentDef();
        InitFontDocumentDef();
        InitLuaDocumentDef();
        InitMeshDocumentDef();
        InitShaderDocumentDef();
        InitSkeletonDocumentDef();
        InitSoundDocumentDef();
        InitTextureDocumentDef();
        InitVfxDocumentDef();

#if !defined(NDEBUG)
        for (int i = 0; i < ASSET_TYPE_COUNT; i++) {
            assert(g_document_defs[i].type != ASSET_TYPE_UNKNOWN && "No DocumentDef registered for AssetType");
            assert(g_document_defs[i].size > 0 && "DocumentDef size not set");
        }
#endif
    }
}
