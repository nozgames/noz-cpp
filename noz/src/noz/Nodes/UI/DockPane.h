///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DockPane_h__
#define __noz_DockPane_h__

namespace noz {

  class TabControl;
  class Button;
  class DockItem;
  class DockTabItem;
  class DockManager;

  NOZ_ENUM() enum class DockStyle {
    Left,
    Right,
    Top,
    Bottom,
    Float
  };

  class DockPane : public Control {
    NOZ_OBJECT(DefaultStyle="{692CDC68-E9AF-4F2D-8F33-555BDA5F9556}")

    friend class DockManager;

    NOZ_CONTROL_PART(Name=TabControl)
    private: ObjectPtr<TabControl> tab_control_;

    NOZ_PROPERTY(Name=Dock,Set=SetDock)
    private: DockStyle dock_style_;

    private: DockManager* manager_;

    private: bool active_;

    public: DockPane(void);

    public: DockStyle GetDock (void) const {return dock_style_;}

    public: void InvalidateDock (void);

    public: void SetDock (DockStyle style);

    private: DockItem* GetActiveItem (void) const;

    private: DockTabItem* GetActiveTab (void) const;

    private: void OnTabSelectionChanged (UINode* sender);
    private: void OnCloseButton (UINode* sender);

    protected: virtual void UpdateAnimationState (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnChildAdded (Node* child) override;

    private: void OnFocusChanged(Window* window, UINode* new_focus);
  };

} // namespace noz


#endif //__noz_DockPane_h__

