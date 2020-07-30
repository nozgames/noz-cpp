///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TabControl_h__
#define __noz_TabControl_h__

namespace noz {

  class TabItem;

  class TabControl : public Control {
    NOZ_OBJECT(DefaultStyle="{58FFDA73-2BD9-4731-AFBB-90B38B78437C}")

    friend class TabItem;

    public: SelectionChangedEventHandler SelectionChanged;

    /// Node to visually parent all TabItem content to
    NOZ_CONTROL_PART(Name=ContentContainer)
    private: ObjectPtr<Node> content_container_;

    /// Part used to add items to
    NOZ_CONTROL_PART(Name=ItemsContainer)
    protected: ObjectPtr<Node> items_container_;

    /// Selected tab item.
    private: ObjectPtr<TabItem> selected_;

    private: bool active_;

    /// Default constructor
    public: TabControl(void);

    /// Destructor
    public: ~TabControl (void);

    public: TabItem* GetFirstChildItem (void) const;

    public: TabItem* GetLastChildItem (void) const;

    public: TabItem* GetSelected (void) const {return selected_;}

    private: void OnFocusChanged(Window* window, UINode* new_focus);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void UpdateAnimationState (void) override;
    protected: virtual void OnGainFocus (void) override;
    protected: virtual void OnLoseFocus (void) override;
    protected: virtual void OnStyleChanged (void) override;
  };

} // namespace noz


#endif //__noz_TabControl_h__

