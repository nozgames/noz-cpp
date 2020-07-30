///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_HierarchyItem_h__
#define __noz_Editor_HierarchyItem_h__

#include <noz/Nodes/UI/TreeViewItem.h>

namespace noz {
namespace Editor {

  class Hierarchy;

  class HierarchyItem : public TreeViewItem {
    NOZ_OBJECT(DefaultStyle="{8B24F407-32CA-4477-9002-C5866F8206FF}")

    friend class Hierarchy;

    /// Control part that represents the hit region for a drag drop before the item
    NOZ_CONTROL_PART(Name=DragBefore)
    private: ObjectPtr<Node> drag_before_;

    /// Control part that represents the hit region for a drag drop after the item
    NOZ_CONTROL_PART(Name=DragAfter)
    private: ObjectPtr<Node> drag_after_;

    /// Pointer to hierarchy that owns the item
    private: Hierarchy* hierarchy_;

    private: ObjectPtr<Node> target_;

    public: HierarchyItem (Hierarchy* hierarchy, Node* target);

    public: ~HierarchyItem (void);

    public: Node* GetTarget(void) const {return target_;}

    private: void UpdateText (void);    

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnDragDrop (DragDropEventArgs* args) override;
    protected: virtual void DoDragDrop (const std::vector<ObjectPtr<TreeViewItem>>& items) override;
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_HierarchyItem_h__

