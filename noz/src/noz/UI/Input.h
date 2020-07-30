///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Input_h__
#define __noz_Input_h__

namespace noz {

  class Input;

  NOZ_ENUM() enum class InputAxis {
    MouseWheel,
  };

  static const noz_uint32 InputAxisCount = 1;

  enum class TouchPhase {
    Began,
    Moved,
    Ended,
    Cancelled
  };

  class Touch {
    friend class Input;

    /// Unique identifier of the touch
    private: noz_uint64 id_;

    /// Screen position of the touch
    private: Vector2 position_;

    /// Delta position from last frame.
    private: Vector2 delta_position_;

    /// Tap count
    private: noz_uint32 tap_count_;

    /// Touch phase.
    private: TouchPhase phase_;

    private: noz_float timestamp_;

    public: noz_uint64 GetId (void) const {return id_;}

    public: const Vector2& GetPosition (void) const {return position_;}

    public: const Vector2& GetDeltaPosition (void) const {return delta_position_;}
    
    public: noz_uint32 GetTapCount (void) const {return tap_count_;}

    public: TouchPhase GetPhase (void) const {return phase_;}

    public: noz_float GetTimestamp (void) const {return timestamp_;}
  };

  class Input {
    private: static const noz_byte ButtonPressed = NOZ_BIT(0);
    private: static const noz_byte ButtonDown    = NOZ_BIT(1);
    private: static const noz_byte ButtonUp      = NOZ_BIT(2);
    private: static const noz_byte ButtonDrag    = NOZ_BIT(3);

    private: static Input* this_;

    private: noz_byte keys_[KeyCodeSize];

    private: noz_float axis_[InputAxisCount];

    private: Vector2 mouse_position_;

    private: Vector2 mouse_delta_;

    private: Vector2 mouse_drag_origin_[3];

    private: Vector2 mouse_drag_delta_[3];

    private: noz_float mouse_button_time_[3];

    private: std::vector<Touch> touches_;

    private: ObjectPtr<Window> mouse_window_;

    private: noz_byte mouse_button_[3];

    private: noz_uint64 mouse_touch_id_;

    public: static void Initialize(void);

    public: static void Uninitialize(void);

    private: Input (void);

    public: static void Update (void);

    /// Returns true if the key is pressed
    public: static bool GetKey (KeyCode key) {
      return !!(this_->keys_[key] & ButtonPressed);
    }

    /// Returns true if the key was pressed this frame
    public: static bool GetKeyDown (KeyCode key) {
      return !!(this_->keys_[key] & ButtonDown);
    }

    /// Returns true if the key was released this frame
    public: static bool GetKeyUp (KeyCode key) {
      return !!(this_->keys_[key] & ButtonUp);
    }

    public: static bool GetMouseButton (MouseButton button) {
      noz_int32 i = (noz_int32)button;
      noz_assert(i>=0&&i<=2);
      return !!(this_->mouse_button_[i] & ButtonPressed);
    }

    public: static bool GetMouseButtonDown (MouseButton button) {
      noz_int32 i = (noz_int32)button;
      noz_assert(i>=0&&i<=2);
      return !!(this_->mouse_button_[i] & ButtonDown);
    }

    public: static bool GetMouseButtonUp (MouseButton button) {
      noz_int32 i = (noz_int32)button;
      noz_assert(i>=0&&i<=2);
      return !!(this_->mouse_button_[i] & ButtonUp);
    }

    public: static bool GetMouseButtonDrag (MouseButton button) {
      noz_int32 i = (noz_int32)button;
      noz_assert(i>=0&&i<=2);
      return !!(this_->mouse_button_[i] & ButtonDrag);
    }

    public: static const Vector2& GetMouseButtonDragDelta (MouseButton button) {
      noz_int32 i = (noz_int32)button;
      noz_assert(i>=0&&i<=2);
      return this_->mouse_drag_delta_[i];
    }

    public: static noz_float GetMouseButtonTime (MouseButton btn) {return this_->mouse_button_time_[(noz_int32)btn];}

    public: static const Vector2& GetMousePosition (void) {return this_->mouse_position_;}

    public: static const Vector2& GetMouseDelta (void) {return this_->mouse_delta_;}

    public: static Window* GetMouseWindow (void) {return this_->mouse_window_;}

    public: static noz_uint32 GetTouchCount (void) {return this_->touches_.size();}

    public: static const Touch& GetTouch (noz_uint32 i) {return this_->touches_[i];}

    public: static void HandleEvent (SystemEvent* e);


    private: void HandleMouseDown (SystemEvent* e);
    private: void HandleMouseUp (SystemEvent* e);
    private: void HandleMouseMove (SystemEvent* e);
    private: void HandleMouseWheel (SystemEvent* e);

    private: void HandleTouchBegan (SystemEvent* e);
    private: void HandleTouchEnded (SystemEvent* e);
    private: void HandleTouchMoved (SystemEvent* e);
    private: void HandleTouchCancelled (SystemEvent* e);

    private: void UpdateMousePosition (SystemEvent* e);
    
    private: Touch* FindTouch (noz_uint64 id);

  };

} // namespace noz


#endif //__noz_Input_h__

