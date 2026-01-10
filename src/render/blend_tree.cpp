//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz {

    constexpr float ANIMATOR_BLEND_TIME = 0.05f;

    extern void AddEvent(Animator& animator, int event_id);

    void Play(BlendTree& blend_tree, int blend_index, float value, Animation* animation, float speed, float normalized_time) {
        assert(blend_index <= blend_tree.blend_count);
        assert(blend_index >= 0);

        BlendTreeBlend& blend = blend_tree.blends[blend_index];
        blend.value = value;
        Play(blend.animator, animation, 0, speed, normalized_time);
    }

    void SetValue(BlendTree& blend_tree, float value) {
        blend_tree.value = value;
    }

    void Update(BlendTree& blend_tree, float time_scale, Animator& animator) {
        if (blend_tree.blend_count == 0)
            return;

        for (int i=0; i<blend_tree.blend_count; i++) {
            Animator& blend_animator = blend_tree.blends[i].animator;
            Update(blend_animator, time_scale);

            while (HasEvents(blend_animator))
                AddEvent(animator, GetEvent(blend_animator));
        }

        int bone_count = GetBoneCount(blend_tree.skeleton);
        for (int bone_index=0; bone_index<bone_count; bone_index++) {
            if ((blend_tree.bone_mask & (static_cast<u64>(1) << static_cast<u64>(bone_index))) == 0)
                continue;

            BoneTransform bt = {};
            float root_motion_delta = 0.0f;
            float base_rotation = 0.0f;
            bool first = true;
            for (int i=0; i<blend_tree.blend_count; i++) {
                BlendTreeBlend& blend = blend_tree.blends[i];
                float weight = Clamp(1.0f - Abs(blend.value - blend_tree.value), 0.0f, 1.0f);
                if (weight < 0.0f)
                    continue;

                BoneTransform bone_transform = blend.animator.transforms[bone_index];
                bt.position += bone_transform.position * weight;

                // Blend rotation using shortest angle
                if (first) {
                    base_rotation = bone_transform.rotation;
                    bt.rotation = bone_transform.rotation * weight;
                    first = false;
                } else {
                    float angle_diff = AngleDelta(base_rotation, bone_transform.rotation);
                    bt.rotation += (base_rotation + angle_diff) * weight;
                }

                bt.scale += bone_transform.scale * weight;

                if (bone_index == 0)
                    root_motion_delta += blend.animator.root_motion_delta * weight;
            }

            animator.transforms[bone_index] = bt;

            if (bone_index == 0)
                animator.root_motion_delta = root_motion_delta;
        }

        bool playing = true;
        for (int i=0; playing && i<blend_tree.blend_count; i++) {
            Animator& blend_animator = blend_tree.blends[i].animator;
            playing &= blend_animator.layers[0].playing;
        }

        Update(animator, 0.0f);

        animator.layers[0].playing = playing;
    }

    void Stop(BlendTree& blend_tree) {
        for (int i=0; i<blend_tree.blend_count; i++) {
            Stop(blend_tree.blends[i].animator);
        }
    }

    void Init(BlendTree& blend_tree, Skeleton* skeleton, int blend_count, u64 bone_mask) {
        blend_tree.blend_count = blend_count;
        blend_tree.skeleton = skeleton;
        blend_tree.bone_mask = bone_mask;

        for (int i=0; i<blend_count; i++) {
            Init(blend_tree.blends[i].animator, skeleton, 1);
            SetBoneMask(blend_tree.blends[i].animator, 0, bone_mask);
        }
    }
}
