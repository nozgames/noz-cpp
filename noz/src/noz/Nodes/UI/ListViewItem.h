///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ListViewItem_h__
#define __noz_ListViewItem_h__

#include "SelectionMode.h"

namespace noz {

  class ListView;
  class TextBox;

  class ListViewItem : public ContentControl {
    NOZ_OBJECT(DefaultStyle="{22EDE59C-4B6E-4749-8F29-C0BAC955F5BF}")

    private: NOZ_CONTROL_PART(Name=TextBox) ObjectPtr<TextBox> text_box_;

    friend class ListView;

    private: struct {
      /// True if the item is currently selected
      noz_byte selected_ : 1;

      /// True if a text edit operation is pending
      noz_byte text_edit_pending_ : 1;
    };

    /// List view the item is contained in
    private: ObjectPtr<ListView> list_view_;
    
    private: void* user_data_;

    /// Default constructor
    public: ListViewItem (void);
  
    public: void BringIntoView (void);

    /// Set the selected state of the item
    public: void SetSelected (bool selected);

    /// Returns true if the item is selected
    public: bool IsSelected(void) const {return selected_;}

    /// Return the list view that the item is contained within
    public: ListView* GetListView (void) const {return list_view_;}

    /// Returns the next sibling item
    public: ListViewItem* GetNextSiblingItem (void) const;

    /// Returns the previous sibling item
    public: ListViewItem* GetPrevSiblingItem (void) const;

    public: bool BeginTextEdit (bool delay=false);

    public: void EndTextEdit (bool cancel=false);

    public: void SetUserData (void* o) {user_data_ = o;}

    public: void* GetUserData (void) const {return user_data_;}

    public: SelectionMode GetSelectionMode (void) const;

    private: void SetSelectedInternal (bool selected, bool set_state);

    private: void SetListView(ListView* lv);

    private: bool HandleSelection (noz_uint32 modifiers);

    protected: virtual void UpdateAnimationState (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;
    protected: virtual void OnPreviewKeyDown (SystemEvent* e) override;
    protected: virtual void Update (void) override;
    
    protected: virtual void OnOpen (void) { }
    
    protected: virtual bool DragStart (void) {return false;}

    private: void OnTextBoxLostFocus (UINode* sender);
    private: void OnTextBoxTextCommited (UINode* sender);
  };

} // namespace noz


#endif //__noz_ListViewItem_h__

