/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/nodes/Billboard.h>
#include <noz/nodes/Camera.h>
#include <noz/Scene.h>

namespace noz::node
{
    NOZ_DEFINE_TYPEID(Billboard)
    
    Billboard::Billboard()
    {
    }

	void Billboard::initialize()
	{
		// for create pattern
	}
    
    void Billboard::lateUpdate()
    {
        Node3d::update();        
        updateBillboard();
        updateFacingScale();
    }
    
    void Billboard::updateBillboard()
    {
        auto camera = scene()->camera();
        assert(camera);

#if 1
		setRotation(glm::quatLookAt(-camera->forward(), VEC3_UP));
#else
        // Get camera position
        vec3 cameraPosition = camera->position();
        vec3 billboardPosition = position();
        
        // Calculate direction from billboard to camera
        vec3 direction = cameraPosition - billboardPosition;
        
		camera->forward();

        // Only rotate around Y axis to keep billboard upright
        //direction.y = 0.0f;
        
        if (noz::math::length(direction) > 0.0001f)
        {
            direction = noz::math::normalize(direction);
            
            // Create rotation that faces the camera
            lookAt(billboardPosition - camera->forward(), VEC3_UP);
        }
#endif
    }
    
    void Billboard::updateFacingScale()
    {
        // Get parent's facing direction if it exists
		auto parentNode = parent()->as<node::Node3d>();
		assert(parentNode);
                        
        // Get parent's forward direction (facing direction)
        vec3 parentForward = parentNode->forward();
        
        // Check if parent is facing predominantly right (+X) or left (-X)
        // If forward.x > 0, facing right, scale normally (1)
        // If forward.x <= 0, facing left, flip X scale (-1)
        vec3 currentScale = localScale();
        
        // Preserve the absolute scale values but change the sign of X
        float absScaleX = abs(currentScale.x);
        
        if (parentForward.x > 0.0f)
        {
            // Facing right - positive X scale
            currentScale.x = absScaleX;
        }
        else
        {
            // Facing left - negative X scale
            currentScale.x = -absScaleX;
        }
        
        setLocalScale(currentScale);
    }
}