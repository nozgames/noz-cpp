///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Control_h__
#define __noz_Control_h__

#include "UINode.h"
#include "../../UI/Style.h"

namespace noz {

  class Control : public UINode {
    NOZ_OBJECT()

    friend class Style;
    friend class Style::Template;

    /// Forced style set on control
    protected: NOZ_PROPERTY(Name=Style,IsDefault=IsStyleDefault,Set=SetStyle) ObjectPtr<Style> style_;

    /// Style sheet applied to all decendant controls
    protected: NOZ_PROPERTY(Name=StyleSheet) ObjectPtr<StyleSheet> style_sheet_;

    struct {
      /// True when a custom style is set d irectly on the control using SetStyle or
      /// by setting the property.  If this value is false then the style s on the control 
      /// was automatically set by either a style sheet or the default style.
      noz_byte custom_style_ : 1;

      noz_byte styled_ : 1;

      /// True if the style applied to the control is valid.  
      noz_byte style_valid_ : 1;
    };
 
    public: Control (void);

    public: ~Control (void);
       
    public: bool IsStyled (void) const {return styled_;}

    public: bool IsStyleValid (void) const {return style_valid_;}

    public: void SetStyle (Style* style);

    public: Style* GetStyle (void) const {return style_;}

    protected: void SetAnimationState (const Name& state) override;

    private: void ApplyStyle (void);

    public: virtual Vector2 Measure (const Vector2& a) override;

    protected: virtual bool OnApplyStyle (void) {return true;}
    protected: virtual void OnGainFocus(void) override;
    protected: virtual void OnInteractiveChanged (void) override;
    protected: virtual void OnLoseFocus(void) override;
    protected: virtual void OnMouseEnter(void) override;
    protected: virtual void OnMouseLeave(void) override;
    protected: virtual void OnStyleChanged (void) {}

    protected: virtual void UpdateAnimationState (void);

    private: bool IsStyleDefault(void) const {return !custom_style_;}
  };

} // namespace noz


#endif // __noz_Control_h__

