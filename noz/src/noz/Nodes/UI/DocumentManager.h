///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DocumentManager_h__
#define __noz_DocumentManager_h__

#include "TabControl.h"
#include "DocumentTabItem.h"

namespace noz {

  class Document;

  class DocumentManager : public Control {
    NOZ_OBJECT(DefaultStyle="{199C014B-B9C1-48C4-BFE3-15174B857BF2}")

    friend class Document;

    NOZ_CONTROL_PART(Name=TabControl)
    private: ObjectPtr<TabControl> tab_control_;

    /// All document managed by the manager
    private: std::vector<ObjectPtr<Document>> documents_;

    /// Selected document
    private: ObjectPtr<Document> selected_;

    public: DocumentManager(void);

    public: ~DocumentManager(void);

    public: const std::vector<ObjectPtr<Document>>& GetDocuments(void) const {return documents_;}

    public: void SetActiveDocument (Document* document);

    public: Document* GetActiveDocument (void) const {return selected_;}

    /// Add a document to the manager.
    public: void AddDocument (Document* document);

    public: void CloseDocument (Document* document);

    private: void AddDocumentTab (Document* document);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnKeyDown (SystemEvent* e) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnStyleChanged (void) override;

    private: void OnTabSelectionChanged (UINode* sender);
  };

} // namespace noz


#endif //__noz_DocumentManager_h__

