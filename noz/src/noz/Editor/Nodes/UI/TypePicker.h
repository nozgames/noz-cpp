///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_TypePicker_h__
#define __noz_Editor_TypePicker_h__

#include <noz/Nodes/UI/ListViewItem.h>

namespace noz { class TextBox; }
namespace noz { class Button; }
namespace noz { class ListView; }

namespace noz {
namespace Editor {

  class TypePickerItem : public ListViewItem {
    NOZ_OBJECT(DefaultStyle="{22EDE59C-4B6E-4749-8F29-C0BAC955F5BF}")

    public: TypePickerItem(void);

    protected: virtual void DoDragDrop (void) override;
  };

  class TypePicker : public Control {
    NOZ_OBJECT(DefaultStyle="{440D9B90-92AF-4DA2-B362-40230DDC2BB9}")

    public: TypeSelectedEventHandler SelectionChanged;

    private: NOZ_CONTROL_PART(Name=TextBox) ObjectPtr<TextBox> text_box_;
    private: NOZ_CONTROL_PART(Name=ListView) ObjectPtr<ListView> list_view_;
    private: NOZ_PROPERTY(Name=BaseType,Set=SetBaseType) Type* base_type_;
    private: NOZ_PROPERTY(Name=FilterEditorOnly) bool filter_editor_only_;
    private: NOZ_PROPERTY(Name=FilterNoAllocator) bool filter_no_allocator_;

    private: std::vector<Type*> types_;

    private: String filter_text_;

    public: TypePicker (void);

    public: ~TypePicker (void);

    public: void SetBaseType (Type* type);

    public: void Clear (void);

    public: void SetFilterText (const char* text);

    public: Type* GetSelected (void) const;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnGainFocus (void) override;

    protected: void Refresh (void);

    private: void OnSearchTextChanged (UINode* sender);

    private: void OnListViewSelectionChanged (UINode* sender);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_TypePicker_h__

