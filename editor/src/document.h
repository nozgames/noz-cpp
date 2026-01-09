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
        void (*editor_rename)(Document* a, const Name* new_name);
        Bounds2 (*editor_bounds)();
    };

    struct Document {
        AssetType type;
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

    extern bool IsFile(Document* a);

    extern void InitDocument();
    extern void LoadDocument(Document* a);
    extern void PostLoadDocument(Document* a);
    extern void HotloadEditorAsset(AssetType type, const Name* name);
    extern void MarkModified(Document* a);
    extern void MarkMetaModified();
    extern void MarkMetaModified(Document* a);
    extern Document* CreateAssetData(const std::filesystem::path& path);
    extern std::filesystem::path GetEditorAssetPath(const Name* name, const char* ext);
    extern void Clone(Document* dst, Document* src);
    extern bool OverlapPoint(Document* a, const Vec2& overlap_point);
    extern bool OverlapPoint(Document* a, const Vec2& position, const Vec2& overlap_point);
    extern bool OverlapBounds(Document* a, const Bounds2& bounds);
    extern Document* HitTestAssets(const Vec2& overlap_point);
    extern Document* HitTestAssets(const Bounds2& bit_bounds);
    extern void DrawAsset(Document* a);
    extern Document* GetFirstSelectedAsset();
    extern void SetPosition(Document* a, const Vec2& position);
    extern void ClearAssetSelection();
    extern void SetSelected(Document* a, bool selected);
    extern void ToggleSelected(Document* a);
    extern bool InitImporter(Document* a);
    extern const Name* MakeCanonicalAssetName(const char* name);
    extern const Name* MakeCanonicalAssetName(const std::filesystem::path& path);
    extern void DeleteAsset(Document* a);
    extern void SortAssets();
    extern bool Rename(Document* a, const Name* new_name);
    extern Document* Duplicate(Document* a);
    extern void NewAsset(AssetType asset_type, const Name* asset_name, const Vec2* position = nullptr);
    extern std::filesystem::path GetTargetPath(Document* a);
    extern std::filesystem::path GetUniqueAssetPath(const std::filesystem::path& path);
    extern int GetSelectedAssets(Document** out_assets, int max_assets);
    inline bool IsEditing(Document* a) { return a->editing; }

    inline Bounds2 GetBounds(Document* a) { return a->bounds; }
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
