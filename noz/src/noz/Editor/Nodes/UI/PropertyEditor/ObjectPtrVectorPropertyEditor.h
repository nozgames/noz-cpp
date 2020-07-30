///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ObjectPtrVectorPropertyEditor_h__
#define __noz_Editor_ObjectPtrVectorPropertyEditor_h__

#include "PropertyEditor.h"
#include <noz/Nodes/UI/Expander.h>
#include <noz/Nodes/UI/ListView.h>

namespace noz {
namespace Editor {

  class ObjectPtrVectorPropertyEditor;

  class ObjectPtrVectorPropertyEditorItem : public ListViewItem {
    NOZ_OBJECT(DefaultStyle="{FF662EA7-7197-4E74-BEF1-7B2ACE949FA5}")
        
    friend class ObjectPtrVectorPropertyEditor;

    private: ObjectPtrVectorPropertyEditor* editor_;
    
    public: ObjectPtrVectorPropertyEditorItem(void);

    protected: virtual void OnDragDrop (DragDropEventArgs* args) override;

    private: bool GetValueFromArgs (DragDropEventArgs* args, Object** value=nullptr) const;
    private: void SetObject(Object* o);
  };

  class ObjectPtrVectorPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{65688BE0-9181-4CA4-8DD1-97D4F3727553}",EditorProperty=noz::ObjectPtrVectorProperty)

    private: NOZ_CONTROL_PART(Name=AddButton) ObjectPtr<Button> add_button_;
    private: NOZ_CONTROL_PART(Name=RemoveButton) ObjectPtr<Button> remove_button_;
    private: NOZ_CONTROL_PART(Name=Expander) ObjectPtr<Expander> expander_;
    private: NOZ_CONTROL_PART(Name=ListView) ObjectPtr<ListView> list_view_;
    private: NOZ_CONTROL_PART(Name=EmptyText) ObjectPtr<TextNode> empty_text_;

    public: ObjectPtrVectorPropertyEditor (void);

    public: ~ObjectPtrVectorPropertyEditor (void);

    public: virtual bool IsExpander (void) const {return true;}

    public: Type* GetValueType(void) const;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void WriteProperty (Object* target, Property* prop) final;
    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    private: void OnAdd (UINode*);
    private: void OnRemove (UINode*);
    private: void RefreshListView (void);
    private: void AddItem (Object* o);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_ObjectPtrVectorPropertyEditor_h__

