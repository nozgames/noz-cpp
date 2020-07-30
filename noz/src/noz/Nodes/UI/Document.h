///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Document_h__
#define __noz_Document_h__

namespace noz {

  class DocumentManager;
  class DocumentTabItem;

  class Document : public Control {
    NOZ_OBJECT()

    friend class DocumentManager;

    private: String name_;

    private: String path_;

    /// Manager that owns this document (nullptr if unowned)
    private: DocumentManager* manager_;

    private: ObjectPtr<DocumentTabItem> tab_;

    private: bool modified_;

    public: Document(void);

    public: ~Document (void);

    public: bool IsActive (void) const;

    public: void Close (void);

    public: void SetModified(bool modified);

    public: void SetName(const String& name) {name_ = name;}
    public: void SetName(const char* name) {name_ = name;}

    public: void SetPath(const String& path) {path_ = path;}
    public: void SetPath(const char* path) {path_ = path;}

    public: const String& GetName(void) const {return name_;}
    public: const String& GetPath(void) const {return path_;}

    protected: virtual void OnOpen (void) {}

    public: virtual void Save (void) {}

    protected: void UpdateTitle (void);

    protected: virtual void OnActivate(void) {}
    protected: virtual void OnDeactivate(void) {}
  };

} // namespace noz


#endif //__noz_Document_h__

