/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <box2d/box2d.h>
#include <enet/enet.h>

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
