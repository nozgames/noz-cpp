///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Float_h__
#define __noz_Float_h__

namespace noz {

  class Float : public Object {
    NOZ_OBJECT(TypeCode=Float)

    public: static const noz_float NaN;
    public: static const noz_float PI;
    public: static const noz_float PI_180;
    public: static const noz_float PI_2;
    public: static const noz_float Infinity;
    public: static const noz_float M_180_PI;

    private: noz_float value_;

    public: Float (noz_float value);
    public: Float (void);

    public: virtual bool Equals (Object* o) const override;

    public: operator noz_float (void) const {return value_;}
    public: Float& operator= (noz_float value) {value_=value; return *this;}

    public: static noz_float Parse(String s);
    public: static noz_float Parse(const char* s);

    public: String ToString(void) const override;
  };

} // namespace noz


#endif //__noz_Float_h__

