#pragma once

#include "math/noz_math.h"
#include <typeindex>

namespace noz
{
	class Node;
	class Collider;

	struct EngineStarted {};
	struct EngineShutdown {};

	struct NodeAdded
	{
		Node* node;
	};

	struct NodeRemoved
	{
		Node* node;
	};

	struct NodeTransformChanged
	{
		Node* node;
		vec3 oldPosition;
		vec3 newPosition;
	};

	struct KeyPressed
	{
		int key;
		bool repeat;
	};

	struct KeyReleased
	{
		int key;
	};

	struct MouseButtonPressed
	{
		int button;
		vec2 position;
	};

	struct MouseButtonReleased
	{
		int button;
		vec2 position;
	};

	struct MouseMoved
	{
		vec2 position;
		vec2 delta;
	};

	struct MouseWheelScrolled
	{
		vec2 scroll;
	};

	struct CollisionEnter
	{
		Collider* colliderA;
		Collider* colliderB;
		vec2 contactPoint;
		vec2 contactNormal;
	};

	struct CollisionExit
	{
		Collider* colliderA;
		Collider* colliderB;
	};

	struct TriggerEnter
	{
		Collider* trigger;
		Collider* other;
	};

	struct TriggerExit
	{
		Collider* trigger;
		Collider* other;
	};

	struct ResourceLoaded
	{
		std::string resourcePath;
		std::type_index resourceType;
	};

	struct ResourceUnloaded
	{
		std::string resourcePath;
		std::type_index resourceType;
	};

	struct ResourceReloaded
	{
		std::string resourcePath;
		std::type_index resourceType;
	};

	struct WindowResized
	{
		int width;
		int height;
	};

	struct WindowClosed {};

	struct ApplicationFocusGained {};
	struct ApplicationFocusLost {};
}