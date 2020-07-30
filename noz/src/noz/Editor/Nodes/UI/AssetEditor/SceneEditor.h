///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_SceneEditor_h__
#define __noz_Editor_SceneEditor_h__

#include "AssetEditor.h"
#include <noz/Nodes/UI/DropDownList.h>
#include <noz/Editor/Nodes/Render/GridNode.h>
#include "../Inspector.h"
#include "../Hierarchy.h"

namespace { class Process; }

namespace noz {
namespace Editor {

  class Memento;

  class SceneEditorRootNode : public Node {
    NOZ_OBJECT()

    public: virtual void HandleMouseMoveEvent (SystemEvent* e) { }
    public: virtual void RenderOverride (RenderContext* rc) override;
    public: virtual void Update (void) override {}
  };

  class SceneEditor : public AssetEditor {
    NOZ_OBJECT(DefaultStyle="{1121A0D0-B76C-4277-91B8-202DDBFFFE5A}",AssetType=noz::Scene)

    friend class SceneEditorRootNode;

    private: NOZ_CONTROL_PART(Name=Resolution) ObjectPtr<DropDownList> resolution_;
    private: NOZ_CONTROL_PART(Name=SceneRoot) ObjectPtr<SceneEditorRootNode> scene_root_;
    private: NOZ_CONTROL_PART(Name=GridNode) ObjectPtr<GridNode> grid_node_;
    private: NOZ_CONTROL_PART(Name=ZoomNode) ObjectPtr<Node> zoom_node_;
    private: NOZ_CONTROL_PART(Name=PanNode) ObjectPtr<Node> pan_node_;
    private: NOZ_CONTROL_PART(Name=Play) ObjectPtr<Button> play_;

    private: NOZ_PROPERTY(Name=Zoom) noz_float zoom_;

    private: ObjectPtr<Inspector> inspector_;

    private: ObjectPtr<Hierarchy> hierarchy_;

    private: ObjectPtr<Scene> scene_;

    private: Process* playing_process_;

    private: bool panning_;

    private: Vector2 pan_start_;

    public: SceneEditor (void);

    public: ~SceneEditor (void);

    public: void SetZoom (noz_float zoom);

    public: virtual void Save (void) override;

    protected: bool Load (AssetFile* file);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnActivate (void) override;
    protected: virtual void Update (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseWheel (SystemEvent* e) override;
    protected: virtual void OnMouseOver (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;

    private: void OnHeirarchySelectionChanged (UINode* sender);
    private: void RefreshResolutions (void);
    private: void OnResolutionSelected (UINode*);
    private: void OnPlay (UINode*);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_SceneEditor_h__

