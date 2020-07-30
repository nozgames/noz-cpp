///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_UIEventArgs_h__
#define __noz_UIEventArgs_h__


namespace noz {

  class Node;
  class UINode;

  class DragDeltaEventArgs {
    private: Vector2 delta_;

    public: DragDeltaEventArgs(Object* sender, const Vector2& delta) {
      delta_ = delta;
    }

    public: const Vector2& GetDelta (void) const {return delta_;}
  };


  typedef Event<UINode*> ClickEventHandler;
  typedef Event<UINode*> SelectionChangedEventHandler;
  typedef Event<UINode*> GainFocusEventHandler;
  typedef Event<UINode*> LostFocusEventHandler;
  typedef Event<UINode*> ScrollEventHandler;
  typedef Event<UINode*> DragStartedEventHandler;
  typedef Event<DragDeltaEventArgs*> DragDeltaEventHandler;
  typedef Event<UINode*> DragCompletedEventHandler;
  typedef Event<UINode*> ValueChangedEventHandler;
  typedef Event<Window*,UINode*> FocusChangedEventHandler;
  
 
} // namespace noz

#endif //__noz_UIEventArgs_h__

