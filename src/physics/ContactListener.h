/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::physics
{
	class ContactListener : public b2ContactListener
	{
	public:

		using ContactCallback = std::function<void(b2Contact*)>;

		void setBeginContactCallback(ContactCallback callback) { _beginContactCallback = callback; }
		void setEndContactCallback(ContactCallback callback) { _endContactCallback = callback; }
		void setPreSolveCallback(ContactCallback callback) { _preSolveCallback = callback; }
		void setPostSolveCallback(ContactCallback callback) { _postSolveCallback = callback; }

	protected:
		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;
		void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
		void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

	private:
		ContactCallback _beginContactCallback;
		ContactCallback _endContactCallback;
		ContactCallback _preSolveCallback;
		ContactCallback _postSolveCallback;
	};
}
