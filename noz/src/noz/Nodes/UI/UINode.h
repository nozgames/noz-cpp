///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_UINode_h__
#define __noz_UINode_h__

namespace noz {

  NOZ_ENUM() enum class TabNavigation {
    Contained,
    Continue,
    Cycle,
    None    
  };

  class UINode : public Node, public EventObserver {
    NOZ_OBJECT()

    public: LostFocusEventHandler LostFocus;
    public: GainFocusEventHandler GainFocus;
    public: DragDropEventHandler DragDrop;

    /// True if the control is interactive.
    NOZ_PROPERTY(Name=Interactive,Type=bool,Set=SetInteractive,Get=GetLocalInteractive)

    NOZ_PROPERTY(Name=Focusable,Type=bool,Set=SetFocusable,Get=IsFocusable)
    
    private: struct {
      /// Interactive state of the node
      noz_byte interactive_:1;

      /// Inherited interactive state (true if not a child of another UINode)
      noz_byte inherited_interactive_:1;

      /// True if the component is focusable
      noz_byte focusable_:1;

      /// True if the component has focus
      noz_byte focused_:1;

      /// True if the node is a drag drop target
      noz_byte drag_drop_target_:1;

      /// True if the node is a drag drop source
      noz_byte drag_drop_source_:1;

      /// True if the ui node requires a keyboard to function.
      noz_byte requires_keyboard_:1;
    };
    
    NOZ_PROPERTY(Name=Cursor,Set=SetCursor)
    private: ObjectPtr<Cursor> cursor_;

    NOZ_PROPERTY(Name=TabNavigation)
    private: TabNavigation tab_navigation_;

    public: UINode (void);

    public: Cursor* GetCursor (void) const {return cursor_;}

    private: bool GetLocalInteractive (void) const {return interactive_;}

    private: UINode* GetNextFocusable (void);

    public: bool HasFocus (void) const {return focused_;}

    public: bool IsDragDropTarget (void) const {return drag_drop_target_;}

    public: bool IsDragDropSource (void) const {return drag_drop_source_;}

    public: bool IsFocusable (void) const {return focusable_;}

    public: bool IsKeyboardRquired (void) const {return requires_keyboard_;}

    /// Returns true if the component is currently interactive.
    public: bool IsInteractive (void) const {return interactive_ && inherited_interactive_;}

    public: void SetCursor (Cursor* cursor);

    public: void SetFocus (void);

    public: void SetFocusable (bool v=true);

    public: void SetInteractive (bool interactive);

    public: void SetTabNavigation (TabNavigation tab);

    public: void SetDragDropTarget (bool v=true);

    public: void SetDragDropSource (bool v=true);

    public: void SetRequiresKeyboard (bool v=true);

    public: virtual void OnGainFocus (void);

    protected: virtual void OnInteractiveChanged (void);

    protected: virtual void OnLineageChanged (void) override;

    public: virtual void OnDragDrop (DragDropEventArgs* args);

    public: virtual void OnLoseFocus (void);

    protected: virtual void DoDragDrop (void) {noz_assert(false);}

    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseOver (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;

    private: void PropagateInteractiveChange (Node* node, bool inherited);
  };

} // namespace noz


#endif // __noz_UINode_h__

