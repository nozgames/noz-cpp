///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TreeView_h__
#define __noz_TreeView_h__

#include "TreeViewItem.h"

namespace noz {

  class ScrollView;
  
  typedef Event<UINode*,TreeViewItem*,const String*> TreeViewTextChangedEventHandler;

  class TreeView : public Control {
    NOZ_OBJECT(DefaultStyle="{EF09851D-20B5-447D-A949-0892BCD76508}",EditorIcon="{1409AE1F-CA55-45EA-8FFD-E9CEFE55AFA8}")

    friend class TreeViewItem;

    /// Event fired when selection within the treeview changes.
    public: SelectionChangedEventHandler SelectionChanged;
    public: TreeViewTextChangedEventHandler ItemTextChanged;

    protected: NOZ_CONTROL_PART(Name=ItemsContainer) ObjectPtr<Node> items_container_;
    protected: NOZ_CONTROL_PART(Name=ScrollView) ObjectPtr<ScrollView> scroll_view_;

    private: NOZ_PROPERTY(Name=SelectionMode) SelectionMode selection_mode_;

    /// Array of all selected items
    private: std::vector<ObjectPtr<TreeViewItem>> selected_items_;

    /// Array of selected items saved during drag drop process
    private: static std::vector<ObjectPtr<TreeViewItem>> saved_selected_items_;

    /// Item that currently has focus for keyboard navigation
    private: ObjectPtr<TreeViewItem> focus_item_;

    private: ObjectPtr<TreeViewItem> selection_anchor_;

    private: ObjectPtr<TreeViewItem> text_edit_pending_;

    public: TreeView (void);

    public: ~TreeView (void);

    public: void UnselectAll (void);

    public: void Select (TreeViewItem* item);

    public: void Unselect (TreeViewItem* item);

    public: void ExpandAll (void);

    public: void SetSelectedItem (TreeViewItem* item);
    
    /// Return the item that is acting as the selection anchor
    public: TreeViewItem* GetSelectionAnchorItem (void) const {return selection_anchor_;}

    /// Return the TreeViewItem that has current focus or nullptr if none.
    public: TreeViewItem* GetFocusedItem(void) const {return focus_item_;}

    public: TreeViewItem* GetFirstChildItem (void) const;

    public: TreeViewItem* GetLastChildItem (void) const;

    public: TreeViewItem* GetSelectedItem(void) const {return selected_items_.empty()?nullptr:selected_items_[0];}

    public: SelectionMode GetSelectionMode (void) const {return selection_mode_;}

    /// Return the vector of selected items.
    public: const std::vector<ObjectPtr<TreeViewItem>>& GetSelectedItems(void) const {return selected_items_;}

    private: void AdjustFocusForRemove (noz_uint32 index);

    private: void SetFocusedItem (TreeViewItem* tvitem);

    private: void UnselectAllNoEvent (void);

    private: void OnFocusChanged(Window* window, UINode* new_focus);

    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnChildRemoved (Node* child) override;
    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnGainFocus (void) override;
    protected: virtual void OnLoseFocus (void) override;
    protected: virtual void OnMouseWheel (SystemEvent* e) override;
    protected: virtual void OnKeyDown (SystemEvent* e) override;
    protected: virtual void Update (void) override;
  };

} // namespace noz


#endif //__noz_TreeView_h__

