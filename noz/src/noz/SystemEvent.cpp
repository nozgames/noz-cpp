///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>
#include "SystemEvent.h"

using namespace noz;

std::vector<SystemEvent> SystemEvent::queue_;
noz_uint32 SystemEvent::queue_pop_ = 0;

SystemEvent::SystemEvent (SystemEventType etype, Window* window) {
  id_ = 0;
  handled_ = false;
  click_count_ = 0;
  button_ = MouseButton::Unknown;
  key_data_ = 0;
  event_type_ = etype;
  window_ = window;
}

SystemEvent::SystemEvent (SystemEventType etype, Window* window, noz_uint32 key_data) : SystemEvent(etype,window) {
  key_data_ = key_data;
}

SystemEvent::SystemEvent (SystemEventType etype, Window* window, const Vector2& position, const Vector2& delta, noz_uint32 modifiers) : SystemEvent(etype,window) {
  position_ = position;
  key_data_ = modifiers & Keys::ModifiersMask;
  delta_ = delta;
}

SystemEvent::SystemEvent (SystemEventType etype, Window* window, MouseButton button, noz_uint32 click_count, const Vector2& position, noz_uint32 modifiers) : SystemEvent(etype,window) {
  position_ = position;
  key_data_ = modifiers & Keys::ModifiersMask;
  button_ = button;
  click_count_ = click_count;
}

SystemEvent::SystemEvent (SystemEventType etype, Window* window, noz_uint64 id, const Vector2& position, noz_uint32 tap_count) : SystemEvent(etype,window) {
  position_ = position;
  click_count_ = tap_count;
  id_ = id;
}

SystemEvent::SystemEvent (SystemEventType etype, Window* window, Object* drag_object, const Vector2& position, noz_uint32 modifiers) : SystemEvent(etype,window) {
  position_ = position;
  key_data_ = modifiers & Keys::ModifiersMask;
  object_ = drag_object;  
}

void SystemEvent::PushEvent (const SystemEvent& e) {
  noz_assert(e.GetWindow());

  // Clear the queue if pushing a new event when the queue is empty
  if(queue_pop_ == queue_.size()) {
    queue_.clear();
    queue_pop_ = 0;
  } 

  queue_.push_back(e);
}

SystemEvent SystemEvent::PopEvent(void) {
  noz_assert(GetQueueCount()>0);

  // Increase the queue pop
  queue_pop_++;
  
  // Return a copy of the event
  return queue_[queue_pop_-1];
}

void SystemEvent::SetCursor (Cursor* cursor, bool force) {
  if(cursor_ && !force) return;
  if(cursor_ == cursor) return;
  cursor_ = cursor;
}

char SystemEvent::GetKeyChar(void) const {
  noz_uint32 code = GetKeyCode();
  bool shift = !!(GetModifiers() & Keys::Shift);

  if(code >= Keys::A && code <= Keys::Z) {
    if(shift) {
      return 'A' + (code-Keys::A);
    }
    return 'a' + (code-Keys::A);
  } else if (code >= Keys::D0 && code <= Keys::D9) {
    if(GetModifiers() & Keys::Shift) {
      switch(code) {
        case Keys::D0: return ')';
        case Keys::D1: return '!';
        case Keys::D2: return '@';
        case Keys::D3: return '#';
        case Keys::D4: return '$';
        case Keys::D5: return '%';
        case Keys::D6: return '^';
        case Keys::D7: return '&';
        case Keys::D8: return '*';
        case Keys::D9: return '(';
        default:
          break;
      }
    }

    return '0' + (code-Keys::D0);
  }

  switch(code) {
    case Keys::Space: return ' ';
    case Keys::OemComma: return ',';
    case Keys::OemPeriod: return '.';
    case Keys::OemMinus: return '-';
    case Keys::OemSemicolon: return shift ? ':' : ';';
  }

  return 0;
}
