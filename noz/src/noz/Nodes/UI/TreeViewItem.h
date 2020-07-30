///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TreeViewItem_h__
#define __noz_TreeViewItem_h__

#include "SelectionMode.h"

namespace noz {

  class Spacer;
  class TextBox;
  class ToggleButton;
  class TreeView;

  class TreeViewItem : public ContentControl {
    NOZ_OBJECT(DefaultStyle="{721C28DE-AEAE-4135-9DC2-2B63D835890D}",EditorIcon="{80C227E5-2801-414D-BD48-0991BC1A74F9}")

    friend class TreeView;

    /// Part used to add items to
    protected: NOZ_CONTROL_PART(Name=ItemsContainer) ObjectPtr<Node> items_container_;

    /// Part used to apply depth spacing to items to allow for the option of full row selected
    protected: NOZ_CONTROL_PART(Name=Spacer) ObjectPtr<Spacer> spacer_;

    /// ToggleButton used to expand and collapse the item
    protected: NOZ_CONTROL_PART(Name=ExpandButton) ObjectPtr<ToggleButton> expand_button_;

    /// TextBox used to in-place edit the item text
    protected: NOZ_CONTROL_PART(Name=TextBox) ObjectPtr<TextBox> text_box_;

    NOZ_PROPERTY(Name=Expanded,Type=bool,Set=SetExpanded,Get=IsExpanded)
    NOZ_PROPERTY(Name=Selectable,Type=bool,Set=SetSelectable,Get=IsSelectable)

    private: struct {
      /// True if the tree view item is expanded.
      noz_byte expanded_ : 1;

      /// True if the item is currently selected
      noz_byte selected_ : 1;

      /// True if in-place editing is allowed for this item
      noz_byte editable_ : 1;

      /// True if the mouse is hovering over the tree view item or any child items
      noz_byte hover_ : 1;

      /// True if the item is selectable
      noz_byte selectable_ : 1;
    };
    
    /// TreeView the item is contained within.
    private: ObjectPtr<TreeView> tree_view_;

    /// Parent item.  If the item is a root item then the value will be nullptr
    private: ObjectPtr<TreeViewItem> parent_item_;

    /// Depth within the containing tree view. (0==Root)
    private: noz_int32 depth_;

    /// User data associated with the item
    private: Object* user_data_;
    
    public: TreeViewItem (void);

    public: ~TreeViewItem (void);  
     
    public: void SetUserData (Object* user_data);

    public: Object* GetUserData (void) const {return user_data_;}

    public: void ExpandAll (void);

    public: void SetEditable (bool v);

    public: void SetExpanded (bool expanded);

    public: void SetSelected (bool selected);

    public: void SetSelectable (bool v);

    public: bool IsExpanded(void) const {return expanded_;}

    public: bool IsCollapsed(void) const {return !expanded_;}

    public: bool IsSelected(void) const {return selected_;}

    public: bool IsChildItemOf(TreeViewItem* item) const;

    public: bool IsSelectable (void) const {return selectable_;}

    public: bool IsFocused (void) const;

    public: void BringIntoView (void);

    public: SelectionMode GetSelectionMode (void) const;

    /// Returns the next flattened TreeViewItem in the heirarchy that is within an expanded tree
    public: TreeViewItem* GetNextVisibleItem (void) const;

    public: TreeViewItem* GetPrevVisibleItem (void) const;

    /// Returns the first child item 
    public: TreeViewItem* GetFirstChildItem (void) const;

    /// Returns the first child item 
    public: TreeViewItem* GetLastChildItem (void) const;

    /// Returns the next sibling item
    public: TreeViewItem* GetNextSiblingItem (void) const;

    /// Returns the previous sibling item
    public: TreeViewItem* GetPrevSiblingItem (void) const;

    /// Return the parent item or nullptr if parented directly to the treeview
    public: TreeViewItem* GetParentItem (void) const {return parent_item_;}

    public: bool BeginTextEdit (bool delay=false);

    public: void EndTextEdit (bool cancel=false);

    /// Handler for TreeViewItem button being pressed.
    protected: void OnButton(UINode* sender);

    protected: void UpdateExpanded (void);

    private: TreeView* GetTreeView (void) const {return tree_view_;}

    private: void SetTreeView (TreeView* tv, TreeViewItem* parent_item);

    private: void SetSelectedInternal (bool selected, bool set_state);

    private: void AdjustFocusForDelete (void);

    protected: virtual void UpdateAnimationState (void) override;

    protected: virtual void DoDragDrop (const std::vector<ObjectPtr<TreeViewItem>>& items) {noz_assert(false);}

    protected: virtual void OnExpanded (void) { }
    protected: virtual void OnCollapsed (void) { }

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnChildRemoved (Node* child) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;
    protected: virtual void OnMouseOver (SystemEvent* e) override;
    protected: virtual void OnPreviewKeyDown (SystemEvent* e) override;
    protected: virtual void OnDrag (void) {}
    protected: virtual void DoDragDrop (void) final;
    
    private: void OnTextBoxLostFocus (UINode* sender);
    private: void OnTextBoxTextCommited (UINode* sender);
    private: bool HandleSelection (noz_uint32 modifiers);
  };

} // namespace noz


#endif //__noz_Int32_h__

