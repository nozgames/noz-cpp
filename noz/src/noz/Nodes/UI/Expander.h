///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Expander_h__
#define __noz_Expander_h__

namespace noz {

  class ToggleButton;

  class Expander : public ContentControl {
    NOZ_OBJECT(DefaultStyle="{00334F68-2EAE-4A4D-83E7-4A140E47E9FB}")

    /// Button control part for expanding and collapsing the expander
    NOZ_CONTROL_PART(Name=ExpandButton)
    private: ObjectPtr<ToggleButton> expand_button_;

    /// Node control part that will be collapsed when the expander is collapsed
    NOZ_CONTROL_PART(Name=ContentContainer)
    private: ObjectPtr<Node> content_container_;
   
    /// True if the expander is expanded.
    NOZ_PROPERTY(Name=Expanded,Set=SetExpanded)
    private: bool expanded_;

    public: Expander (void);

    public: ~Expander (void);
  
    public: void SetExpanded (bool expanded);

    public: bool IsExpanded(void) const {return expanded_;}

    public: bool IsCollapsed(void) const {return !expanded_;}

    /// Handler for expander button being pressed.
    protected: void OnButton(void);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnStyleChanged (void) override;

    protected: void UpdateExpandedState (void);
    protected: virtual void UpdateAnimationState (void) override;
  };

} // namespace noz


#endif // __noz_Expander_h__

