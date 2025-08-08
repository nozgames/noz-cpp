/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "noz/Asset.h"

namespace noz::renderer
{
    // Forward declarations
    class Skeleton;
    struct BoneTransform;

    /**
     * @brief Interface for all animation types (single animations, blend trees, etc.)
     * Allows animations to be treated uniformly and loaded as resources
     */
    class IAnimation : public noz::Asset
    {
    public:
        // Constructor with name (required for Asset)
        IAnimation(const std::string& name) : Asset(name) {}
        virtual ~IAnimation() = default;

        /**
         * @brief Evaluate the animation at a specific time and populate bone transforms
         * @param time Current animation time
         * @param deltaTime Time since last frame (for blend trees that need to update internal state)
         * @param bones The skeleton bones to animate
         * @param outTransforms Output bone transforms
         */
        virtual void evaluate(float time, float deltaTime, const std::vector<struct Bone>& bones, std::vector<BoneTransform>& outTransforms) = 0;

        /**
         * @brief Get the duration of the animation in seconds
         * @return Animation duration, or -1 if infinite/looping
         */
        virtual float duration() const = 0;

        /**
         * @brief Check if this animation supports looping
         * @return True if the animation can loop
         */
        virtual bool canLoop() const = 0;

        /**
         * @brief Get the frame count for this animation (if applicable)
         * @return Frame count, or 0 if not applicable
         */
        virtual uint32_t frameCount() const { return 0; }
    };

} // namespace noz::renderer