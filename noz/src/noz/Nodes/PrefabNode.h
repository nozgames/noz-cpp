///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PrefabNode_h__
#define __noz_PrefabNode_h__

namespace noz {

  class Prefab;

  class PrefabNode : public Node {
    NOZ_OBJECT(EditorName="Prefab")

    friend class Prefab;

    private: ObjectPtr<Node> instance_;

    private: Prefab* template_;

    /// Objects vector which matches the proxy properties in the prefab
    private: std::vector<ObjectPtr<Object>> template_objects_;

    public: PrefabNode(void);

    public: template <typename T> T* GetInstance (void) const {return Cast<T>(instance_);}

    public: virtual Type* GetType (void) const override;

    public: void Reload (void);

    private: void ApplyTemplate(Prefab* prefab);

    /// Return the property proxy object
    protected: virtual Object* GetPropertyProxyObject(noz_uint32 id) const override;
  };

} // namespace noz


#endif // __noz_PrefabNode_h__

