/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
	static int s_boneTransformDataOffsets[3] =
	{
		0,
		3,
		7
	};

    glm::mat4 BoneTransform::localToWorld() const
    {
		return
			glm::translate(position) *
			glm::mat4_cast(rotation) *
			glm::scale(scale);
    }

    float* BoneTransform::get(AnimationTrackType trackType)
    {
		return &position.x + s_boneTransformDataOffsets[static_cast<int>(trackType)];
    }
}
