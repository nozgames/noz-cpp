/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "BoneTransform.h"

namespace noz::renderer
{
	class Animation;

    struct Bone
    {
        std::string name;
        int index;
        int parentIndex;
        glm::mat4 worldToLocal;
        glm::mat4 localToWorld;
        BoneTransform transform;
		float length;
        glm::vec3 direction;  // World space direction (Y-axis of bone)
    };

    class Skeleton : public noz::Asset
    {
    public:
        Skeleton(const std::string& path);
        ~Skeleton();

        // Bone accessors
        const std::vector<Bone>& bones() const { return _bones; }
        std::vector<Bone>& bones() { return _bones; }
        
        // Animation accessors
        const std::vector<std::shared_ptr<Animation>>& animations() const { return _animations; }
        std::vector<std::shared_ptr<Animation>>& animations() { return _animations; }
        
        // Get animation by name
        std::shared_ptr<Animation> animation(const std::string& name) const;
        
        // Get animation by index
        std::shared_ptr<Animation> animation(int index) const;
        
        // Get animation count
        size_t animationCount() const { return _animations.size(); }
        
        // Get bone count
        size_t boneCount() const { return _bones.size(); }
        
        // Check if skeleton has animations
        bool hasAnimations() const { return !_animations.empty(); }
        
        // Check if skeleton has bones
        bool hasBones() const { return !_bones.empty(); }
        
        // Calculate bone lengths using child projection
        void calculateBoneLengths();

		int boneIndex(const std::string& name) const;

        // Static factory methods  
        static std::shared_ptr<Skeleton> load(const std::string& name);

    protected:

		static std::shared_ptr<Skeleton> loadInternal(const std::string& filePath, const std::string& resourceName);

    private:
        
		std::vector<Bone> _bones;
        std::vector<std::shared_ptr<Animation>> _animations;
    };

} // namespace noz::renderer 