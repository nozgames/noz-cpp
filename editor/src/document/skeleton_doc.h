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

    extern Document* NewSkeletonDocument(const std::filesystem::path& path);
    extern Document* CreateEditorSkeletonAsset(const std::filesystem::path& path, SkeletonDocument* skeleton);
    extern void DrawSkeletonDocument(SkeletonDocument* sdoc, const Vec2& position);
    extern int HitTestBones(SkeletonDocument* sdoc, const Mat3& transform, const Vec2& position, int* bones, int max_bones=MAX_BONES);
    extern int HitTestBone(SkeletonDocument* sdoc, const Vec2& world_pos);
    extern int HitTestBone(SkeletonDocument* sdoc, const Mat3& transform, const Vec2& world_pos);
    extern void UpdateTransforms(SkeletonDocument* sdoc);
    extern void PostLoadEditorAssets(SkeletonDocument* sdoc);
    extern int FindBoneIndex(SkeletonDocument* sdoc, const Name* name);
    extern void Serialize(SkeletonDocument* sdoc, Stream* stream);
    extern Skeleton* ToSkeleton(Allocator* allocator, SkeletonDocument* sdoc);
    extern int ReparentBone(SkeletonDocument* sdoc, int bone_index, int parent_index);
    extern const Name* GetUniqueBoneName(SkeletonDocument* sdoc);
    extern void RemoveBone(SkeletonDocument* sdoc, int bone_index);
    extern int GetMirrorBone(SkeletonDocument* sdoc, int bone_index);

    inline BoneData* GetParent(SkeletonDocument* sdoc, BoneData* eb) {
        return eb->parent_index >= 0 ? &sdoc->bones[eb->parent_index] : nullptr;
    }

    inline Mat3 GetParentLocalToWorld(SkeletonDocument* sdoc, BoneData* eb, const Mat3& default_local_to_world) {
        return eb->parent_index >= 0 ? sdoc->bones[eb->parent_index].local_to_world : default_local_to_world;
    }
}
