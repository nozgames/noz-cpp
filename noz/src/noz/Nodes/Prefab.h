///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Prefab_h__
#define __noz_Prefab_h__

namespace noz {

  class PrefabNode;

  namespace Editor {class PrefabFile;}

  class Prefab : public Asset {
    NOZ_OBJECT(Managed)

    friend class Editor::PrefabFile;
    friend class PrefabNode;

    public: class Def : public Object {
      NOZ_OBJECT()

      /// Nodes exported by template
      public: NOZ_PROPERTY(Name=Node) ObjectPtr<Node> node_;

      /// Proxy properties exported by template
      public: NOZ_PROPERTY(Name=Properties) std::vector<PropertyProxy::Def> properties_;

      public: Def(void) {}
    };

    private: class Instance : public Object {
      NOZ_OBJECT()

      friend class Editor::PrefabFile;

      private: NOZ_PROPERTY(Name=Node,Deserialize=DeserializeNode) ObjectPtr<Node> node_;
      private: NOZ_PROPERTY(Name=Objects,Deserialize=DeserializeObjects) std::vector<ObjectPtr<Object>> objects_;

      /// Target node for deserialize
      private: PrefabNode* target_;

      public: Instance(void) : target_(nullptr) {}
      public: Instance(PrefabNode* t) : target_(t) {}

      private: bool DeserializeNode (Deserializer& s);
      private: bool DeserializeObjects (Deserializer& s);
    };

    /// Instance data
    private: NOZ_PROPERTY(Name=Instance) SerializedObject instance_;

    /// Proxy properties exported by template
    public: NOZ_PROPERTY(Name=Properties) std::vector<PropertyProxy> properties_;

    /// Dynamic type that represents the prefab node used to instantiate this prefab on a node
    private: Type* prefab_node_type_;

    /// Default constructor
    public: Prefab (void);

    /// Default destructor
    public: ~Prefab (void);

    /// Instantiate the Prefab as an orphan
    public: PrefabNode* Instantiate (const Name& name=Name::Empty, NodeAttributes attr=NodeAttributes::Default);

    public: Type* GetPrefabNodeType (void) const {return prefab_node_type_;}

    public: virtual void OnLoaded (void) override;
  };

} // namespace noz


#endif // __noz_Prefab_h__

