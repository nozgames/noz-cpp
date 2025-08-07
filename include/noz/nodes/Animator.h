/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
	class IAnimation;
	class Skeleton;
	struct BoneTransform;
}

namespace noz::node
{
    class MeshRenderer;


    /**
     * @brief Node that handles animation logic and drives bone transforms
     * Works with MeshRenderer to provide animated mesh rendering
     */
    class Animator : public Node
    {
    public:

		NOZ_DECLARE_TYPEID(Animator, Node);

        Animator();
        virtual ~Animator() = default;

        // Animation control
        void setSkeleton(const std::shared_ptr<noz::renderer::Skeleton>& skeleton);
        void play(const std::shared_ptr<noz::renderer::IAnimation>& animation, bool loop = true, float blendTime = 0.1f);
		void setAnimationSpeed(float speed) { _animationSpeed = speed; }
        float animationSpeed() const { return _animationSpeed; }
        
        std::shared_ptr<noz::renderer::IAnimation> currentAnimation() const { return _currentAnimation; }
                
        // Node lifecycle
        void update() override;
        void start() override;
        
		const glm::mat4& boneTransform(int index) const { return _boneMatrices[index]; }

        std::shared_ptr<noz::renderer::Skeleton> skeleton() const { return _skeleton; }

    private:

		void calculateBoneMatrices();
        void updateAnimationBlending(float deltaTime);
        void blendBoneTransforms(const std::vector<noz::renderer::BoneTransform>& from, 
                               const std::vector<noz::renderer::BoneTransform>& to, 
                               float blendWeight, 
                               std::vector<noz::renderer::BoneTransform>& outTransforms);
        
        std::shared_ptr<noz::renderer::Skeleton> _skeleton;
        std::shared_ptr<noz::renderer::IAnimation> _currentAnimation;
        std::shared_ptr<noz::renderer::IAnimation> _targetAnimation;
        std::shared_ptr<MeshRenderer> _renderer;
		std::vector<noz::renderer::BoneTransform> _boneTransforms;
		std::vector<noz::renderer::BoneTransform> _targetBoneTransforms;
		std::vector<glm::mat4> _boneMatrices;
        
        float _currentTime = 0.0f;
        float _blendTime = 0.1f;
        float _currentBlendTime = 0.0f;
        bool _isBlending = false;
        bool _isLooping = true;
        float _animationSpeed = 1.0f;
    };
} 