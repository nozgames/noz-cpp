///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonPrimative_h__
#define __noz_JsonPrimative_h__

#include "JsonValue.h"

namespace noz {

  class JsonString : public JsonValue {
    private: String value_;

    public: JsonString(void);
    public: JsonString(const String& value);
    
    public: void Set(const String& value) {value_ = value;}
    public: void Set(const char* value) {value_ = value;}

    public: const String& Get (void) const {return value_;}

    public: virtual JsonValue* Clone (void) const override;

    public: virtual bool CloneInto (JsonValue* dst, bool overwrite) const override;
  };

} // namespace noz

#endif // __noz_JsonPrimative_h__
