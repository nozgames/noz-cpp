///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ColorPickerPopup_h__
#define __noz_ColorPickerPopup_h__

namespace noz {
  
  class TextBox;

  class ColorPickerSlider : public UINode {
    NOZ_OBJECT()

    public: ValueChangedEventHandler ValueChanged;

    NOZ_PROPERTY(Name=Horizontal)
    private: bool horizontal_;

    NOZ_PROPERTY(Name=Vertical)
    private: bool vertical_;

    private: Vector2 value_;
    private: ObjectPtr<Node> content_container_;

    public: ColorPickerSlider (void);

    public: const Vector2& GetValue (void) const {return value_;}

    public: void SetValue (const Vector2& v);

    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;
    protected: virtual void OnMouseOver (SystemEvent* e) override;
    protected: virtual void ArrangeChildren (const Rect& r) override;
  };

  typedef Event<Color> ColorChangedEvent;

  class ColorPickerPopup : public Control {
    NOZ_OBJECT(DefaultStyle="{F6A9D752-B224-4DB7-810A-3076413FCF00}")

    NOZ_CONTROL_PART(Name=ColorSquare)
    private: ObjectPtr<ColorPickerSlider> color_square_;

    NOZ_CONTROL_PART(Name=SliderA)
    private: ObjectPtr<ColorPickerSlider> slider_a_;

    NOZ_CONTROL_PART(Name=SliderR)
    private: ObjectPtr<ColorPickerSlider> slider_r_;

    NOZ_CONTROL_PART(Name=SliderG)
    private: ObjectPtr<ColorPickerSlider> slider_g_;

    NOZ_CONTROL_PART(Name=SliderB)
    private: ObjectPtr<ColorPickerSlider> slider_b_;

    NOZ_CONTROL_PART(Name=SliderH1)
    private: ObjectPtr<ColorPickerSlider> slider_h1_;

    NOZ_CONTROL_PART(Name=SliderH2)
    private: ObjectPtr<ColorPickerSlider> slider_h2_;

    NOZ_CONTROL_PART(Name=SliderS)
    private: ObjectPtr<ColorPickerSlider> slider_s_;

    NOZ_CONTROL_PART(Name=SliderV)
    private: ObjectPtr<ColorPickerSlider> slider_v_;

    NOZ_CONTROL_PART(Name=ColorsA)
    private: ObjectPtr<SpriteColors> colors_a_;

    NOZ_CONTROL_PART(Name=ColorsR)
    private: ObjectPtr<SpriteColors> colors_r_;

    NOZ_CONTROL_PART(Name=ColorsG)
    private: ObjectPtr<SpriteColors> colors_g_;

    NOZ_CONTROL_PART(Name=ColorsB)
    private: ObjectPtr<SpriteColors> colors_b_;

    NOZ_CONTROL_PART(Name=ColorsS)
    private: ObjectPtr<SpriteColors> colors_s_;

    NOZ_CONTROL_PART(Name=ColorsV)
    private: ObjectPtr<SpriteColors> colors_v_;

    NOZ_CONTROL_PART(Name=ColorSquareColors)
    private: ObjectPtr<SpriteColors> color_quare_colors_;

    NOZ_CONTROL_PART(Name=TextR)
    private: ObjectPtr<TextBox> text_r_;

    NOZ_CONTROL_PART(Name=TextG)
    private: ObjectPtr<TextBox> text_g_;

    NOZ_CONTROL_PART(Name=TextB)
    private: ObjectPtr<TextBox> text_b_;

    NOZ_CONTROL_PART(Name=TextA)
    private: ObjectPtr<TextBox> text_a_;

    NOZ_CONTROL_PART(Name=TextH)
    private: ObjectPtr<TextBox> text_h_;

    NOZ_CONTROL_PART(Name=TextS)
    private: ObjectPtr<TextBox> text_s_;

    NOZ_CONTROL_PART(Name=TextV)
    private: ObjectPtr<TextBox> text_v_;

    NOZ_CONTROL_PART(Name=TextHex)
    private: ObjectPtr<TextBox> text_hex_;

    private: Color original_color_;

    private: Color color_;

    private: Vector3 rgb_;
    private: Vector3 hsv_;
    private: noz_float alpha_;
    private: bool refresh_;

    private: static ColorPickerPopup* this_;

    private: ColorChangedEvent::Delegate delegate_;

    private: ObjectPtr<Popup> popup_;

    public: ColorPickerPopup (void);

    public: ~ColorPickerPopup (void);

    public: static void Show (Color color_, PopupPlacement placement, const Vector2& placement_offset, Node* placement_target, const ColorChangedEvent::Delegate& delegate);

    public: static void Hide (void);

    public: static bool IsOpen (void) {return this_ && this_->popup_->IsOpen();}

    private: void Refresh (void);

    protected: virtual bool OnApplyStyle(void) override;
    protected: virtual void OnKeyDown (SystemEvent* e) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;

    private: void OnSliderR (UINode* sender);
    private: void OnSliderG (UINode* sender);
    private: void OnSliderB (UINode* sender);
    private: void OnSliderA (UINode* sender);
    private: void OnHueChanged (UINode* sender);
    private: void OnColorSquareValueChanged (UINode* sender);
    private: void OnTextHex (UINode* sender);
    private: void OnTextR (UINode* sender);
    private: void OnTextG (UINode* sender);
    private: void OnTextB (UINode* sender);
    private: void OnTextA (UINode* sender);

    private: void UpdateRGB (void);
    private: void UpdateHSV (void);
  };

} // namespace noz


#endif //__noz_ColorPickerPopup_h__

