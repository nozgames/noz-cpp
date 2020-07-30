///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Spacer_h__
#define __noz_Spacer_h__

namespace noz {

  class Spacer : public UINode {
    NOZ_OBJECT(EditorIcon="{E809FE38-FBA7-48F6-973F-B6561B91DF2A}")
 
    NOZ_PROPERTY(Name=Width,Set=SetWidth)
    private: noz_float width_;

    NOZ_PROPERTY(Name=Height,Set=SetHeight)
    private: noz_float height_;

    NOZ_PROPERTY(Name=Count,Set=SetCount)
    private: noz_int32 count_;

    public: Spacer (void);

    public: void SetWidth(noz_float width);
    public: void SetHeight(noz_float height);
    public: void SetCount(noz_int32 count);

    public: virtual Vector2 MeasureChildren (const Vector2& a) override;
  };

} // namespace noz


#endif //__noz_Spacer_h__


