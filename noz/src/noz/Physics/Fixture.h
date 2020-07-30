///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Physics_Fixture_h__
#define __noz_Physics_Fixture_h__

class b2Fixture;

namespace noz {

  class Body;

  class Fixture : public Object {
    NOZ_OBJECT(NoAllocator)

    private: b2Fixture* fixture_;

    /// Rectangle fixture.
    public: Fixture(Body* body, Rect rect);

    /// Circle fixture
    public: Fixture(Body* body, noz_float radius, const Vector2& center);

    /// Test if the given point in world coordinates is within the fixture
    public: bool TestPoint (const Vector2& pt);

    public: ~Fixture (void);
  };

} // namespace noz


#endif // __noz_Physics_Fixture_h__


