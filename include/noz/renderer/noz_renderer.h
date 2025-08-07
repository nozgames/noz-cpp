#pragma once



// Prevent Windows.h min/max macros from interfering with std::min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

#include <noz/noz.h>

// noz_renderer includes
#include <noz/renderer/IAnimation.h>
#include <noz/renderer/Animation.h>
#include <noz/renderer/AnimationBlendTree.h>
#include <noz/renderer/AnimationLoader.h>
#include <noz/renderer/BoneTransform.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/renderer/Mesh.h>
#include <noz/renderer/MeshBuilder.h>
#include <noz/renderer/MeshData.h>
#include <noz/renderer/PipelineFactory.h>
#include <noz/renderer/Renderer.h>
#include <noz/renderer/ResourceSpecializations.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/Font.h>
#include <noz/renderer/Skeleton.h>
#include <noz/renderer/Texture.h> 
