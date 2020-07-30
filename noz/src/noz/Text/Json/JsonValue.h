///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonValue_h__
#define __noz_JsonValue_h__

#include <noz/IO/TextReader.h>

namespace noz {

  enum class JsonType {
    String,
    Object,
    Array,
  };

  class JsonValue {
    /// Value type
    private: JsonType json_type_;

    /// Line number the value was parsed from
    private: noz_uint32 line_number_;

    /// Column number the value was parsed from
    private: noz_uint32 column_number_;

    protected: JsonValue (JsonType vt);

    public: static JsonValue* Load (TextReader& reader);

    public: static JsonValue* Parse (const char* s);
    public: static JsonValue* Parse (const String& s);

    public: bool IsObject (void) const {return json_type_==JsonType::Object;}
    public: bool IsArray (void) const {return json_type_==JsonType::Array;}
    public: bool IsString (void) const {return json_type_==JsonType::String;}

    public: JsonType GetJsonType(void) const {return json_type_;}

    public: virtual bool IsEmpty(void) const {return GetCount()==0;}
    public: virtual noz_uint32 GetCount(void) const {return 0;}

    public: virtual JsonValue* operator[] (noz_int32 index) const {return nullptr;} 
    public: virtual JsonValue* operator[] (const Name& key) const {return nullptr;}

    public: virtual operator String (void) const {return String::Empty;}

    public: virtual JsonValue* Clone (void) const {return nullptr;}

    public: String ToString (void) const;

    /// Clone the value into the destination value if compatible
    public: virtual bool CloneInto (JsonValue* dst, bool overwrite) const {return false;}

    private: static void ToString (const JsonValue* v, int depth, StringBuilder& out);
  };

} // namespace noz

#endif // __noz_JsonValue_h__
