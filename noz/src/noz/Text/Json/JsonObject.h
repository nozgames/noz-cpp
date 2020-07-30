///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonObject_h__
#define __noz_JsonObject_h__

#include "JsonValue.h"

namespace noz {

  class JsonString;
  class JsonArray;

  class JsonObject : public JsonValue {
    private: std::map<Name,JsonValue*> values_;

    public: JsonObject(void);

    public: ~JsonObject (void);

    public: void Clear (void);

    public: void Add(const Name& key, JsonValue* value);
    public: void Add(const Name& key, const char* value);
    public: void Add(const Name& key, const String& value);

    public: bool Contains (const Name& key) const {return values_.find(key) != values_.end();}

    public: void Remove(const Name& key);

    public: bool GetBoolean (const Name& key, bool default_value=false) const;

    public: noz_int32 GetInt32 (const Name& key, noz_int32 default_value=0) const;
    public: noz_float GetFloat (const Name& key, noz_float default_value=0) const;

    public: String GetString (const Name& key) const;

    public: void Set (const Name& key, JsonValue* v);

    public: void SetString (const Name& key, const String& v);
    public: void SetString (const Name& key, const char* v);
    public: void SetInt32 (const Name& key, noz_int32 v);
    public: void SetBoolean (const Name& key, bool v);

    public: JsonValue* Get (const Name& key) const;

    public: template <typename T> T* Get (const Name& key) const {return nullptr;}

    public: const std::map<Name,JsonValue*>& GetValues(void) const {return values_;}

    public: virtual noz_uint32 GetCount(void) const override {return values_.size();}

    public: virtual JsonValue* operator[] (const Name& key) const override {return Get(key);}

    public: virtual JsonValue* Clone (void) const override;

    public: virtual bool CloneInto (JsonValue* dst, bool overwrite) const override;
  };

  template <> inline JsonString* JsonObject::Get (const Name& key) const {
    JsonValue* v = Get(key);
    if(!v || !v->IsString()) return nullptr;
    return (JsonString*)v;
  }

  template <> inline JsonArray* JsonObject::Get (const Name& key) const {
    JsonValue* v = Get(key);
    if(!v || !v->IsArray()) return nullptr;
    return (JsonArray*)v;
  }

  template <> inline JsonObject* JsonObject::Get (const Name& key) const {
    JsonValue* v = Get(key);
    if(!v || !v->IsObject()) return nullptr;
    return (JsonObject*)v;
  }
    

} // namespace noz

#endif // __noz_JsonObject_h__
