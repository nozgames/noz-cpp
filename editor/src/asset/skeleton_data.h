//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma  once

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

struct SkeletonDataImpl {
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

struct SkeletonData : AssetData {
    SkeletonDataImpl* impl;
};

extern void InitSkeletonData(AssetData* a);
extern AssetData* NewEditorSkeleton(const std::filesystem::path& path);
extern AssetData* CreateEditorSkeletonAsset(const std::filesystem::path& path, SkeletonData* skeleton);
extern void DrawSkeletonData(SkeletonData* s, const Vec2& position);
extern int HitTestBones(SkeletonData* s, const Mat3& transform, const Vec2& position, int* bones, int max_bones=MAX_BONES);
extern int HitTestBone(SkeletonData* s, const Vec2& world_pos);
extern int HitTestBone(SkeletonData* s, const Mat3& transform, const Vec2& world_pos);
extern void UpdateTransforms(SkeletonData* s);
extern void PostLoadEditorAssets(SkeletonData* s);
extern int FindBoneIndex(SkeletonData* s, const Name* name);
extern void Serialize(SkeletonData* s, Stream* stream);
extern Skeleton* ToSkeleton(Allocator* allocator, SkeletonData* s);
extern int ReparentBone(SkeletonData* s, int bone_index, int parent_index);
extern const Name* GetUniqueBoneName(SkeletonData* s);
extern void RemoveBone(SkeletonData* s, int bone_index);
extern int GetMirrorBone(SkeletonData* s, int bone_index);

inline BoneData* GetParent(SkeletonData* es, BoneData* eb) {
    return eb->parent_index >= 0 ? &es->impl->bones[eb->parent_index] : nullptr;
}

inline Mat3 GetParentLocalToWorld(SkeletonData* es, BoneData* eb, const Mat3& default_local_to_world) {
    return eb->parent_index >= 0 ? es->impl->bones[eb->parent_index].local_to_world : default_local_to_world;
}
