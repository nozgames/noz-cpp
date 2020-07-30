///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Int32_h__
#define __noz_Int32_h__

namespace noz {

  class Int32 : public Object {
    NOZ_OBJECT()

    private: noz_int32 value_;

    public: Int32 (noz_int32 value);
    public: Int32 (void);

    public: operator noz_int32 (void) const {return value_;}
    public: Int32& operator= (noz_int32 value) {value_=value; return *this;}

    public: virtual String ToString(void) const override;

    public: static noz_int32 Parse(String s);
    public: static noz_int32 Parse(const char* s);
  };        

} // namespace noz


#endif //__noz_Int32_h__

