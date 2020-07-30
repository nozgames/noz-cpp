///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SystemEvent_h__
#define __noz_SystemEvent_h__

#include "UI/Keys.h"

namespace noz {

  class Cursor;
  class Window;
  class BinaryReader;
  class BinaryWriter;

  enum class MouseButton {
    Unknown = -1,
    Left,
    Right,
    Middle
  };

  enum class SystemEventType {
    CaptureChanged,
    KeyUp,
    KeyDown,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseWheel,
    DragEnter,
    DragOver,
    DragLeave,
    Drop,
    TouchBegan,
    TouchMoved,
    TouchEnded,
    TouchCancelled
  };

  class SystemEvent {
    protected: SystemEventType event_type_;

    /// Window the event was raised on
    protected: ObjectPtr<Window> window_;

    /// True if the event has been handle
    protected: bool handled_;

    /// Mouse button for MouseUp and MouseDown events
    protected: MouseButton button_;

    /// Number of consectutive clicks that were received
    protected: noz_uint32 click_count_;

    /// KeyCode and Modifiers time of event generation.
    protected: noz_uint32 key_data_;

    /// Position of event in Window coordinates
    protected: Vector2 position_;

    /// Delta position of event in Window coordinates
    protected: Vector2 delta_;

    protected: Vector2 drag_delta_;

    protected: ObjectPtr<Cursor> cursor_;

    protected: ObjectPtr<Object> object_;

    protected: noz_uint64 id_;

    /// Circular queue
    private: static std::vector<SystemEvent> queue_;

    /// Index of next item to be popped from queue.
    private: static noz_uint32 queue_pop_;

    public: SystemEvent (SystemEventType etype, Window* window);

    public: SystemEvent (SystemEventType etype, Window* window, noz_uint32 key_data);

    public: SystemEvent (SystemEventType etype, Window* window, const Vector2& position, const Vector2& delta, noz_uint32 modifiers);

    public: SystemEvent (SystemEventType etype, Window* window, MouseButton button, noz_uint32 click_count, const Vector2& position, noz_uint32 modifiers);

    public: SystemEvent (SystemEventType etype, Window* window, noz_uint64 id, const Vector2& position, noz_uint32 tap_count);

    public: SystemEvent (SystemEventType etype, Window* window, Object* object, const Vector2& position, noz_uint32 modifiers);

    public: MouseButton GetButton (void) const {return button_;}

    public: noz_uint32 GetClickCount (void) const {return click_count_;}

    public: Cursor* GetCursor (void) const {return cursor_;}

    public: const Vector2& GetDelta (void) const {return delta_;}

    public: SystemEventType GetEventType (void) const {return event_type_;}

    public: char GetKeyChar(void) const;

    public: noz_uint32 GetKeyCode (void) const {return key_data_ & Keys::KeyCodeMask;}

    public: noz_uint32 GetModifiers(void) const {return key_data_ & Keys::ModifiersMask;}

    public: Object* GetObject (void) const {return object_;}

    public: noz_uint64 GetId (void) const {return id_;}

    public: template <typename T> T* GetObject (void) const {return Cast<T>(object_);}

    public: const Vector2& GetPosition(void) const {return position_;}

    public: Window* GetWindow (void) const {return window_;}

    public: bool IsAlt (void) const {return !!(key_data_ & Keys::Alt);}
    public: bool IsControl (void) const {return !!(key_data_ & Keys::Control);}
    public: bool IsHandled (void) const {return handled_;}
    public: bool IsShift (void) const {return !!(key_data_ & Keys::Shift);}

    public: static void PushEvent (SystemEventType etype, Window* window) {PushEvent(SystemEvent(etype,window));}
    public: static void PushEvent (SystemEventType etype, Window* window, const Vector2& position, const Vector2& delta, noz_uint32 modifiers) {PushEvent(SystemEvent(etype,window,position,delta,modifiers));} 
    public: static void PushEvent (SystemEventType etype, Window* window, MouseButton button, noz_uint32 click_count, const Vector2& position, noz_uint32 modifiers) {PushEvent(SystemEvent(etype,window,button,click_count,position,modifiers));}
    public: static void PushEvent (SystemEventType etype, Window* window, noz_uint64 id, const Vector2& position, noz_uint32 tap_count) {PushEvent(SystemEvent(etype,window,id,position,tap_count));}
    public: static void PushEvent (SystemEventType etype, Window* window, noz_uint32 key_data) {PushEvent(SystemEvent(etype,window,key_data));}
    public: static void PushEvent (SystemEventType etype, Window* window, Object* object, const Vector2& position, noz_uint32 key_data) {PushEvent(SystemEvent(etype,window,object,position,key_data));}

    public: static noz_uint32 GetQueueCount (void) {return queue_.size()-queue_pop_;}

    public: static SystemEvent PopEvent (void);

    public: void SetCursor (Cursor* cursor, bool force=false);

    public: void SetHandled (void) {handled_ = true;}

    private: static void PushEvent (const SystemEvent& e);
  };
  
} // namespace noz

#endif // __noz_SystemEvent_h__
