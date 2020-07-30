///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_StringObject_h__
#define __noz_StringObject_h__

namespace noz {

  class StringObject : public Object {
    NOZ_OBJECT(TypeCode=String,NoAllocator)

    NOZ_PROPERTY(Name=Value)
    protected: String value_;

    public: StringObject(void) {}

    public: StringObject(const String& value) {value_ = value;}

    public: const String& operator *(void) {return value_;}

    public: const String& GetValue (void) const {return value_;}

    public: virtual String ToString(void) const override {return value_;}
  };

}

#include "StringBuilder.h"

#endif // __noz_StringObject_h__
