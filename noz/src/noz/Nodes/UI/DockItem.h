///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DockItem_h__
#define __noz_DockItem_h__

#include "TabItem.h"

namespace noz {

  class DockManager;
  class DockPane;
  class DockTabItem;

  class DockItem : public UINode {
    NOZ_OBJECT()

    friend class DockManager;
    friend class DockPane;

    /// DockManager the dock item is being managed by
    private: DockManager* manager_;

    /// DockPane the item belongs to
    private: ObjectPtr<DockPane> pane_;

    /// Tab associated with dock item
    private: ObjectPtr<DockTabItem> tab_;

    NOZ_PROPERTY(Name=Sprite,Set=SetSprite)
    private: ObjectPtr<Sprite> sprite_;

    NOZ_PROPERTY(Name = Text, Set = SetText)
    private: String text_;

    private: ObjectPtr<Node> content_container_;

    /// Index of the item within the items list 
    private: noz_uint32 index_;

    public: DockItem(void);

    public: DockPane* GetPane (void) const {return pane_;}

    public: void Hide (void);

    public: void Show (void);

    public: void Select (void);

    public: void SetText (const char* text);
    public: void SetText (const String& text) {SetText(text.ToCString());}

    public: void SetSprite (Sprite* sprite);

    public: DockManager* GetManager (void) const {return manager_;}

    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void Update (void) override;
  };

} // namespace noz


#endif //__noz_DockItem_h__

