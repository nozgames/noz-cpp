/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
	FollowBone::FollowBone()
		: _animator(nullptr)
		, _boneIndex(-1)
		, _followPosition(true)
		, _followRotation(true)
		, _followScale(false)
	{
	}

	void FollowBone::start()
	{
		Node::start();
		
		assert(dynamic_cast<Node3d*>(parent().get()));
	}

	void FollowBone::follow(const std::shared_ptr<Animator>& animator, const std::string& boneName)
	{
		_animator = animator;
		_boneIndex = -1;

		if (animator == nullptr)
			return;
		
		assert(animator->skeleton());
		assert(boneName.size() > 0);

		_boneName = boneName;
		_boneIndex = animator->skeleton()->boneIndex(boneName);
	}

	void FollowBone::lateUpdate()
	{
		Node::lateUpdate();
		
		updateTransformFromBone();
	}

	void FollowBone::updateTransformFromBone()
	{
		if (!_animator || _boneIndex < 0)
			return;
			
		auto* parentNode = (Node3d*)parent().get();
		assert(parentNode);
			
		// Get the bone's local transform
		auto& boneTransform = _animator->boneTransform(_boneIndex);
		
		// Calculate bone world transform: animator world transform * bone local transform
		auto animatorWorldTransform = _animator->parent<node::Node3d>()->localToWorld();
		auto boneWorldTransform = animatorWorldTransform * boneTransform;

		// Extract world position from bone transform
		if (_followPosition)
			parentNode->setPosition(glm::vec3(boneWorldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));

		// Extract world rotation from bone transform
		if (_followRotation)
			parentNode->setRotation(glm::quat_cast(boneWorldTransform));

		// Extract world scale from bone transform
		if (_followScale)
		{
			glm::vec3 boneWorldScale;
			boneWorldScale.x = glm::length(glm::vec3(boneWorldTransform[0]));
			boneWorldScale.y = glm::length(glm::vec3(boneWorldTransform[1]));
			boneWorldScale.z = glm::length(glm::vec3(boneWorldTransform[2]));
			
			// Note: setScale doesn't exist yet, would need to be implemented similar to setPosition/setRotation
			// parentNode->setScale(boneWorldScale);
		}
	}
}