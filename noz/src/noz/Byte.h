///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Byte_h__
#define __noz_Byte_h__

namespace noz {

  class Byte : public Object {
    NOZ_OBJECT()

    private: noz_byte value_;

    public: Byte (noz_byte value);
    public: Byte (void);

    public: operator noz_byte (void) const {return value_;}
    public: Byte& operator= (noz_byte value) {value_=value; return *this;}

    public: virtual String ToString(void) const override;

    public: static Byte Parse (const char* s);
    public: static Byte Parse (const String& s) {return Parse(s.ToCString());}
  };



} // namespace noz


#endif //__noz_Int32_h__

