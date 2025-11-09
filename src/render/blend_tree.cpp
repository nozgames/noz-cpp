//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr float ANIMATOR_BLEND_TIME = 0.05f;


void Play(BlendTree& blend_tree, int blend_index, float value, Animation* animation, float speed, bool loop) {
    assert(blend_index <= blend_tree.blend_count);
    assert(blend_index >= 0);

    BlendTreeBlend& blend = blend_tree.blends[blend_index];
    blend.value = value;
    Play(blend.animator, animation, speed, loop);
}

void SetValue(BlendTree& blend_tree, float value) {
    blend_tree.value = value;
}

void Update(BlendTree& blend_tree, float time_scale, Animator& animator) {
    for (int i=0; i<blend_tree.blend_count; i++)
        Update(blend_tree.blends[i].animator, time_scale);

    int bone_count = GetBoneCount(blend_tree.skeleton);
    for (int bone_index=0; bone_index<bone_count; bone_index++) {
        BoneTransform bt = {};
        for (int i=0; i<blend_tree.blend_count; i++) {
            BlendTreeBlend& blend = blend_tree.blends[i];
            float weight = Clamp(1.0f - Abs(blend.value - blend_tree.value), 0.0f, 1.0f);
            if (weight < 0.0f)
                continue;

            BoneTransform bone_transform = blend.animator.transforms[bone_index];
            bt.position += bone_transform.position * weight;
            bt.rotation += bone_transform.rotation * weight;
            bt.scale += bone_transform.scale * weight;
        }

        animator.transforms[bone_index] = bt;
        animator.bones[bone_index] = ToMat3(bt);
    }

    SkeletonImpl* skel_impl = static_cast<SkeletonImpl*>(animator.skeleton);
    for (int bone_index=1; bone_index<bone_count; bone_index++) {
        int parent_index = skel_impl->bones[bone_index].parent_index;
        animator.bones[bone_index] = animator.bones[parent_index] * animator.bones[bone_index];
    }
}

void Init(BlendTree& blend_tree, Skeleton* skeleton, int blend_count) {
    blend_tree.blend_count = blend_count;
    blend_tree.skeleton = skeleton;

    for (int i=0; i<blend_count; i++) {
        Init(blend_tree.blends[i].animator, skeleton);
    }
}
