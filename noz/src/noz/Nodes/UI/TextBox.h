///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TextBox_h__
#define __noz_TextBox_h__

namespace noz {

  class ScrollView;
  class EditableTextNode;

  class TextBox : public Control {
    NOZ_OBJECT(DefaultStyle="{16F8F575-167B-46FB-92FA-BDD77D5101A3}")

    public: ValueChangedEventHandler TextChanged;
    public: ValueChangedEventHandler TextCommited;

    NOZ_CONTROL_PART(Name=EditableTextNode)
    protected: ObjectPtr<EditableTextNode> text_node_;

    NOZ_CONTROL_PART(Name=PlaceholderTextNode)
    protected: ObjectPtr<TextNode> placeholder_text_node_;

    NOZ_CONTROL_PART(Name=ScrollView)
    protected: ObjectPtr<ScrollView> scroll_view_;

    NOZ_PROPERTY(Name=Text,Set=SetText)
    protected: String text_;

    NOZ_PROPERTY(Name=PlaceholderText,Set=SetPlaceholderText)
    protected: String placeholder_text_;

    /// Set to true to select all text in the text box when it gains focus
    NOZ_PROPERTY(Name=SelectOnFocus)
    private: bool select_on_focus_;



    private: noz_int32 selection_drag_;

    public: TextBox (void);

    public: void SetText(const char* text);
    public: void SetText(const String& text) {SetText(text.ToCString());}

    public: void SetPlaceholderText(const char* text);
    public: void SetPlaceholderText(const String& text) {SetPlaceholderText(text.ToCString());}    

    public: const String& GetText(void) const;

    public: void SetSelectOnFocus(bool s) {select_on_focus_ = s;}

    protected: virtual void UpdateAnimationState(void) override;

    protected: void ScrollText (void);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnKeyDown (SystemEvent* e) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseOver (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;
    protected: virtual void OnGainFocus (void) override;
    protected: virtual void OnLoseFocus (void) override;    
  };

} // namespace noz


#endif //__noz_TextBox_h__

