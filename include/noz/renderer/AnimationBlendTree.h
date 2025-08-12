/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "IAnimation.h"

namespace noz::renderer
{
    // Forward declarations
    class Skeleton;

    class AnimationBlendTree2d : public IAnimation
    {
    public:

		NOZ_DECLARE_TYPEID(AnimationBlendTree2d, IAnimation)

        ~AnimationBlendTree2d();

        // IAnimation interface implementation
        void evaluate(float time, float deltaTime, const std::vector<Bone>& bones, std::vector<BoneTransform>& outTransforms) override;
        float duration() const override;
        bool canLoop() const override { return true; }

        /**
         * @brief Set the 2D blend parameters (-1 to 1 for each axis)
         * @param x Horizontal blend (-1 = left, 0 = center, 1 = right)
         * @param y Vertical blend (-1 = bottom, 0 = center, 1 = top)
         */
        void setBlendParameters(float x, float y);

        /**
         * @brief Set the center animation (default/idle)
         * @param animation The center animation
         */
        void setCenterAnimation(const std::shared_ptr<IAnimation>& animation);

        /**
         * @brief Set the left animation
         * @param animation The left animation
         */
        void setLeftAnimation(const std::shared_ptr<IAnimation>& animation);

        /**
         * @brief Set the right animation
         * @param animation The right animation
         */
        void setRightAnimation(const std::shared_ptr<IAnimation>& animation);

        /**
         * @brief Set the top animation
         * @param animation The top animation
         */
        void setTopAnimation(const std::shared_ptr<IAnimation>& animation);

        /**
         * @brief Set the bottom animation
         * @param animation The bottom animation
         */
        void setBottomAnimation(const std::shared_ptr<IAnimation>& animation);

    private:

		friend class AssetDatabase;

        static std::shared_ptr<AnimationBlendTree2d> load(const std::string& name);

        AnimationBlendTree2d();

        void initialize(const std::string& name) override;
        void initialize(const std::shared_ptr<Skeleton>& skeleton);

        void loadInternal();

        void blendTransforms(const std::vector<BoneTransform>& from,
            const std::vector<BoneTransform>& to,
            float weight,
            std::vector<BoneTransform>& result);

        // 2D blend parameters
        float _blendX; // -1 to 1 (left to right)
        float _blendY; // -1 to 1 (bottom to top)
        
        // Animation slots
        std::shared_ptr<IAnimation> _centerAnimation;
        std::shared_ptr<IAnimation> _leftAnimation;
        std::shared_ptr<IAnimation> _rightAnimation;
        std::shared_ptr<IAnimation> _topAnimation;
        std::shared_ptr<IAnimation> _bottomAnimation;
        
        // Temporary transform storage for blending
        std::vector<BoneTransform> _tempTransforms1;
        std::vector<BoneTransform> _tempTransforms2;
        std::vector<BoneTransform> _tempTransforms3;
    };
    
}