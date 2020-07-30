///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonArray_h__
#define __noz_JsonArray_h__

#include "JsonValue.h"

namespace noz {

  class JsonArray : public JsonValue {
    private: std::vector<JsonValue*> values_;
    
    public: JsonArray (void);

    public: ~JsonArray (void);

    public: void Clear (void);

    public: void Add (JsonValue* value);

    public: const std::vector<JsonValue*>& GetValues(void) const {return values_;}

    public: virtual noz_uint32 GetCount(void) const override {return values_.size();}

    public: virtual JsonValue* operator[] (noz_int32 index) const override {return values_[index];} 

    public: virtual JsonValue* Clone (void) const override;

    public: virtual bool CloneInto (JsonValue* dst, bool overwrite) const override;
  };

} // namespace noz

#endif // __noz_JsonArray_h__
