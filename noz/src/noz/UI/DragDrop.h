///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DragDrop_h__
#define __noz_DragDrop_h__

namespace noz {

  enum class DragDropEffects {
    None,
    Move,
    Copy,
    Link,
  };

  class Node;
  class UINode;
  class SystemEvent;
  class Window;

  enum DragDropEventType {
    Enter,
    Leave,
    Over,
    Drop
  };

  class DragDropEventArgs {
    private: DragDropEventType event_type_;
    private: ObjectPtr<Object> object_;
    private: Vector2 position_;
    private: DragDropEffects effects_;
    private: noz_uint32 modifiers_;
    public: DragDropEventArgs(DragDropEventType t, Object* o, const Vector2& position, noz_uint32 modifiers) : 
      event_type_(t), object_(o), position_(position), effects_(DragDropEffects::None), modifiers_(modifiers) {}
    public: const Vector2& GetPosition(void) const {return position_;}
    public: Object* GetObject (void) const {return object_;}
    public: template <typename T> T* GetObject (void) const {return Cast<T>(object_);}
    public: DragDropEffects GetEffects (void) const {return effects_;}
    public: void SetEffects (DragDropEffects effects) {effects_ = effects;}
    public: DragDropEventType GetEventType(void) const {return event_type_;}
    public: bool IsControl (void) const {return !!(Keys::Control & modifiers_);}
    public: bool IsAlt (void) const {return !!(Keys::Alt & modifiers_);}
    public: bool IsShift (void) const {return !!(Keys::Shift & modifiers_);}
  };

  typedef Event<UINode*,DragDropEventArgs*> DragDropEventHandler;

  class DragDrop {
    /// Current drag node
    private: static ObjectPtr<UINode> drag_node_;

    /// Current drag effects
    private: static DragDropEffects drag_effects_;

    /// Current drag modifiers.
    private: static noz_uint32 drag_modifiers_;

    /// True if currently within a call to DoDragDrop
    private: static bool drag_drop_active_;

    public: static DragDropEffects DoDragDrop (UINode* source, Object* data, DragDropEffects allowedEffects);

    public: static bool IsDragDropActive (void) {return drag_drop_active_;}

    public: static void HandleEvent (SystemEvent* e);

    public: static DragDropEffects GetCurrentEffects (void) {return drag_effects_;}

    private: static UINode* HitTest (Node* root, const Vector2& pos);

    private: static DragDropEffects DoDragDropImplementation (Window* source, Object* data, DragDropEffects allowedEffects);
  };

} // namespace noz


#endif //__noz_DragDrop_h__

