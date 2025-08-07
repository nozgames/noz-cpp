/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
    Follow::Follow()
        : Node3d()
        , _offset(0.0f, 0.0f, 0.0f)
        , _followX(true)
        , _followY(true)
        , _followZ(true)
        , _followRotation(false)
    {
        setName("Follow");
    }

    void Follow::lateUpdate()
    {
        // Call base class lateUpdate first
        Node3d::lateUpdate();
        
        // Get target if it's still valid
        auto targetNode = _target.lock();
        if (!targetNode)
            return;

        // Calculate new position based on target position + offset
        vec3 targetPos = targetNode->localPosition();
        vec3 newPos = localPosition();
        
        if (_followX)
            newPos.x = targetPos.x + _offset.x;
        if (_followY)
            newPos.y = targetPos.y + _offset.y;
        if (_followZ)
            newPos.z = targetPos.z + _offset.z;
            
        setLocalPosition(newPos);
        
        // Follow rotation if enabled
        if (_followRotation)
        {
            setLocalRotation(targetNode->localRotation());
        }
    }
}