///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Thumb_h__
#define __noz_Thumb_h__

namespace noz {


  class Thumb : public Control {
    NOZ_OBJECT(DefaultStyle="{177814C8-161B-43E9-8E4F-20C56985ACEF}")

    private: Vector2 drag_start_;

    public: DragStartedEventHandler DragStarted;;
    public: DragDeltaEventHandler DragDelta;
    public: DragCompletedEventHandler DragCompleted;

    public: virtual void OnMouseDown(SystemEvent* e) override;
    public: virtual void OnMouseUp(SystemEvent* e) override;
    public: virtual void OnMouseOver(SystemEvent* e) override;

    /// Default ScrollView constructor
    public: Thumb (void);

    protected: virtual void UpdateAnimationState (void) override;
  };

} // namespace noz


#endif //__noz_Thumb_h__

