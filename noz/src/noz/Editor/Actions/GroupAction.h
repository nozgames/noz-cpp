///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GroupAction_h__
#define __noz_Editor_GroupAction_h__

#include "Action.h"

namespace noz {
namespace Editor {

  class GroupAction : public Action {
    NOZ_OBJECT()

    private: std::vector<ObjectPtr<Action>> actions_;

    public: GroupAction (void) { }

    public: ~GroupAction (void) {
      for(noz_uint32 i=0,c=actions_.size();i<c;i++) delete actions_[i];
      actions_.clear();
    }

    public: noz_uint32 GetActionCount (void) const {return actions_.size();}

    public: Action* GetAction (noz_uint32 i) const {return actions_[i];}

    public: Action* GetLastAction (void) const {return actions_.back();}

    public: void AddAction (Action* action) {
      actions_.push_back(action);
    }

    /// Execute the action
    public: virtual void Do (void) final {
      for(noz_uint32 i=0,c=actions_.size();i<c;i++) actions_[i]->Do();
    }

    public: virtual void Undo (void) final {
      for(noz_uint32 i=actions_.size();i>0;i--) actions_[i-1]->Undo();
    }

    public: virtual bool CanMerge (Action* action) final {
      if(action->GetType() != GetType()) return false;
      GroupAction* ga = (GroupAction*)action;
      if(ga->actions_.size() != actions_.size()) return false;
      for(noz_uint32 i=0,c=actions_.size();i<c;i++) {
        if(!actions_[i]->CanMerge(ga->actions_[i])) return false;
      }
      return true;
    }

    public: virtual void Merge (Action* action) final {
      noz_assert(action->GetType()==GetType());
      noz_assert(((GroupAction*)action)->actions_.size() == actions_.size());

      GroupAction* ga = (GroupAction*)action;
      for(noz_uint32 i=0,c=actions_.size();i<c;i++) actions_[i]->Merge(ga->actions_[i]);
    }

  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GroupAction_h__
