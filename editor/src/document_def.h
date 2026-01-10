//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    struct Document;

    struct DocumentDef {
        AssetType type = ASSET_TYPE_UNKNOWN;
        int size;
        const char* ext;
        void (*init_func)(Document* doc);
        void (*import_func) (Document* doc, const std::filesystem::path& path, Props* config, Props* meta);
        bool (*check_dependency_func) (Document* doc, Document* dependency);
    };

    extern void InitDocumentDefs();
    extern void InitDocumentDef(const DocumentDef& info);
    extern const DocumentDef* GetDocumentDef(AssetType type);
    extern const DocumentDef* FindDocumentDef(const char* ext);

}
