/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "ContactListener.h"

namespace noz::physics
{
	void ContactListener::BeginContact(b2Contact* contact)
	{
		if (_beginContactCallback)
		{
			_beginContactCallback(contact);
		}
	}

	void ContactListener::EndContact(b2Contact* contact)
	{
		if (_endContactCallback)
		{
			_endContactCallback(contact);
		}
	}

	void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		if (_preSolveCallback)
		{
			_preSolveCallback(contact);
		}
	}

	void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		if (_postSolveCallback)
		{
			_postSolveCallback(contact);
		}
	}
}
