/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
    enum class AnimationTrackType : uint8_t
    {
        Translation = 0,  // position (vec3)
        Rotation = 1,     // rotation (quat)
        Scale = 2         // scale (vec3)
    };

    struct BoneTransform
    {
        glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        glm::mat4 localToWorld() const;
        
        float* get(AnimationTrackType trackType);
        const float* get(AnimationTrackType trackType) const;
    };

	inline const float* BoneTransform::get(AnimationTrackType trackType) const
	{
		return get(trackType);
	}
}