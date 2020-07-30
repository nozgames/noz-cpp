///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_UI_UIStates_h__
#define __noz_UI_UIStates_h__

namespace noz {
namespace UI {

  /// Common states
  extern Name StateNormal;
  extern Name StateMouseOver;
  extern Name StateDisabled;
  extern Name StatePressed;

  /// Checked/Unchecked
  extern Name StateChecked;
  extern Name StateUnChecked;

  /// Focused/Unfocused
  extern Name StateFocused;
  extern Name StateUnFocused;

  /// Expanded/Collapsed
  extern Name StateExpanded;
  extern Name StateCollapsed;

  /// Selected/UnSelected
  extern Name StateSelected;
  extern Name StateUnSelected;
  extern Name StateSelectedUnFocused;

  /// Items / NoItems
  extern Name StateNoItems;
  extern Name StateSingleItem;
  extern Name StateMultipleItems;

  /// Empty / NotEmpty
  extern Name StateEmpty;
  extern Name StateNotEmpty;

  /// DragDropBefore / DragDropAfter / DragDropInto / DragDropNone
  extern Name StateDragDropBefore;
  extern Name StateDragDropAfter;
  extern Name StateDragDropInto;
  extern Name StateDragDropNone;

} // namespace UI
} // namespace noz


#endif //__noz_UI_UIStates_h__

