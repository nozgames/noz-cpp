//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    struct BoneData {
        const Name* name;
        int index;
        int parent_index;
        BoneTransform transform;
        Mat3 local_to_world;
        Mat3 world_to_local;
        float length;
        bool selected;
    };

    struct SkeletonDocument : Document {
        BoneData bones[MAX_BONES];
        Skin skins[SKIN_MAX];
        int bone_count;
        int skin_count;
        int selected_bone_count;
        float opacity;
        Mesh* display_mesh;
        bool display_mesh_dirty;
        int display_mesh_zoom_version;
        Vec2 display_mesh_position;
    };

    extern void InitSkeletonData(Document* doc);
    extern Document* NewSkeletonDocument(const std::filesystem::path& path);
    extern Document* CreateEditorSkeletonAsset(const std::filesystem::path& path, SkeletonDocument* skeleton);
    extern void DrawSkeletonData(SkeletonDocument* s, const Vec2& position);
    extern int HitTestBones(SkeletonDocument* s, const Mat3& transform, const Vec2& position, int* bones, int max_bones=MAX_BONES);
    extern int HitTestBone(SkeletonDocument* s, const Vec2& world_pos);
    extern int HitTestBone(SkeletonDocument* s, const Mat3& transform, const Vec2& world_pos);
    extern void UpdateTransforms(SkeletonDocument* s);
    extern void PostLoadEditorAssets(SkeletonDocument* s);
    extern int FindBoneIndex(SkeletonDocument* s, const Name* name);
    extern void Serialize(SkeletonDocument* s, Stream* stream);
    extern Skeleton* ToSkeleton(Allocator* allocator, SkeletonDocument* s);
    extern int ReparentBone(SkeletonDocument* s, int bone_index, int parent_index);
    extern const Name* GetUniqueBoneName(SkeletonDocument* s);
    extern void RemoveBone(SkeletonDocument* s, int bone_index);
    extern int GetMirrorBone(SkeletonDocument* s, int bone_index);

    inline BoneData* GetParent(SkeletonDocument* es, BoneData* eb) {
        return eb->parent_index >= 0 ? &es->bones[eb->parent_index] : nullptr;
    }

    inline Mat3 GetParentLocalToWorld(SkeletonDocument* doc, BoneData* eb, const Mat3& default_local_to_world) {
        return eb->parent_index >= 0 ? doc->bones[eb->parent_index].local_to_world : default_local_to_world;
    }
}
