///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_EditorDocument_h__
#define __noz_Editor_EditorDocument_h__

#include <noz/Nodes/UI/Document.h>
#include "Nodes/UI/Inspector.h"
#include "Nodes/UI/Hierarchy.h"
#include "Actions/GroupAction.h"

namespace noz {
namespace Editor {

  class Memento;
  class Inspector;
  class Action;

  class EditorDocument : public Document {
    NOZ_OBJECT()

    protected: std::vector<Action*> undo_;
    protected: noz_int32 undo_action_;
    protected: GroupAction* current_group_action_;
    protected: bool undo_group_comitted_;

    protected: ObjectPtr<Inspector> inspector_;

    protected: ObjectPtr<Hierarchy> hierarchy_;

    protected: ObjectPtr<Animator> animation_view_source_;

    public: EditorDocument (void);

    public: ~EditorDocument (void);

    public: void Undo (void);

    public: void Redo (void);

    public: void CommitUndoGroup (void);

    public: void ExecuteAction (Action* action, bool commit_group=true);

    public: void Select (Node* n);

    public: void SetInspector (Inspector* inspector);

    public: void SetHierarchy (Hierarchy* hierarchy);

    public: void SetAnimationViewSource (Animator* animator);

    public: static EditorDocument* GetActiveDocument(Node* node);
    public: static EditorDocument* GetActiveDocument(Object* o);

    protected: virtual void OnActivate (void) override;
    protected: virtual void OnDeactivate (void) override;
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_EditorDocument_h__



/*

  ObjectLinks ?!?!




*/

