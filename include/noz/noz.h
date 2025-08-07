/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

// Standard library includes
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <stack>
#include <set>
#include <map>
#include <array>
#include <tuple>
#include <optional>
#include <variant>
#include <any>
#include <filesystem>
#include <random>
#include <regex>
#include <locale>
#include <codecvt>
#include <condition_variable>
#include <atomic>
#include <future>
#include <typeindex>
#include <utility>
#include <cstddef>
#include <cstdint>

// SDL includes
#include <SDL3/SDL.h>


// core
#include <noz/math/noz_math.h>
#include <noz/Object.h>
#include <noz/IResource.h>
#include <noz/ISingleton.h>
#include <noz/Resources.h>
#include <noz/StreamReader.h>
#include <noz/StreamWriter.h>
#include <noz/Time.h>
#include <noz/HashUtils.h> 
#include <noz/Log.h>
#include <noz/Rect.h>
#include <noz/RectPacker.h>
#include <noz/TypeId.h>
#include <noz/Color.h>
#include <noz/Image.h> 
#include <noz/Application.h>

// Core node system
#include <noz/Node.h>
#include <noz/Node2d.h>
#include <noz/Node3d.h>
#include <noz/Scene.h>

// Renderer
#include <noz/renderer/CommandBuffer.h>
#include <noz/renderer/Renderer.h>
#include <noz/renderer/MeshBuilder.h>
#include <noz/renderer/Mesh.h>
#include <noz/renderer/Texture.h>
#include <noz/renderer/Animation.h>
#include <noz/renderer/AnimationBlendTree.h>
#include <noz/renderer/Shader.h>

// Render nodes (moved from noz_renderer)
#include <noz/nodes/Camera.h>
#include <noz/nodes/MeshRenderer.h>
#include <noz/nodes/Animator.h>
#include <noz/nodes/DirectionalLight.h>
#include <noz/nodes/Follow.h>
#include <noz/nodes/FollowBone.h>
#include <noz/nodes/Billboard.h>

// Input
#include <noz/input/InputAction.h>
#include <noz/input/InputActionMap.h>
#include <noz/input/InputSystem.h>

// Physics
#include <noz/physics/PhysicsSystem.h>
#include <noz/nodes/RigidBody.h>
#include <noz/nodes/Collider.h>
#include <noz/nodes/BoxCollider.h>
#include <noz/nodes/CircleCollider.h>

// UI
#include <noz/ui/Style.h>
#include <noz/ui/ElementFlags.h>
#include <noz/nodes/ui/Element.h>
#include <noz/nodes/ui/Canvas.h>
#include <noz/nodes/ui/Image.h>
#include <noz/nodes/ui/Label.h>
#include <noz/nodes/ui/Button.h>

// Event System
#include <noz/Event.h>
#include <noz/EventTypes.h>