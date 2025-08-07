/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::node
{
	class Animator;

	class FollowBone : public Node
	{
	public:
		FollowBone();
		virtual ~FollowBone() = default;

		void follow(const std::shared_ptr<Animator>& animator, const std::string& boneName);

		// Configuration
		void setFollowPosition(bool follow) { _followPosition = follow; }
		void setFollowRotation(bool follow) { _followRotation = follow; }
		void setFollowScale(bool follow) { _followScale = follow; }

		// Getters
		const std::string& boneName() const { return _boneName; }
		bool followPosition() const { return _followPosition; }
		bool followRotation() const { return _followRotation; }
		bool followScale() const { return _followScale; }

		// Node lifecycle
		void start() override;
		void lateUpdate() override;


	private:

		void updateTransformFromBone();

		std::shared_ptr<Animator> _animator;
		std::string _boneName;
		int _boneIndex;
		bool _followPosition;
		bool _followRotation;
		bool _followScale;
	};
}