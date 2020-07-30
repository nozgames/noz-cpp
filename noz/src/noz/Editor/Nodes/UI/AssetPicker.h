///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetPicker_h__
#define __noz_Editor_AssetPicker_h__

namespace noz {

  class TreeView;
  class ListView;
  class TextBox;
  class Button;

namespace Editor {

  class AssetPicker : public Control {
    NOZ_OBJECT(DefaultStyle="{7F28EE3D-1B4F-4221-B86F-B9ED6E2FA0CF}")

    public: AssetSelectedEventHandler SelectionChanged;

    private: NOZ_CONTROL_PART(Name=ListView) ObjectPtr<ListView> list_view_;
    private: NOZ_CONTROL_PART(Name=TextBox) ObjectPtr<TextBox> text_box_;
    private: NOZ_CONTROL_PART(Name=NoneButton) ObjectPtr<Button> none_button_;

    private: String filter_text_;

    public: AssetPicker (void);

    public: ~AssetPicker (void);

    public: void SetFilterText (const char* text);

    public: void Clear (void);

    protected: virtual bool OnApplyStyle (void) override;

    protected: virtual void OnLineageChanged(void) override;

    protected: void Refresh (void);

    private: void OnListViewSelectionChanged (UINode* sender);
    private: void OnSearchTextChanged (UINode* sender);
    private: void OnNoneButtonClicked (UINode* sender);

  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AssetPicker_h__

