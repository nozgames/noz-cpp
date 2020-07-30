///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_UInt32_h__
#define __noz_UInt32_h__

namespace noz {

  class UInt32 : public Object {
    NOZ_OBJECT()

    private: noz_uint32 value_;

    public: UInt32 (noz_uint32 value);
    public: UInt32 (void);

    public: operator noz_uint32 (void) const {return value_;}
    public: UInt32& operator= (noz_uint32 value) {value_=value; return *this;}

    public: virtual String ToString(void) const override;

    public: static noz_uint32 Parse(String s);
    public: static noz_uint32 Parse(const char* s);
  };        

} // namespace noz


#endif //__noz_UInt32_h__

