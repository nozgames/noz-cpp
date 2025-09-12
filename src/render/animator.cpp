//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//


static void EvalulateFrame(Animator& animator)
{
#if 0
    auto impl = animation.impl();
    assert(!impl->tracks.empty());
    assert(!impl->data.empty());
    assert(!bones.empty());

    auto frame_time = time * FRAME_RATE;
    if (frame_time >= static_cast<float>(impl->frame_count))
        frame_time = fmod(frame_time, static_cast<float>(impl->frame_count));

    auto frame1 = static_cast<int>(frame_time);
    auto frame2 = (frame1 + 1) % impl->frame_count;
    auto t = frame_time - static_cast<float>(frame1);
	    auto frame_stride = impl->frame_stride;
	    auto data = impl->data.data();

    for (const animation_track& track : impl->tracks)
    {
        // Skip if bone index is invalid
        if (track.bone < 0 || track.bone >= static_cast<int>(bones.size()))
            continue;

        auto& bone_transform = transforms[track.bone];
        auto data_offset1 = static_cast<size_t>(frame1) * frame_stride + static_cast<size_t>(track.data_offset);
        auto data_offset2 = static_cast<size_t>(frame2) * frame_stride + static_cast<size_t>(track.data_offset);

        // Apply the interpolated transform data based on track type
        switch (track.type)
        {
        case animation_track_type::translation:
        {
            auto pos1 = (glm::vec3*)(data + data_offset1);
            auto pos2 = (glm::vec3*)(data + data_offset2);
            bone_transform.position = glm::mix(*pos1, *pos2, t);
            break;
        }

        case animation_track_type::rotation:
        {
            auto rot1 = (glm::quat*)(data + data_offset1);
            auto rot2 = (glm::quat*)(data + data_offset2);
            bone_transform.rotation = glm::slerp(*rot1, *rot2, t);
            break;
        }

        case animation_track_type::scale:
        {
            auto scale1 = (glm::vec3*)(data + data_offset1);
            auto scale2 = (glm::vec3*)(data + data_offset2);
            bone_transform.scale = glm::mix(*scale1, *scale2, t);
            break;
        }
        }
    }
#endif
}

void Init(Animator& animator, Skeleton* skeleton)
{
    animator.skeleton = skeleton;
    animator.animation = nullptr;
    animator.time = 0.0f;
    animator.speed = 1.0f;
    animator.last_frame = -1;
    for (int i=0; i<GetBoneCount(skeleton); i++)
        animator.bones[i] = MAT3_IDENTITY;
}

void Play(Animator& animator, Animation* animation, float speed)
{
    animator.animation = animation;
    animator.speed = speed;
    animator.time = 0.0f;
}

void Update(Animator& animator, float time_scale)
{
    animator.time += GetFrameTime() * time_scale * animator.speed;

}

