///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_PrefabEditor_h__
#define __noz_Editor_PrefabEditor_h__

#include <noz/Nodes/Prefab.h>
#include "AssetEditor.h"

namespace noz {
namespace Editor {

  class PrefabEditorRootNode : public Node {
    NOZ_OBJECT(EditorName="Prefab", EditorIcon="{BF23FB4B-0C29-49E5-81F4-0C98B837EACE}")
    public: Prefab::Def* prefab_def_;
    public: PrefabEditorRootNode(Prefab::Def* def) : prefab_def_(def) {
      attr_ = attr_ & (~NodeAttributes::AllowRename);
    }
  };

  class PrefabEditor : public AssetEditor {
    NOZ_OBJECT(DefaultStyle="{9B75D8A9-72BA-4921-B15E-6271A12633F2}",AssetType=noz::Prefab)

    private: NOZ_CONTROL_PART(Name=ContentContainer) ObjectPtr<Node> content_container_;
    private: NOZ_PROPERTY(Name=Zoom) noz_float zoom_;

    private: ObjectPtr<PrefabEditorRootNode> root_node_;

    private: Prefab::Def prefab_def_;

    public: PrefabEditor (void);

    public: ~PrefabEditor (void);

    public: virtual void Save (void) override;

    protected: bool Load (AssetFile* file);

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnHeirarchySelectionChanged (UINode* sender);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_PrefabEditor_h__

