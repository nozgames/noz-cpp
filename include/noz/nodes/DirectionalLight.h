/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::node
{
    class Scene;

    class DirectionalLight : public Node3d
    {
    public:

        DirectionalLight();
        virtual ~DirectionalLight() = default;

        // Light properties
        const vec3& ambientColor() const { return _ambientColor; }
        void setAmbientColor(const vec3& color) { _ambientColor = color; }

        const vec3& diffuseColor() const { return _diffuseColor; }
        void setDiffuseColor(const vec3& color) { _diffuseColor = color; }

        float ambientIntensity() const { return _ambientIntensity; }
        void setAmbientIntensity(float intensity) { _ambientIntensity = intensity; }

        vec3 direction() const;

        // Override update to register with scene
        void update() override;

		void beginShadowPass(noz::renderer::CommandBuffer* commandBuffer);
		void endShadowPass(noz::renderer::CommandBuffer* commandBuffer);

    private:

        vec3 _ambientColor;
        vec3 _diffuseColor;
        float _ambientIntensity;
        float _diffuseIntensity;
    };
}