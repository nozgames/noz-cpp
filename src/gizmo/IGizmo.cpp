/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/gizmo/IGizmo.h>
#include <noz/gizmo/Gizmos.h>

namespace noz::debug
{
	NOZ_DEFINE_TYPEID(IGizmo);

	void IGizmo::update()
	{
		if (Gizmos::instance())
			Gizmos::instance()->registerGizmo(this->as<IGizmo>());
	}
}