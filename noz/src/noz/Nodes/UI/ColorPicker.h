///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ColorPicker_h__
#define __noz_ColorPicker_h__

namespace noz {  

  class TextBox;

  class ColorPicker : public Control {
    NOZ_OBJECT(DefaultStyle="{929B814A-CAD1-438C-AFCC-32F5C5669868}")

    public: ValueChangedEventHandler ColorChanged;

    NOZ_CONTROL_PART(Name=AlphaNode)
    private: ObjectPtr<SpriteNode> alpha_node_;

    NOZ_CONTROL_PART(Name=ColorNode)
    private: ObjectPtr<SpriteNode> color_node_;

    NOZ_CONTROL_PART(Name=Popup)
    private: ObjectPtr<Popup> popup_;

    NOZ_CONTROL_PART(Name=TextBoxR)
    private: ObjectPtr<TextBox> text_box_r_;

    NOZ_CONTROL_PART(Name=TextBoxG)
    private: ObjectPtr<TextBox> text_box_g_;

    NOZ_CONTROL_PART(Name=TextBoxB)
    private: ObjectPtr<TextBox> text_box_b_;

    NOZ_CONTROL_PART(Name=TextBoxA)
    private: ObjectPtr<TextBox> text_box_a_;

    NOZ_CONTROL_PART(Name=TextBoxHex)
    private: ObjectPtr<TextBox> text_box_hex_;

    NOZ_PROPERTY(Name=Color,Set=SetColor)
    private: Color color_;

    private: bool popup_open_;

    /// Default ColorPicker constructor
    public: ColorPicker (void);

    public: Color GetColor (void) const {return color_;}
    
    public: void SetColor (Color color);
    
    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;

    /// Override to handle state changes
    protected: virtual void UpdateAnimationState (void) override;

    private: void OnTextCommittedRBGA (UINode* sender);
    private: void OnTextCommittedHex (UINode* sender);
    
    private: void OnColorPickerPopupColorChanged (Color color);
  };

} // namespace noz


#endif //__noz_ColorPicker_h__

