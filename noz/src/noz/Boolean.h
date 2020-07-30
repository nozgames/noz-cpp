///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Boolean_h__
#define __noz_Boolean_h__

namespace noz {

  class Boolean : public Object {
    NOZ_OBJECT()

    private: bool value_;

    public: Boolean (bool value);
    public: Boolean (void);

    public: operator bool (void) const {return value_;}
    public: Boolean& operator= (bool value) {value_=value; return *this;}

    public: virtual String ToString(void) const override;

    public: static bool Parse(String s) {return Parse(s.ToCString()); }
    public: static bool Parse(const char* s);
  };

} // namespace noz


#endif //__noz_Int32_h__

