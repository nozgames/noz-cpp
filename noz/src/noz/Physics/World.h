///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Physics_World_h__
#define __noz_Physics_World_h__

class b2World;

namespace noz {

  class Body;

  class World {
    friend class Body;

    private: static World* instance_;

    private: b2World* world_;

    public: World(void);

    public: ~World(void);

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static void Update (void);

    public: static void SetGravity (const Vector2& gravity);

    //public: static void OverlapPointAll(Vector2 point, int layerMask = DefaultRaycastLayers, float minDepth = -Mathf.Infinity, float maxDepth = Mathf.Infinity);

    private: void Update_ (void);
  };

} // namespace noz


#endif // __noz_Physics_World_h__


