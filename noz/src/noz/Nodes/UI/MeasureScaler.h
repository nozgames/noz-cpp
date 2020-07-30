///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_MeasureScaler_h__
#define __noz_MeasureScaler_h__

namespace noz {

  class MeasureScaler : public UINode {
    NOZ_OBJECT()
 
    NOZ_PROPERTY(Name=Scale,Set=SetScale)
    private: noz_float scale_;

    public: MeasureScaler (void);

    public: void SetScale (noz_float scale);

    public: virtual Vector2 MeasureChildren (const Vector2& a) override;
  };

} // namespace noz


#endif //__noz_MeasureScale_h__


