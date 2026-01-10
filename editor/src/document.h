//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct Animation;

namespace noz::editor {

    struct Document;
    struct MeshDocument;

    constexpr int SKIN_MAX = 64;

    struct Skin {
        const Name* asset_name;
        MeshDocument* mesh;
        Animation* animation;;
    };

    struct DocumentVtable {
        void (*destructor)(Document* doc);
        void (*load)(Document* doc);
        void (*reload)(Document* doc);
        void (*post_load)(Document* doc);
        void (*save)(Document* doc, const std::filesystem::path& path);
        void (*load_metadata)(Document* doc, Props* meta);
        void (*save_metadata)(Document* doc, Props* meta);
        void (*draw)(Document* doc);
        void (*play)(Document* doc);
        void (*clone)(Document* doc);
        void (*undo_redo)(Document* doc);

        void (*editor_begin)(Document* doc);
        void (*editor_end)();
        void (*editor_update)();
        void (*editor_draw)();
        void (*editor_overlay)();
        void (*editor_help)();
        void (*editor_context_menu)();
        void (*editor_rename)(Document* doc, const Name* new_name);
        Bounds2 (*editor_bounds)();
    };

    struct DocumentDef;

    struct Document {
        const DocumentDef* def;
        int asset_path_index;
        const Name* name;
        String1024 path;
        Vec2 position;
        Vec2 saved_position;
        bool selected;
        bool editing;
        bool modified;
        bool meta_modified;
        bool clipped;
        bool loaded;
        bool post_loaded;
        bool editor_only;
        DocumentVtable vtable;
        Bounds2 bounds;
    };

    extern bool IsFile(Document* doc);

    extern void InitDocuments();
    extern void LoadDocument(Document* doc);
    extern Document* Clone(Document* doc);
    extern void CloneInto(Document* dst, Document* src);
    extern void PostLoadDocument(Document* doc);
    extern void HotloadEditorAsset(AssetType type, const Name* name);
    extern void MarkModified(Document* doc);
    extern void MarkMetaModified();
    extern void MarkMetaModified(Document* doc);
    extern Document* CreateDocument(const std::filesystem::path& path);
    extern std::filesystem::path GetEditorAssetPath(const Name* name, const char* ext);
    extern bool OverlapPoint(Document* doc, const Vec2& overlap_point);
    extern bool OverlapPoint(Document* doc, const Vec2& position, const Vec2& overlap_point);
    extern bool OverlapBounds(Document* doc, const Bounds2& bounds);
    extern Document* HitTestAssets(const Vec2& overlap_point);
    extern Document* HitTestAssets(const Bounds2& bit_bounds);
    extern void DrawAsset(Document* doc);
    extern Document* GetFirstSelectedAsset();
    extern void SetPosition(Document* doc, const Vec2& position);
    extern void ClearAssetSelection();
    extern void SetSelected(Document* doc, bool selected);
    extern void ToggleSelected(Document* doc);
    extern const Name* MakeCanonicalAssetName(const char* name);
    extern const Name* MakeCanonicalAssetName(const std::filesystem::path& path);
    extern void DeleteAsset(Document* doc);
    extern bool Rename(Document* doc, const Name* new_name);
    extern Document* Duplicate(Document* doc);
    extern void NewAsset(AssetType asset_type, const Name* asset_name, const Vec2* position = nullptr);
    extern std::filesystem::path GetTargetPath(Document* doc);
    extern std::filesystem::path GetUniqueAssetPath(const std::filesystem::path& path);
    extern int GetSelectedAssets(Document** out_assets, int max_assets);
    inline bool IsEditing(Document* doc) { return doc->editing; }
    inline Bounds2 GetBounds(Document* doc) { return doc->bounds; }
}

#include "document/animation_doc.h"
#include "document/atlas_doc.h"
#include "document/bin_doc.h"
#include "document/event_doc.h"
#include "document/font_doc.h"
#include "document/lua_doc.h"
#include "document/mesh_doc.h"
#include "document/skeleton_doc.h"
#include "document/shader_doc.h"
#include "document/sound_doc.h"
#include "document/texture_doc.h"
#include "document/vfx_doc.h"
