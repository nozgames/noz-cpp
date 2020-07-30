///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AnimationControllerEditor_h__
#define __noz_Editor_AnimationControllerEditor_h__

#include <noz/Nodes/UI/TreeViewItem.h>
#include <noz/Animation/AnimationController.h>
#include "AssetEditor.h"

namespace noz { class TreeView; class Button; }

namespace noz {
namespace Editor {

  class Memento;

  class AnimationControllerEditorLayer : public TreeViewItem {
    NOZ_OBJECT(DefaultStyle="{76035210-854A-470A-AECE-2E5E34897886}");

    public: AnimationLayer* layer_;

    public: AnimationControllerEditorLayer(AnimationLayer* layer) : layer_(layer) {}
    
    protected: virtual void Update (void) override;
  };

  class AnimationControllerEditorState : public TreeViewItem {
    NOZ_OBJECT(DefaultStyle="{60CE4842-C905-43C0-A0F3-871481005250}");

    public: AnimationState* state_;

    public: AnimationControllerEditorState(AnimationState* state) : state_(state) {}

    protected: virtual void Update (void) override;
  };

  class AnimationControllerEditor : public AssetEditor {
    NOZ_OBJECT(DefaultStyle="{A642A674-DB79-4E80-A6B0-6765EFA9B3D7}",AssetType=noz::AnimationController)

    NOZ_CONTROL_PART(Name=AddLayerButton)
    private: ObjectPtr<Button> add_layer_button_;

    NOZ_CONTROL_PART(Name=AddStateButton)
    private: ObjectPtr<Button> add_state_button_;

    NOZ_CONTROL_PART(Name=TreeView)
    private: ObjectPtr<TreeView> tree_view_;

    private: ObjectPtr<AnimationController> controller_;

    public: AnimationControllerEditor (void);

    public: ~AnimationControllerEditor (void);

    public: void Refresh (void);

    protected: virtual bool Load (AssetFile* file) override;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void Save (void) override;
    
    private: void OnAddLayerButton (UINode* sender);
    private: void OnAddStateButton (UINode* sender);
    private: void OnTreeViewSelectionChanged (UINode* sender);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AnimationControllerEditor_h__

