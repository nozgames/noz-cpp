///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DocumentTabItem_h__
#define __noz_DocumentTabItem_h__

#include "TabItem.h"

namespace noz {

  class TextNode;
  class Button;
  class Document;

  class DocumentTabItem : public TabItem {
    NOZ_OBJECT(DefaultStyle="{78ABBB41-4BAE-45F4-B368-1CBB38D38292}")

    NOZ_CONTROL_PART(Name=CloseButton)
    private: ObjectPtr<Button> close_button_;

    /// Document that is associated with this tab item.
    private: ObjectPtr<Document> document_;

    /// Default constructor
    public: DocumentTabItem(void);

    /// Default destructor
    public: ~DocumentTabItem (void);

    public: void SetDocument (Document* document);

    public: Document* GetDocument(void) const {return document_;}

    protected: bool OnApplyStyle (void) override;

    private: void OnCloseButton (UINode* sender);
  };

} // namespace noz


#endif //__noz_DocumentTabItem_h__

