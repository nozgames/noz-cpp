/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    void Animation::evaluate(
		int frameIndex,
		const std::vector<Bone>& bones,
		std::vector<BoneTransform>& transforms) const
    {
		assert(!_animationTracks.empty());
		assert(!_animationData.empty());
		assert(!bones.empty());

        // Initialize output transforms with mesh bone local transforms
		transforms.resize(bones.size());
        for (size_t i = 0; i < bones.size(); ++i)
			transforms[i] = bones[i].transform;
        
        // Apply animation data to the appropriate bones using optimized animation tracks
        for (const AnimationTrack& track : _animationTracks)
        {
            // Skip if bone index is invalid
            if (track.boneIndex < 0 || track.boneIndex >= static_cast<int>(bones.size()))
                continue;
                
            BoneTransform& boneTransform = transforms[track.boneIndex];
            
            // Use pre-calculated frame stride and track offset for efficient indexing
            auto dataIndex = static_cast<size_t>(frameIndex) * _frameStride + static_cast<size_t>(track.dataOffset);

			// Apply the transform data based on track type
            switch (track.type)
            {
            case AnimationTrackType::Translation:
                boneTransform.position.x = _animationData[dataIndex];
                boneTransform.position.y = _animationData[dataIndex + 1];
                boneTransform.position.z = _animationData[dataIndex + 2];
                break;
                    
            case AnimationTrackType::Rotation:
                boneTransform.rotation.x = _animationData[dataIndex];
                boneTransform.rotation.y = _animationData[dataIndex + 1];
                boneTransform.rotation.z = _animationData[dataIndex + 2];
                boneTransform.rotation.w = _animationData[dataIndex + 3];
                break;
                    
            case AnimationTrackType::Scale:
                boneTransform.scale.x = _animationData[dataIndex];
                boneTransform.scale.y = _animationData[dataIndex + 1];
                boneTransform.scale.z = _animationData[dataIndex + 2];
                break;
            }
        }
    }

    void Animation::evaluateAtTime(float time, const std::vector<Bone>& bones, std::vector<BoneTransform>& transforms) const
    {
		assert(!_animationTracks.empty());
		assert(!_animationData.empty());
		assert(!bones.empty());
    
		transforms.resize(bones.size());
        for (size_t i = 0; i < bones.size(); ++i)
			transforms[i] = bones[i].transform;
        
        // Calculate frame indices for interpolation
        float frameRate = 24.0f; // Standard animation frame rate
        float frameTime = time * frameRate;
        
        // Handle animation looping
        if (frameTime >= static_cast<float>(_frameCount))
        {
            frameTime = fmod(frameTime, static_cast<float>(_frameCount));
        }
        
        // Get the two frames to interpolate between
        int frame1 = static_cast<int>(frameTime);
        int frame2 = (frame1 + 1) % _frameCount;
        float t = frameTime - static_cast<float>(frame1);
        
        // Apply animation data to the appropriate bones using optimized animation tracks
        for (const AnimationTrack& track : _animationTracks)
        {
            // Skip if bone index is invalid
            if (track.boneIndex < 0 || track.boneIndex >= static_cast<int>(bones.size()))
                continue;
                
            auto& boneTransform = transforms[track.boneIndex];
            
            // Get data indices for both frames
            auto dataIndex1 = static_cast<size_t>(frame1) * _frameStride + static_cast<size_t>(track.dataOffset);
            auto dataIndex2 = static_cast<size_t>(frame2) * _frameStride + static_cast<size_t>(track.dataOffset);
            
            // Apply the interpolated transform data based on track type
            switch (track.type)
            {
            case AnimationTrackType::Translation:
                {
					auto pos1 = (glm::vec3*)(_animationData.data() + dataIndex1);
					auto pos2 = (glm::vec3*)(_animationData.data() + dataIndex2);
                    boneTransform.position = glm::mix(*pos1, *pos2, t);
				}
                break;
                    
            case AnimationTrackType::Rotation:
                {
					auto rot1 = (glm::quat*)(_animationData.data() + dataIndex1);
					auto rot2 = (glm::quat*)(_animationData.data() + dataIndex2);
					boneTransform.rotation = glm::slerp(*rot1, *rot2, t);
                }
                break;
                    
            case AnimationTrackType::Scale:
                {
					auto scale1 = (glm::vec3*)(_animationData.data() + dataIndex1);
					auto scale2 = (glm::vec3*)(_animationData.data() + dataIndex2);
                    boneTransform.scale = glm::mix(*scale1, *scale2, t);
                }
                break;
            }
        }
    }

    // IAnimation interface implementation
    void Animation::evaluate(float time, float deltaTime, const std::vector<Bone>& bones, std::vector<BoneTransform>& outTransforms)
    {
        // deltaTime is not used for single animations, just delegate to the time-based method
        evaluateAtTime(time, bones, outTransforms);
    }

    float Animation::duration() const
    {
        if (_frameCount <= 0)
            return 0.0f;
        
        // Calculate duration based on frame count and standard frame rate
        const float frameRate = 24.0f;
        return static_cast<float>(_frameCount) / frameRate;
    }

    std::shared_ptr<Animation> Animation::load(const std::string& name)
    {
        auto fullPath = AssetDatabase::getFullPath(name, "animation");
        if (!std::filesystem::exists(fullPath))
        {
            noz::Log::error("Animation", "Animation file does not exist: " + name + " (tried .animation)");
            return nullptr;
        }
        
        return loadInternal(fullPath, name);
    }
    
    std::shared_ptr<Animation> Animation::loadInternal(const std::string& filePath, const std::string& resourceName)
    {
        noz::StreamReader reader;
        if (!reader.loadFromFile(filePath))
        {
            noz::Log::error("Animation", resourceName + ": failed to load file");
            return nullptr;
        }

        // Read and verify magic
        if (!reader.readFileSignature("ANIM"))
        {
            noz::Log::error("Animation", resourceName + ": invalid signature");
            return nullptr;
        }

        // Read version
        auto version = reader.readUInt32();
        if (version != 1)
        {
            noz::Log::error("Animation", resourceName + ": unsupported version");
            return nullptr;
        }

        auto animation = std::make_shared<Animation>(resourceName);
        animation->_frameCount = reader.readUInt16();
		animation->_frameStride = reader.readUInt16();
		animation->_animationTracks = reader.readVector<AnimationTrack>();
		animation->_animationData = reader.readVector<float>();
		
		// Print debug information about the loaded animation
		noz::Log::info("Animation", "===== Animation Track Data for: " + resourceName + " =====");
		noz::Log::info("Animation", "Frame Count: " + std::to_string(animation->_frameCount));
		noz::Log::info("Animation", "Frame Stride: " + std::to_string(animation->_frameStride));
		noz::Log::info("Animation", "Total Tracks: " + std::to_string(animation->_animationTracks.size()));
		noz::Log::info("Animation", "Data Size: " + std::to_string(animation->_animationData.size()) + " floats");
		
		// Print details for each track
		for (size_t i = 0; i < animation->_animationTracks.size(); ++i)
		{
			const auto& track = animation->_animationTracks[i];
			std::string trackType;
			int dataSize = 0;
			
			switch (track.type)
			{
			case AnimationTrackType::Translation:
				trackType = "Translation (vec3)";
				dataSize = 3;
				break;
			case AnimationTrackType::Rotation:
				trackType = "Rotation (quat)";
				dataSize = 4;
				break;
			case AnimationTrackType::Scale:
				trackType = "Scale (vec3)";
				dataSize = 3;
				break;
			default:
				trackType = "Unknown";
				break;
			}
			
			noz::Log::info("Animation", "  Track " + std::to_string(i) + ":");
			noz::Log::info("Animation", "    Bone Index: " + std::to_string(static_cast<int>(track.boneIndex)));
			noz::Log::info("Animation", "    Type: " + trackType);
			noz::Log::info("Animation", "    Data Offset: " + std::to_string(track.dataOffset));
			
			// Print all frame data for this track
			noz::Log::info("Animation", "    Frame Data:");
			for (int frame = 0; frame < animation->_frameCount; ++frame)
			{
				size_t frameDataIndex = frame * animation->_frameStride + track.dataOffset;
				if (frameDataIndex < animation->_animationData.size())
				{
					std::string frameData = "      Frame " + std::to_string(frame) + ": ";
					for (int j = 0; j < dataSize && (frameDataIndex + j) < animation->_animationData.size(); ++j)
					{
						if (j > 0) frameData += ", ";
						frameData += std::to_string(animation->_animationData[frameDataIndex + j]);
					}
					noz::Log::info("Animation", frameData);
				}
			}
		}
		
		noz::Log::info("Animation", "===== End Animation Track Data =====");
		
        return animation;
    }
}
