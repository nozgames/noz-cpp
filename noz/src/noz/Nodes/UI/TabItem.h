///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TabItem_h__
#define __noz_TabItem_h__

namespace noz {

  class TabControl;

  class TabItem : public ContentControl {
    NOZ_OBJECT(DefaultStyle="{5261273F-A8BC-41C1-AF04-682F2003ACE8}")

    friend class TabControl;

    private: ObjectPtr<Node> content_container_;

    /// Pointer to tabcontrol that owns this item
    private: TabControl* tc_;

    /// Default constructor
    public: TabItem(void);

    /// Default destructor
    public: ~TabItem (void);

    public: bool IsSelected (void) const;

    /// Set the tab as the selected tab
    public: void Select (void);

    /// Return the tab control that the item is a member of
    public: TabControl* GetTabControl(void) const {return tc_;}

    /// Returns the next sibling item
    public: TabItem* GetNextSiblingItem (void) const;

    /// Returns the previous sibling item
    public: TabItem* GetPrevSiblingItem (void) const;

    private: void SetTabControl (TabControl* t);

    protected: virtual void UpdateAnimationState (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
  };

} // namespace noz


#endif //__noz_TabItem_h__

