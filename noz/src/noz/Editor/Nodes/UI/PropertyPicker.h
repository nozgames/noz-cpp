///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_PropertyPicker_h__
#define __noz_Editor_PropertyPicker_h__

#include <noz/Nodes/UI/TreeViewItem.h>

namespace noz {

  class TreeView;
  class SearchTextBox;

namespace Editor {

  class PropertyPickerItem : public TreeViewItem {
    NOZ_OBJECT(DefaultStyle="{187DEDDC-4C71-4F70-91D1-E48A02F10D5A}")

    private: ObjectPtr<Object> target_;
    private: Property* target_property_;

    public: PropertyPickerItem(Object* o=nullptr, Property* p=nullptr);
  };

  class PropertyPicker : public Control {
    NOZ_OBJECT(DefaultStyle="{A85B9264-F254-4A13-86CE-529D455D54B6}")

    NOZ_CONTROL_PART(Name=SearchTextBox)
    private: ObjectPtr<SearchTextBox> search_box_;

    NOZ_CONTROL_PART(Name=TreeView)
    private: ObjectPtr<TreeView> tree_view_;

    /// If true any components of a node will be shown.
    NOZ_PROPERTY(Name=ShowComponents)
    private: bool show_components_;

    /// If true any children of anode will be shown
    NOZ_PROPERTY(Name=ShowChildren)
    private: bool show_children_;

    private: String filter_text_;

    /// Source object to pick properties from 
    private: ObjectPtr<Object> source_;

    public: PropertyPicker (void);

    public: ~PropertyPicker (void);

    public: void SetSource (Object* o);

    public: void SetFilterText (const char* text);

    protected: virtual bool OnApplyStyle (void) override;

    protected: void Refresh (void);

    private: void OnSearchTextChanged (UINode* sender);
    private: void AddItems (Object* o, PropertyPickerItem* item);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_PropertyPicker_h__

