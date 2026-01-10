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

    extern AnimationDocument* NewAnimationDocument(const std::filesystem::path& path);
    extern void PostLoadDocument(AnimationDocument* adoc);
    extern void UpdateBounds(AnimationDocument* adoc);
    extern void Serialize(AnimationDocument* adoc, Stream* stream, SkeletonDocument* s);
    extern Animation* ToAnimation(Allocator* allocator, AnimationDocument* adoc);
    extern int InsertFrame(AnimationDocument* adoc, int insert_at);
    extern int DeleteFrame(AnimationDocument* adoc, int frame_index);
    extern Transform& GetFrameTransform(AnimationDocument* adoc, int bone_index, int frame_index);
    extern void UpdateTransforms(AnimationDocument* adoc, int frame_index=-1);
    extern void UpdateSkeleton(AnimationDocument* adoc);
    extern void DrawAnimationData(Document* doc);
    extern int HitTestBones(AnimationDocument* adoc, const Mat3& transform, const Vec2& position, int* bones, int max_bones=MAX_BONES);
    extern int HitTestBone(AnimationDocument* adoc, const Mat3& transform, const Vec2& position);
    extern int GetFrameCountWithHolds(AnimationDocument* adoc);
    extern int GetFrameIndexWithHolds(AnimationDocument* adoc, int frame_index);
    extern int GetRealFrameIndex(AnimationDocument* adoc, int frame_index);
    inline bool IsRootMotion(AnimationDocument* adoc) { return (adoc->flags & ANIMATION_FLAG_ROOT_MOTION) != 0; }
    inline bool IsLooping(AnimationDocument* adoc) { return (adoc->flags & ANIMATION_FLAG_LOOPING) != 0; }
    extern void SetLooping(AnimationDocument* adoc, bool looping);
}