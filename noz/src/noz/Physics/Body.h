///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Physics_Body_h__
#define __noz_Physics_Body_h__

class b2Body;

namespace noz {
  
  class Fixture;

  class Body : public Object {
    NOZ_OBJECT(NoAllocator)

    friend class Fixture;

    private: b2Body* body_;

    private: Node* node_;

    public: Body(Node* node);

    public: ~Body(void);

    public: Vector2 GetPosition(void) const;

    public: Vector2 GetLinearVelocity(void) const;

    public: void SetPosition(const Vector2& pos);

    public: void SetAngle(noz_float angle);

    public: void SetGravityScale(noz_float scale);

    public: void SetStatic(void);

    public: void SetDynamic(void);

    public: void SetAwake(bool awake);

    /// Get the node the body is associated with.
    public: Node* GetNode(void) const {return node_;}
  };

} // namespace noz


#endif // __noz_Physics_Body_h__


