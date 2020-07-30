///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DropDownList_h__
#define __noz_DropDownList_h__

#include "DropDownListItem.h"

namespace noz {

  class ScrollView;

  class DropDownList : public Control {
    NOZ_OBJECT(DefaultStyle="{4DFCD83C-99E3-4D37-BE25-B7811BD10BDA}")

    friend class DropDownListItem;

    /// Event raised when selection within the ListVhew changes
    public: ValueChangedEventHandler SelectionChanged;
    
    NOZ_CONTROL_PART(Name=ItemsContainer)
    protected: ObjectPtr<Node> items_container_;

    NOZ_CONTROL_PART(Name=Popup)
    protected: ObjectPtr<Popup> popup_;

    NOZ_CONTROL_PART(Name=TextNode)
    private: ObjectPtr<TextNode> text_node_;

    NOZ_CONTROL_PART(Name=SpriteNode)
    private: ObjectPtr<SpriteNode> sprite_node_;

    NOZ_PROPERTY(Name=SelectedItemIndex,Set=SetSelectedItemIndex)
    private: noz_uint32 selected_item_index_;

    /// Default constructor
    public: DropDownList (void);

    /// Default destructor
    public: ~DropDownList (void);

    public: void SetSelectedItemIndex (noz_uint32 index);

    public: DropDownListItem* GetFirstChildItem (void) const;

    public: DropDownListItem* GetLastChildItem (void) const;

    /// Clear selected items list
    public: void UnselectAll (void);

    /// Return the selected item
    public: DropDownListItem* GetSelectedItem (void) const {
      return selected_item_index_==-1?nullptr:(DropDownListItem*)GetLogicalChild(selected_item_index_);}

    /// Return the index of the selected item or -1 of no items are selected
    public: noz_uint32 GetSelectedItemIndex (void) const {return selected_item_index_;}

    /// Sort the list view using the given sort proc
    public: virtual void SortChildren (noz_int32 (*sort_proc) (const Node* lvitem1, const Node* lvitem2)) override;
  
    private: void OnFocusChanged(Window* window, UINode* new_focus);

    private: void CommitSelection (void);
    private: void UnselectAllNoEvent (void);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnChildRemoved (Node* child) override;
    protected: virtual void OnDeserialized (void) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;
    protected: virtual void OnMouseWheel (SystemEvent* e) override;

    protected: virtual void UpdateAnimationState (void) override;
    protected: virtual void UpdateSelectedItem (void);
  };

} // namespace noz


#endif // __noz_DropDownList_h__

