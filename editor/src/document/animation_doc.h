//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    struct SkeletonDocument;
    struct EventDocument;

    struct AnimationBoneData {
        const Name* name;
        int index;
        bool selected;
        Transform saved_transform;
    };

    struct AnimationFrameData {
        Transform transforms[MAX_BONES];
        EventDocument* event;
        const Name* event_name;
        int hold;
    };

    struct AnimationDocument : Document {
        AnimationBoneData bones[MAX_BONES];
        AnimationFrameData frames[MAX_ANIMATION_FRAMES];
        Animator animator;
        Skin skins[SKIN_MAX];
        const Name* skeleton_name;
        SkeletonDocument* skeleton;
        Animation* animation;
        int frame_count;
        int current_frame;
        int bone_count;
        int selected_bone_count;
        int skin_count;
        AnimationFlags flags;
    };

    extern void InitAnimationDocument(AnimationDocument* doc);
    extern AnimationDocument* NewAnimationDocument(const std::filesystem::path& path);
    extern void PostLoadDocument(AnimationDocument* doc);
    extern void UpdateBounds(AnimationDocument* n);
    extern void Serialize(AnimationDocument* n, Stream* stream, SkeletonDocument* s);
    extern Animation* ToAnimation(Allocator* allocator, AnimationDocument* n);
    extern int InsertFrame(AnimationDocument* n, int insert_at);
    extern int DeleteFrame(AnimationDocument* n, int frame_index);
    extern Transform& GetFrameTransform(AnimationDocument* n, int bone_index, int frame_index);
    extern void UpdateTransforms(AnimationDocument* n, int frame_index=-1);
    extern void UpdateSkeleton(AnimationDocument* n);
    extern void DrawAnimationData(Document* doc);
    extern int HitTestBones(AnimationDocument* n, const Mat3& transform, const Vec2& position, int* bones, int max_bones=MAX_BONES);
    extern int HitTestBone(AnimationDocument* n, const Mat3& transform, const Vec2& position);
    extern int GetFrameCountWithHolds(AnimationDocument* n);
    extern int GetFrameIndexWithHolds(AnimationDocument* n, int frame_index);
    extern int GetRealFrameIndex(AnimationDocument* n, int frame_index);
    inline bool IsRootMotion(AnimationDocument* n) { return (n->flags & ANIMATION_FLAG_ROOT_MOTION) != 0; }
    inline bool IsLooping(AnimationDocument* n) { return (n->flags & ANIMATION_FLAG_LOOPING) != 0; }
    extern void SetLooping(AnimationDocument* n, bool looping);
}