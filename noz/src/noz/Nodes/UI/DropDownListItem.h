///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DropDownListItem_h__
#define __noz_DropDownListItem_h__

namespace noz {

  class DropDownList;

  class DropDownListItem : public ContentControl {
    NOZ_OBJECT(DefaultStyle="{28D4D93D-DA90-4571-B512-1DC89E792B56}")

    friend class DropDownList;

    /// List view the item is contained in
    private: ObjectPtr<DropDownList> drop_down_list_;
        
    /// User data associated with item
    private: void* user_data_;

    /// Default constructor
    public: DropDownListItem (void);
  
    /// Set the item as selected
    public: void Select (void);

    /// Returns true if the item is selected
    public: bool IsSelected(void) const;

    /// Return the list view that the item is contained within
    public: DropDownList* GetDropDownList (void) const {return drop_down_list_;}

    /// Returns the next sibling item
    public: DropDownListItem* GetNextSiblingItem (void) const;

    /// Returns the previous sibling item
    public: DropDownListItem* GetPrevSiblingItem (void) const;

    /// Set the user data associated with the drop down list item.  Note the caller is responsible for 
    /// freeing any memory related to the user data.
    public: void SetUserData (void* o) {user_data_ = o;}

    /// Return the user data associated with the item
    public: void* GetUserData (void) const {return user_data_;}

    protected: virtual void UpdateAnimationState (void) override;

    protected: virtual void OnMouseDown (SystemEvent* e) override;
  };

} // namespace noz


#endif //__noz_DropDownListItem_h__

