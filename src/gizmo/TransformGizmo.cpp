#include <noz/gizmo/TransformGizmo.h>
#include <noz/gizmo/Gizmos.h>
#include <noz/Node3d.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/renderer/MeshBuilder.h>
#include <noz/renderer/Mesh.h>
#include <algorithm>

namespace noz::debug
{
    // Static member definitions
    std::shared_ptr<noz::renderer::Mesh> TransformGizmo::s_sharedMesh;

	NOZ_DEFINE_TYPEID(TransformGizmo)

    TransformGizmo::TransformGizmo()
    {
    }

    TransformGizmo::~TransformGizmo()
    {
    }

	void TransformGizmo::initialize()
	{
		// empty, needed for create pattern
	}

    void TransformGizmo::onAttachedToParent()
    {
        IGizmo::onAttachedToParent();

        // Cache parent Node3d when attached
        _parentNode3d = parent<noz::node::Node3d>();
    }

    void TransformGizmo::onDetachedFromParent()
    {
        // Clear cached parent when detached
        _parentNode3d.reset();

        IGizmo::onDetachedFromParent();
    }

	void TransformGizmo::renderGizmo(noz::renderer::CommandBuffer* commandBuffer)
    {
		assert(commandBuffer);

        // Get parent Node3d transform
        auto parentNode3d = _parentNode3d.lock();
		assert(parentNode3d);

        // Get shared mesh
        auto mesh = getSharedMesh();
        if (!mesh)
            return;

        commandBuffer->setTransform(parentNode3d->localToWorld());
        commandBuffer->drawMesh(mesh);
    }

    std::shared_ptr<noz::renderer::Mesh> TransformGizmo::getSharedMesh()
    {
        if (!s_sharedMesh)
        {
            s_sharedMesh = createSharedMesh();
        }
        return s_sharedMesh;
    }

    std::shared_ptr<noz::renderer::Mesh> TransformGizmo::createSharedMesh()
    {
        // Create a single mesh containing all transform gizmo shapes
        auto mesh = std::make_shared<noz::renderer::Mesh>("transform_gizmo_shared_mesh");
        
        // Use MeshBuilder to create the transform gizmo mesh
        noz::renderer::MeshBuilder builder;

		const auto& red = Gizmos::s_redColorUV;
		const auto& green = Gizmos::s_greenColorUV;
		const auto& blue = Gizmos::s_blueColorUV;
                
        // Get configuration values from constants
        float scale = Scale;
        float cylinderRadius = CylinderRadius * scale;
        float cylinderLength = CylinderLength * scale;
        float coneHeight = ConeHeight * scale;
        float coneRadius = ConeRadius * scale; // Can be larger than cylinder radius
        
        // Create X-axis (Red) - cylinder + cone pointing in +X direction
        glm::vec3 xStart = glm::vec3(cylinderLength * 0.05f, 0.0f, 0.0f);
        glm::vec3 xEnd = glm::vec3(cylinderLength, 0.0f, 0.0f);
        glm::vec3 xConeTip = glm::vec3(cylinderLength + coneHeight, 0.0f, 0.0f);
        
        // X-axis cylinder (red color region in texture)
        builder.addCylinder(xStart, xEnd, cylinderRadius, red, 8, 0);
        
        // X-axis cone (red color region in texture) - can be larger than cylinder
        builder.addCone(xEnd, xConeTip, coneRadius, red, 8, 0);
        
        // Create Y-axis (Green) - cylinder + cone pointing in +Y direction
        glm::vec3 yStart = glm::vec3(0.0f, cylinderLength * 0.05f, 0.0f);
        glm::vec3 yEnd = glm::vec3(0.0f, cylinderLength, 0.0f);
        glm::vec3 yConeTip = glm::vec3(0.0f, cylinderLength + coneHeight, 0.0f);
        
        // Y-axis cylinder (green color region in texture)
        builder.addCylinder(yStart, yEnd, cylinderRadius, green, 8, 1);
        
        // Y-axis cone (green color region in texture) - can be larger than cylinder
        builder.addCone(yEnd, yConeTip, coneRadius, green, 8, 1);
        
        // Create Z-axis (Blue) - cylinder + cone pointing in +Z direction
        glm::vec3 zStart = glm::vec3(0.0f, 0.0f, cylinderLength * 0.05f);
        glm::vec3 zEnd = glm::vec3(0.0f, 0.0f, cylinderLength);
        glm::vec3 zConeTip = glm::vec3(0.0f, 0.0f, cylinderLength + coneHeight);
        
        // Z-axis cylinder (blue color region in texture)
        builder.addCylinder(zStart, zEnd, cylinderRadius, blue, 8, 2);
        
        // Z-axis cone (blue color region in texture) - can be larger than cylinder
        builder.addCone(zEnd, zConeTip, coneRadius, blue, 8, 2);
        
        // Set mesh data from builder
        mesh->positions() = builder.positions();
        mesh->normals() = builder.normals();
        mesh->uv0() = builder.uv0();
        mesh->boneIndices() = builder.boneIndices();
        mesh->indices() = builder.indices();
        
        // Upload to GPU
        mesh->upload(true);
        
        return mesh;
    }

}