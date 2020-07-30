///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ButtonBase_h__
#define __noz_ButtonBase_h__

namespace noz {

  NOZ_ENUM() enum class ClickMode {
    Press,    /// Issue the onClick event on Mouse down
    Release   /// Issue the onClick event on Mouse up
  };

  class ButtonBase : public ContentControl {
    NOZ_OBJECT(Abstract,EditorIcon="{07B5E6E9-4FED-4FE3-9768-B9F7C4046025}")

    /// Event fired when button is clicked
    public: ClickEventHandler Click;

    NOZ_CONTROL_PART(Name=ContentContainer)
    protected: ObjectPtr<Node> content_container_;

    NOZ_PROPERTY(Name=ClickMode)
    protected: ClickMode click_mode_;

    /// Default ButtonBase constructor
    public: ButtonBase (void);


    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnClick (void);
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;
    protected: virtual void OnStyleChanged (void) override;

    /// Override to handle state changes
    protected: virtual void UpdateAnimationState (void) override;
  };

} // namespace noz


#endif //__noz_ButtonBase_h__

