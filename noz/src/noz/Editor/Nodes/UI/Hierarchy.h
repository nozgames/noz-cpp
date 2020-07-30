///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_Hierarchy_h__
#define __noz_Editor_Hierarchy_h__

#include <noz/Nodes/UI/TreeView.h>
#include "HierarchyItem.h"

namespace noz { class Button; }

namespace noz {
namespace Editor {

  class Hierarchy : public Control {
    NOZ_OBJECT(DefaultStyle="{68F2FF32-384C-44D4-91EC-BDB2D9B3134D}")

    friend class HierarchyItem;

    public: SelectionChangedEventHandler SelectionChanged;

    NOZ_CONTROL_PART(Name=TreeView)
    private: ObjectPtr<TreeView> tree_view_;

    NOZ_CONTROL_PART(Name=AddNodeButton)
    private: ObjectPtr<Button> add_node_button_;

    /// Root node that is the target of the heirarchy
    private: ObjectPtr<Node> target_;

    public: Hierarchy (void);

    public: ~Hierarchy (void);

    public: void SetTarget (Node* target);

    public: void Refresh (void);

    public: Node* GetSelected (void) const;

    public: void SetSelected (Node* node);

    public: void UnselectAll (void); 

    public: noz_uint32 GetSelectedItemCount (void) const;

    public: HierarchyItem* GetSelectedItem (noz_uint32 i) const;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnKeyDown (SystemEvent* e) override;

    private: HierarchyItem* InsertItem (Node* node, TreeViewItem* parent, noz_int32 index=-1);

    private: String NodeToString (Node* node) const;

    public: HierarchyItem* FindItem (HierarchyItem* root, Node* target);

    private: bool AddNodeToSelectedItem (Node* n);
    private: bool DeleteSelectedNodes (void);
    private: void Copy (void);
    private: void Paste (void);
    private: void Paste (Object* o, HierarchyItem* parent, noz_int32 insert=-1);
    private: DragDropEffects GetDragDropEffect (Object* o, Node* target);

    private: void OnTreeViewSelectionChanged (UINode* sender);
    private: void OnAddNodeButtonClicked (UINode* sender);
    private: void OnItemDragDrop (HierarchyItem* item, DragDropEventArgs* args);

    private: Name GetUniqueName (Type* t, Node* parent);

    private: void OnTreeViewTextChanged(UINode* sender, TreeViewItem* item, const String* text);
    private: void OnPropertyChanged (PropertyChangedEventArgs* args);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_Hierarchy_h__

