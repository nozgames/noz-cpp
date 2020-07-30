///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_MessageBox_h__
#define __noz_MessageBox_h__

namespace noz {

  class Button;

  enum class MessageBoxResult {
    Cancel,
    No,
    None,
    Ok,
    Yes
  };

  NOZ_ENUM() enum class MessageBoxButton {
    Ok,
    OkCancel,
    YesNo,
    YesNoCancel
  };

  NOZ_ENUM() enum class MessageBoxImage {
    Error,
    Warning,
    None,
    Question
  };

  class MessageBox : public Control {
    NOZ_OBJECT(DefaultStyle="{9CFEECED-3ECA-4F12-A29C-F47FFECE3C59}")

    NOZ_CONTROL_PART(Name=YesButton)
    private: ObjectPtr<Button> yes_button_;

    NOZ_CONTROL_PART(Name=NoButton)
    private: ObjectPtr<Button> no_button_;

    NOZ_CONTROL_PART(Name=OkButton)
    private: ObjectPtr<Button> ok_button_;

    NOZ_CONTROL_PART(Name=CancelButton)
    private: ObjectPtr<Button> cancel_button_;

    NOZ_CONTROL_PART(Name=MessageText)
    private: ObjectPtr<TextNode> message_text_;

    private: String text_;
    private: MessageBoxButton button_;
    private: MessageBoxImage image_;
    private: MessageBoxResult result_;

    public: static MessageBoxResult Show (Window* parent, const char* text, const char* caption, MessageBoxButton button, MessageBoxImage icon);


    protected: virtual bool OnApplyStyle (void) override;

    private: void OnYesButtonClicked (UINode* sender);
    private: void OnNoButtonClicked (UINode* sender);
    private: void OnCancelButtonClicked (UINode* sender);
    private: void OnOkButtonClicked (UINode* sender);
  };

} // namespace noz


#endif //__noz_MessageBox_h__

