///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_NodePathProperty_h__
#define __noz_NodePathProperty_h__

namespace noz {


  class NodePathProperty : public Property {
    NOZ_OBJECT()

    public: NodePathProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual void Set (Object* t, const NodePath& v) = 0;

    public: virtual const NodePath& Get (Object* t) const {
      static NodePath empty;
      return empty;
    }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };

} // namespace noz

#endif // __noz_NodePathProperty_h__
