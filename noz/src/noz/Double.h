///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Double_h__
#define __noz_Double_h__

namespace noz {

  class Double : public Object {
    NOZ_OBJECT(TypeCode=Double)

    public: static const noz_double NaN;

    private: noz_double value_;

    public: Double (noz_double value);
    public: Double (void);

    public: operator noz_double (void) const {return value_;}
    public: Double& operator= (noz_double value) {value_=value; return *this;}

    public: virtual String ToString(void) const override;
  };

} // namespace noz


#endif //__noz_Double_h__

