///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Serializer_h__
#define __noz_Serializer_h__

namespace noz {

  struct Color;

  class Serializer {
    public: virtual bool Serialize (Object* o, Stream* stream) = 0;

    public: virtual bool WriteStartObject (void) = 0;

    public: virtual bool WriteEndObject (void) = 0;

    public: virtual bool WriteStartSizedArray (noz_uint32 size) = 0;
    public: virtual bool WriteStartArray (void) = 0;

    public: virtual bool WriteEndArray (void) = 0;

    public: virtual bool WriteMember (const char* name) = 0;
    public: bool WriteMember (const String& name) {return WriteMember(name.ToCString());}
    public: bool WriteMember (const Name& name) {return WriteMember(name.ToCString());}

    public: virtual bool WriteValueString (const String& v) = 0;

    public: virtual bool WriteValueName (const Name& v) = 0;

    public: virtual bool WriteValueInt32 (noz_int32 v) = 0;

    public: virtual bool WriteValueUInt32 (noz_uint32 v) = 0;

    public: virtual bool WriteValueFloat (noz_float v) = 0;

    public: virtual bool WriteValueNull (void) = 0;

    public: virtual bool WriteValueBoolean (bool v) = 0;

    public: virtual bool WriteValueColor (Color v) = 0;

    public: virtual bool WriteValueByte (noz_byte v) = 0;

    public: virtual bool WriteValueBytes (noz_byte* b, noz_uint32 size) = 0;

    public: virtual bool WriteValueObject (Object* o, bool inline_assets=false) = 0;

    public: virtual bool WriteValueGuid (const Guid& guid) = 0;
  };


} // namespace noz


#endif //__noz_Serializer_h__



