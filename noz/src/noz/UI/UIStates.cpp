///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

namespace noz {
namespace UI {

  /// Common states
  Name StateNormal ("Normal");
  Name StateMouseOver ("Hover");
  Name StateDisabled ("Disabled");
  Name StatePressed ("Pressed");
  
  /// Checked/Unchecked
  Name StateChecked ("Checked");
  Name StateUnChecked ("UnChecked");

  /// Focused/Unfocused
  Name StateFocused("Focused");
  Name StateUnFocused("UnFocused");

  /// Expanded/Collapsed
  Name StateExpanded("Expanded");
  Name StateCollapsed("Collapsed");

  /// Selected/UnSelected
  Name StateSelected("Selected");
  Name StateUnSelected("UnSelected");
  Name StateSelectedUnFocused("SelectedUnFocused");

  /// Item states
  Name StateMultipleItems("MultipleItems");
  Name StateSingleItem("SingleItem");
  Name StateNoItems("NoItems");

  /// Empty/NotEmpty
  Name StateEmpty("Empty");
  Name StateNotEmpty("NotEmpty");

  Name StateDragDropBefore("DragDropBefore");
  Name StateDragDropAfter("DragDropAfter");
  Name StateDragDropInto("DragDropInto");
  Name StateDragDropNone("DragDropNone");
}
}