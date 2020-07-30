///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Scene_h__
#define __noz_Scene_h__

#include <noz/Nodes/SceneNode.h>

namespace noz {

  class SceneNode;

  class Scene : public Asset {
    NOZ_OBJECT(EditorIcon="{052EC1DA-EE1E-4552-A11C-C64A74CE4059}")

    friend class SceneNode;

    /// Root node for scene
    private: NOZ_PROPERTY(Name=RootNode) ObjectPtr<SceneNode> root_;

    private: NOZ_PROPERTY(Name=StyleSheets) std::vector<ObjectPtr<StyleSheet>> style_sheets_;

    /// Default constructor
    public: Scene(void);

    /// Default destructor
    public: ~Scene(void);

    public: Node* GetRootNode(void) const {return (Node*)root_;}

    public: void AddStyleSheet (StyleSheet* sheet);

    public: Style* FindStyle (Type* control_type) const;

    private: void SetWindow (Window* window);

    private: void InvalidateCameraTransform (void);

    protected: virtual void OnDeserialized (void) override;
  };

} // namespace noz


#endif //__noz_Scene_h__


