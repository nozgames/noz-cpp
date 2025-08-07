/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "BoneTransform.h"
#include "IAnimation.h"

namespace noz::renderer
{
	struct Bone;

	struct AnimationTrack
    {
        uint8_t boneIndex;
        AnimationTrackType type;
        int dataOffset;
    };

    class Animation : public IAnimation
    {
    public:
        // Constructor with name
        Animation(const std::string& name) : IAnimation(name) {}
        
        // IAnimation interface implementation
        void evaluate(float time, float deltaTime, const std::vector<Bone>& bones, std::vector<BoneTransform>& outTransforms) override;
        float duration() const override;
        bool canLoop() const override { return true; }
        uint32_t frameCount() const override { return static_cast<uint32_t>(_frameCount); }
        
        // Legacy methods for backwards compatibility
        void evaluate(int frameIndex, const std::vector<Bone>& meshBones, std::vector<BoneTransform>& outputTransforms) const;
        void evaluateAtTime(float time, const std::vector<Bone>& meshBones, std::vector<BoneTransform>& outputTransforms) const;
        
        // Get animation name (inherited from IResource)
        
        // Get frame count (legacy method)
        int frameCountLegacy() const { return _frameCount; }
        
        // Get track count
        int trackCount() const { return static_cast<int>(_animationTracks.size()); }
        
        // Debug method to get data size
        size_t dataSize() const { return _animationData.size(); }
                                
        // Static factory method for loading from binary format
        static std::shared_ptr<Animation> load(const std::string& name);        

    protected:

        static std::shared_ptr<Animation> loadInternal(const std::string& filePath, const std::string& resourceName);

    private:

        int _frameCount;
        size_t _frameStride;
        std::vector<AnimationTrack> _animationTracks;
        std::vector<float> _animationData;
    };
}