///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ListView_h__
#define __noz_ListView_h__

#include "ListViewItem.h"

namespace noz {

  class ListViewItem;
  class ScrollView;

  typedef Event<UINode*,ListViewItem*,const String*> ListViewTextChangedEventHandler;

  class ListView : public Control {
    NOZ_OBJECT(DefaultStyle="{71B64669-D01E-4F1F-86B3-644766174A30}")

    friend class ListViewItem;

    /// Event raised when selection within the ListVhew changes
    public: SelectionChangedEventHandler SelectionChanged;
    public: ListViewTextChangedEventHandler ItemTextChanged;

    /// Part used to add items to
    protected: NOZ_CONTROL_PART(Name=ItemsContainer) ObjectPtr<Node> items_container_;
    protected: NOZ_CONTROL_PART(Name=ScrollView) ObjectPtr<ScrollView> scroll_view_;
    private: NOZ_PROPERTY(Name=SelectionMode) SelectionMode selection_mode_;

    /// Vector of selected items.
    private: std::vector<ObjectPtr<ListViewItem>> selected_items_;

    /// Item that currently has focus for keyboard navigation
    private: ObjectPtr<ListViewItem> focus_item_;

    /// Item that is the anchor for multiple selection
    private: ObjectPtr<ListViewItem> selection_anchor_;

    /// Default constructor
    public: ListView (void);

    /// Default destructor
    public: ~ListView (void);

    /// Clear selected items list
    public: void UnselectAll (void);

    public: ListViewItem* GetFirstChildItem (void) const;

    public: ListViewItem* GetLastChildItem (void) const;

    /// Return the item that currently has focus or nullptr of none
    public: ListViewItem* GetFocusItem (void) const {return focus_item_;}

    public: ListViewItem* GetSelectedItem (void) const {return selected_items_.empty() ? nullptr : selected_items_[0];}

    public: const std::vector<ObjectPtr<ListViewItem>>& GetSelectedItems (void) const {return selected_items_;}

    /// Sort the list view using the given sort proc
    public: virtual void SortChildren (noz_int32 (*sort_proc) (const Node* node1, const Node* node2)) override;

    /// Internal method used to clear the current selection
    private: void UnselectAllNoEvent (void);

    /// Insertnal method used to set the current focus item
    private: void SetFocusedItem (ListViewItem* lvitem, bool anchor=false);
   
    private: void OnFocusChanged(Window* window, UINode* new_focus);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnChildRemoved (Node* child) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseWheel (SystemEvent* e) override;    
    protected: virtual void OnKeyDown (SystemEvent* e) override;
  };

} // namespace noz


#endif // __noz_ListView_h__

