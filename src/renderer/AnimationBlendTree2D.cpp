/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    NOZ_DEFINE_TYPEID(AnimationBlendTree2d)

    AnimationBlendTree2d::AnimationBlendTree2d()
        : _blendX(0.0f)
        , _blendY(0.0f)
    {
    }

    AnimationBlendTree2d::~AnimationBlendTree2d()
    {
    }

    void AnimationBlendTree2d::initialize(const std::string& name)
    {
        Asset::initialize(name);
	}

    void AnimationBlendTree2d::initialize(const std::shared_ptr<Skeleton>& skeleton)
    {
        // Initialize with the skeleton if needed
        // This can be used to set up any internal state based on the skeleton structure
        // For now, we don't need to do anything specific here
	}

    void AnimationBlendTree2d::setBlendParameters(float x, float y)
    {
        _blendX = glm::clamp(x, -1.0f, 1.0f);
        _blendY = glm::clamp(y, -1.0f, 1.0f);
    }

    void AnimationBlendTree2d::setCenterAnimation(const std::shared_ptr<IAnimation>& animation)
    {
        _centerAnimation = animation;
    }

    void AnimationBlendTree2d::setLeftAnimation(const std::shared_ptr<IAnimation>& animation)
    {
        _leftAnimation = animation;
    }

    void AnimationBlendTree2d::setRightAnimation(const std::shared_ptr<IAnimation>& animation)
    {
        _rightAnimation = animation;
    }

    void AnimationBlendTree2d::setTopAnimation(const std::shared_ptr<IAnimation>& animation)
    {
        _topAnimation = animation;
    }

    void AnimationBlendTree2d::setBottomAnimation(const std::shared_ptr<IAnimation>& animation)
    {
        _bottomAnimation = animation;
    }

    void AnimationBlendTree2d::evaluate(float time, float deltaTime, const std::vector<Bone>& bones, std::vector<BoneTransform>& outTransforms)
    {
        outTransforms.resize(bones.size());
        
        // If no animations are set, use bind pose
        if (!_centerAnimation && !_leftAnimation && !_rightAnimation && !_topAnimation && !_bottomAnimation)
        {
            for (size_t i = 0; i < bones.size(); ++i)
                outTransforms[i] = bones[i].transform;
            return;
        }

        // Start with center animation if available, otherwise use bind pose
        if (_centerAnimation)
        {
            _centerAnimation->evaluate(time, deltaTime, bones, outTransforms);
        }
        else
        {
            for (size_t i = 0; i < bones.size(); ++i)
                outTransforms[i] = bones[i].transform;
        }

        // Blend horizontal (left/right)
        if (_blendX < 0.0f && _leftAnimation)
        {
            // Blend towards left
            _leftAnimation->evaluate(time, deltaTime, bones, _tempTransforms1);
            float blendWeight = -_blendX; // Convert -1..0 to 1..0
            blendTransforms(outTransforms, _tempTransforms1, blendWeight, _tempTransforms2);
            outTransforms = _tempTransforms2;
        }
        else if (_blendX > 0.0f && _rightAnimation)
        {
            // Blend towards right
            _rightAnimation->evaluate(time, deltaTime, bones, _tempTransforms1);
            float blendWeight = _blendX; // 0..1 stays 0..1
            blendTransforms(outTransforms, _tempTransforms1, blendWeight, _tempTransforms2);
            outTransforms = _tempTransforms2;
        }

        // Blend vertical (top/bottom) - this blends on top of the horizontal result
        if (_blendY < 0.0f && _bottomAnimation)
        {
            // Blend towards bottom
            _bottomAnimation->evaluate(time, deltaTime, bones, _tempTransforms1);
            float blendWeight = -_blendY; // Convert -1..0 to 1..0
            blendTransforms(outTransforms, _tempTransforms1, blendWeight, _tempTransforms2);
            outTransforms = _tempTransforms2;
        }
        else if (_blendY > 0.0f && _topAnimation)
        {
            // Blend towards top
            _topAnimation->evaluate(time, deltaTime, bones, _tempTransforms1);
            float blendWeight = _blendY; // 0..1 stays 0..1
            blendTransforms(outTransforms, _tempTransforms1, blendWeight, _tempTransforms2);
            outTransforms = _tempTransforms2;
        }
    }

    float AnimationBlendTree2d::duration() const
    {
        // For blend trees, return the duration of the center animation if available
        // Otherwise return the maximum duration of all animations
        float maxDuration = 0.0f;
        
        if (_centerAnimation)
            maxDuration = std::max(maxDuration, _centerAnimation->duration());
        if (_leftAnimation)
            maxDuration = std::max(maxDuration, _leftAnimation->duration());
        if (_rightAnimation)
            maxDuration = std::max(maxDuration, _rightAnimation->duration());
        if (_topAnimation)
            maxDuration = std::max(maxDuration, _topAnimation->duration());
        if (_bottomAnimation)
            maxDuration = std::max(maxDuration, _bottomAnimation->duration());
            
        return maxDuration;
    }

    void AnimationBlendTree2d::blendTransforms(const std::vector<BoneTransform>& from, 
                                             const std::vector<BoneTransform>& to, 
                                             float weight, 
                                             std::vector<BoneTransform>& result)
    {
        weight = glm::clamp(weight, 0.0f, 1.0f);
        result.resize(std::min(from.size(), to.size()));
        
        for (size_t i = 0; i < result.size(); ++i)
        {
            result[i].position = glm::mix(from[i].position, to[i].position, weight);
            result[i].scale = glm::mix(from[i].scale, to[i].scale, weight);
            result[i].rotation = glm::slerp(from[i].rotation, to[i].rotation, weight);
        }
    }

    std::shared_ptr<AnimationBlendTree2d> AnimationBlendTree2d::load(const std::string& name)
    {
		auto blendTree = Object::create<AnimationBlendTree2d>(name);        
        blendTree->loadInternal();
        return blendTree;
    }

    void AnimationBlendTree2d::loadInternal()
    {
        noz::StreamReader reader;
        if (!reader.loadFromFile(AssetDatabase::getFullPath(name(), "blend2d")))
            throw std::runtime_error("Failed to load AnimationBlendTree2d resource: " + name());

        // Read and verify signature
        if (!reader.readFileSignature("BT2D"))
			throw std::runtime_error("Invalid AnimationBlendTree2d signature in resource: " + name());

        // Read version
        auto version = reader.readUInt32();
        if (version != 1)
			throw std::runtime_error("Unsupported AnimationBlendTree2d version in resource: " + name());

        // Read animation references
        std::string skeleton = reader.readString();
        std::string centerAnim = reader.readString();
        std::string leftAnim = reader.readString();
        std::string rightAnim = reader.readString();
        std::string topAnim = reader.readString();
        std::string bottomAnim = reader.readString();
        
        // Load the actual animation resources and set them on the blend tree
        if (!centerAnim.empty())
        {
            auto anim = Asset::load<IAnimation>(centerAnim);
            if (anim)
                setCenterAnimation(anim);
            else
                noz::Log::warning("AnimationBlendTree2d", "Failed to load center animation: " + centerAnim);
        }
        
        if (!leftAnim.empty())
        {
            auto anim = Asset::load<IAnimation>(leftAnim);
            if (anim)
                setLeftAnimation(anim);
            else
                noz::Log::warning("AnimationBlendTree2d", "Failed to load left animation: " + leftAnim);
        }
        
        if (!rightAnim.empty())
        {
            auto anim = Asset::load<IAnimation>(rightAnim);
            if (anim)
                setRightAnimation(anim);
            else
                noz::Log::warning("AnimationBlendTree2d", "Failed to load right animation: " + rightAnim);
        }
        
        if (!topAnim.empty())
        {
            auto anim = Asset::load<IAnimation>(topAnim);
            if (anim)
                setTopAnimation(anim);
            else
                noz::Log::warning("AnimationBlendTree2d", "Failed to load top animation: " + topAnim);
        }
        
        if (!bottomAnim.empty())
        {
            auto anim = Asset::load<IAnimation>(bottomAnim);
            if (anim)
                setBottomAnimation(anim);
            else
                noz::Log::warning("AnimationBlendTree2d", "Failed to load bottom animation: " + bottomAnim);
        }
    }
}
