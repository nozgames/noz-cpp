///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_Action_h__
#define __noz_Editor_Action_h__

namespace noz {
namespace Editor {

  class Action : public Object {
    NOZ_OBJECT()

    /// Create an action to undo this action
    public: virtual Action* CreateUndoAction (void) const {return nullptr;}

    /// Execute the actions
    public: virtual void Do (void) = 0;

    /// Undo the action
    public: virtual void Undo (void) = 0;

    /// Override and return true if the action can be merged together with the given action.
    public: virtual bool CanMerge (Action* action) const {return false;}

    /// Merge the given action into the same action
    public: virtual void Merge (Action* action) {noz_assert(false);}
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_Action_h__
