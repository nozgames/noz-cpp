/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
	NOZ_DEFINE_TYPEID(Animator)

    Animator::Animator()
    {
    }

	void Animator::start()
	{
		assert(_skeleton);

		Node::start();

		_renderer = parent()->child<MeshRenderer>();
		assert(_renderer);

		_renderer->boneTransforms().resize(_skeleton->boneCount());
	}

    void Animator::setSkeleton(const std::shared_ptr<noz::renderer::Skeleton>& skeleton)
    {
        _skeleton = skeleton;
		if (nullptr == _skeleton)
			return;

        _boneTransforms.resize(_skeleton->boneCount());
        _targetBoneTransforms.resize(_skeleton->boneCount());
		_boneMatrices.resize(_skeleton->boneCount());
    }

	void Animator::play(const std::shared_ptr<noz::renderer::IAnimation>& animation, bool loop, float blendTime)
	{
		if (!animation)
			return;
		
		// If we're already playing this animation, don't restart
		if (_currentAnimation == animation)
			return;
		
		// Set up target animation
		_targetAnimation = animation;
		_isLooping = loop;
		
		// Set up blending
		_blendTime = blendTime;
		_currentBlendTime = 0.0f;
		
		if (blendTime > 0.0f && _currentAnimation)
		{
			_isBlending = true;
		}
		else
		{
			// Immediate transition
			_currentAnimation = _targetAnimation;
			_targetAnimation.reset();
			_currentTime = 0.0f;
			_isBlending = false;
		}
	}

    void Animator::update()
    {
		assert(_renderer);

        Node::update();

        if (!_skeleton)
            return;

        float deltaTime = noz::Time::deltaTime();
        
        // Update animation time
        _currentTime += deltaTime * _animationSpeed;
        
        // Update animation blending
        updateAnimationBlending(deltaTime);
        
        if (_isBlending && _currentAnimation && _targetAnimation)
        {
            // Evaluate both animations and blend between them
            _currentAnimation->evaluate(_currentTime, deltaTime, _skeleton->bones(), _boneTransforms);
            _targetAnimation->evaluate(0.0f, deltaTime, _skeleton->bones(), _targetBoneTransforms);
            
            float blendWeight = _currentBlendTime / _blendTime;
            std::vector<noz::renderer::BoneTransform> blendedTransforms(_boneTransforms.size());
            blendBoneTransforms(_boneTransforms, _targetBoneTransforms, blendWeight, blendedTransforms);
            _boneTransforms = blendedTransforms;
        }
        else if (_currentAnimation)
        {
            // Single animation
            _currentAnimation->evaluate(_currentTime, deltaTime, _skeleton->bones(), _boneTransforms);
            
            // Handle looping
            if (!_isLooping && _currentAnimation->duration() > 0.0f)
            {
                _currentTime = std::min(_currentTime, _currentAnimation->duration());
            }
        }
        else if (_skeleton)
        {
            // Use bind pose (identity transforms)
            for (size_t i = 0; i < _boneTransforms.size(); ++i)
                _boneTransforms[i] = _skeleton->bones()[i].transform;
        }

        // Calculate world bone transforms
		calculateBoneMatrices();
    }

    void Animator::updateAnimationBlending(float deltaTime)
    {
        if (!_isBlending)
            return;
        
        // Update blend progress
        _currentBlendTime += deltaTime;
        
        // Check if blending is complete
        if (_currentBlendTime >= _blendTime)
        {
            _currentAnimation = _targetAnimation;
            _targetAnimation.reset();
            _currentTime = 0.0f;
            _isBlending = false;
        }
    }
    
    void Animator::blendBoneTransforms(
		const std::vector<noz::renderer::BoneTransform>& from, 
        const std::vector<noz::renderer::BoneTransform>& to, 
        float blendWeight, 
        std::vector<noz::renderer::BoneTransform>& outTransforms)
    {
        blendWeight = glm::clamp(blendWeight, 0.0f, 1.0f);
        
        for (size_t i = 0; i < outTransforms.size() && i < from.size() && i < to.size(); ++i)
        {
            outTransforms[i].position = glm::mix(from[i].position, to[i].position, blendWeight);
            outTransforms[i].scale = glm::mix(from[i].scale, to[i].scale, blendWeight);
            outTransforms[i].rotation = glm::slerp(from[i].rotation, to[i].rotation, blendWeight);
        }
    }

    void Animator::calculateBoneMatrices()
    {
		assert(_renderer);
		assert(_skeleton);
		assert(!_boneTransforms.empty());
		assert(!_boneMatrices.empty());

		auto& renderBoneMatrices = _renderer->boneTransforms();
		assert(_boneTransforms.size() == _skeleton->boneCount());
		assert(_boneMatrices.size() == _skeleton->boneCount());
		assert(renderBoneMatrices.size() == _skeleton->boneCount());

		for (size_t i = 0; i < _skeleton->boneCount() && i < _boneTransforms.size(); ++i)
        {
            const auto& bone = _skeleton->bones()[i];
            const auto& boneTransform = _boneTransforms[i];

			auto localBoneMatrix = boneTransform.localToWorld();

			if (bone.parentIndex != -1)
			{
				assert(bone.parentIndex < _boneMatrices.size());
				_boneMatrices[i] = _boneMatrices[bone.parentIndex] * localBoneMatrix;
			}
            else
				_boneMatrices[i] = localBoneMatrix;
        }
        
		// Apply bone bind poses after we are finished calculating the local transforms
        for (size_t i = 0; i < _boneMatrices.size(); ++i)
        {
            const auto& bone = _skeleton->bones()[i];
			renderBoneMatrices[i] = _boneMatrices[i] * bone.worldToLocal;
        }
    }
} 