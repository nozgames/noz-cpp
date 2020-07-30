///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Transform_h__
#define __noz_Transform_h__

namespace noz {

  class Transform : public Component {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Position,Set=SetLocalPosition)
    protected: Vector2 local_position_;

    NOZ_PROPERTY(Name=Scale,Set=SetScale)
    protected: Vector2 local_scale_;

    /// Normalized location within local transform to pivot around
    NOZ_PROPERTY(Name=Pivot,Set=SetPivot)
    protected: Vector2 pivot_;

    NOZ_PROPERTY(Name=Rotation,Set=SetRotation)
    protected: noz_float local_rotation_;

    public: Transform (void);

    public: virtual Rect Update (const Rect& arrange_rect, const Vector2& measured_size);

    public: virtual Rect AdjustArrangeRect(const Rect& r) const {return r;}

    public: virtual Vector2 AdjustAvailableSize (const Vector2& available_size) {return available_size;}

    public: virtual Vector2 AdjustMeasuredSize (const Vector2& measured_size) {return measured_size;}

    public: virtual bool IsDependentOnMeasure (void) const {return true;}

    public: virtual void Apply (Matrix3& local_to_world);

    public: const Vector2& GetPivot(void) const {return pivot_;}

    public: const Vector2& GetLocalPosition(void) const {return local_position_;}

    public: const Vector2& GetScale(void) const {return local_scale_;}

    public: void SetPivot(const Vector2& pivot);

    public: void SetLocalPosition(const Vector2& position);

    public: void SetScale(const Vector2& scale);

    /// Set the rotation of the node in degrees
    public: void SetRotation (noz_float rotation);
  };

} // namespace noz


#endif //__noz_Transform_h__


