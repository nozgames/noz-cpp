///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_Inspector_h__
#define __noz_Editor_Inspector_h__

#include "InspectorRow.h"

namespace noz {
namespace Editor {

  class EditorDocument;
  
  class Inspector : public Control {
    NOZ_OBJECT(DefaultStyle="{568D52D1-F178-4256-9B91-2D94CDC44F32}")

    NOZ_CONTROL_PART(Name=ContentContainer)
    private: ObjectPtr<Node> content_container_;

    /// Object being inspected
    private: ObjectPtr<Object> target_;

    /// Document that the inspecor is associated with
    private: ObjectPtr<EditorDocument> document_;

    /// Default constructor
    public: Inspector (void);

    /// Destructor
    public: ~Inspector (void);

    /// Set the target being inspected
    public: void SetTarget (Object* target);

    /// Attach the inspector to a document
    public: void SetDocument (EditorDocument* document);

    /// Return the target object being inspected
    public: Object* GetTarget (void) const {return target_;}

    /// Return the document associated with the inspector
    public: EditorDocument* GetDocument (void) const {return document_;}

    protected: virtual bool FilterProperty (Property* p) const;

    protected: virtual void OnSetTarget (Object* t);

    protected: virtual bool OnApplyStyle (void) override;

    /// Add an sub-inspector to the content container.
    protected: void AddInspector (Object* target);

    /// Add target properties to the given container node
    protected: void AddProperties(Node* container);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_Inspector_h__

