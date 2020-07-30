///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_Workspace_h__
#define __noz_Editor_Workspace_h__

#include <noz/Nodes/UI/Document.h>
#include <noz/Nodes/UI/DocumentManager.h>
#include <noz/Nodes/UI/DockItem.h>
#include <noz/Nodes/UI/DockManager.h>
#include "EditorDocument.h"
#include "TypePicker.h"

namespace noz {

  class Button;
  class DockItem;

namespace Editor {

  class Memento;
  class Inspector;
  class Hierarchy;
  class AssetFile;
  class TypePicker;
  class AnimationView;

  class Workspace : public Control {
    NOZ_OBJECT(DefaultStyle="{F26BD4CF-60B2-41E6-A84C-91B6267DF5F8}")

    private: NOZ_CONTROL_PART(Name=AssetsItem) ObjectPtr<DockItem> assets_item_;
    private: NOZ_CONTROL_PART(Name=ConsoleItem) ObjectPtr<DockItem> console_item_;
    private: NOZ_CONTROL_PART(Name=DocumentManager) ObjectPtr<DocumentManager> document_manager_;
    private: NOZ_CONTROL_PART(Name=DockManager) ObjectPtr<DockManager> dock_manager_;
    private: NOZ_CONTROL_PART(Name=InspectorDockItem) ObjectPtr<DockItem> inspector_dock_item_;
    private: NOZ_CONTROL_PART(Name=HierarchyDockItem) ObjectPtr<DockItem> hierarchy_dock_item_;
    private: NOZ_CONTROL_PART(Name=AnimationView) ObjectPtr<AnimationView> animation_view_;
    private: NOZ_CONTROL_PART(Name=ToolBox) ObjectPtr<TypePicker> tool_box_;

    private: ObjectPtr<Inspector> inspector_;

    private: bool inspector_owned_;

    private: ObjectPtr<Hierarchy> hierarchy_;

    private: ObjectPtr<Animator> animation_view_animator_;

    public: Workspace (void);

    public: ~Workspace (void);

    public: void SetInspector (Inspector* inspector, bool own=false);

    public: void SetHierarchy (Hierarchy* hierarchy);

    public: void SetAnimationViewAnimator (Animator* animator);

    public: TypePicker* GetToolBox(void) const {return tool_box_;}

    public: Inspector* GetInspector (void) const {return inspector_;}

    public: Hierarchy* GetHierarchy (void) const {return hierarchy_;}

    public: AnimationView* GetAnimationView(void) const {return animation_view_;}

    public: EditorDocument* GetActiveDocument (void) const;

    public: static Workspace* GetWorkspace (Node *node);

    public: void EditAsset (AssetFile* file);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnPreviewKeyDown (SystemEvent* e) override;
    protected: virtual void OnKeyDown (SystemEvent* e) override;
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_Workspace_h__

