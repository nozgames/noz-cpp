///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_StyleEditor_h__
#define __noz_Editor_StyleEditor_h__

#include "AssetEditor.h"
#include "../Inspector.h"
#include "../Hierarchy.h"

namespace noz {
namespace Editor {

  class Memento;

  class StyleEditorRootNode : public Node {
    NOZ_OBJECT(EditorName="Style", EditorIcon="{BF23FB4B-0C29-49E5-81F4-0C98B837EACE}")
    public: Style::Def* style_def_;
    public: StyleEditorRootNode(Style::Def* def) : style_def_(def) {
      attr_ = attr_ & (~NodeAttributes::AllowRename);
    }
    public: virtual void HandleMouseMoveEvent (SystemEvent* e) { }
  };

  class StyleEditor : public AssetEditor {
    NOZ_OBJECT(DefaultStyle="{0DACD4CF-AADE-46A9-B353-EEAF05A90CA7}",AssetType=noz::Style)

    private: NOZ_CONTROL_PART(Name=ContentContainer) ObjectPtr<Node> content_container_;
    private: NOZ_PROPERTY(Name=Zoom) noz_float zoom_;

    private: ObjectPtr<StyleEditorRootNode> root_node_;

    private: ObjectPtr<Hierarchy> hierarchy_;

    private: Style::Def style_def_;

    public: StyleEditor (void);

    public: ~StyleEditor (void);

    public: virtual void Save (void) override;

    public: static void UpdateControlParts (Style::Def* def);

    protected: bool Load (AssetFile* file);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;

    private: void OnHeirarchySelectionChanged (UINode* sender);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_StyleEditor_h__

