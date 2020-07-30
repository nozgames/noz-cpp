///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Deserializer_h__
#define __noz_Deserializer_h__

namespace noz {

  class Deserializer {
    public: virtual bool PeekEndObject (void) = 0;
    
    public: virtual bool PeekEndArray (void) = 0;

    public: virtual bool PeekValueNull(void) = 0;

    public: virtual bool PeekValueFloat (void) = 0;

    public: virtual bool ReadStartObject (void) = 0;

    public: virtual bool ReadEndObject (void) = 0;

    public: virtual bool ReadStartArray (void) = 0;

    public: virtual bool ReadStartSizedArray (noz_uint32& size) = 0;

    public: virtual bool ReadEndArray (void) = 0;

    public: virtual bool ReadMember (Name& v) = 0;

    public: virtual bool ReadValueString (String& v) = 0;

    public: virtual bool ReadValueName (Name& v) = 0;

    public: virtual bool ReadValueInt32 (noz_int32& v) = 0;

    public: virtual bool ReadValueUInt32 (noz_uint32& v) = 0;

    public: virtual bool ReadValueFloat (noz_float& v) = 0;

    public: virtual bool ReadValueNull (void) = 0;

    public: virtual bool ReadValueBoolean (bool& v) = 0;

    public: virtual bool ReadValueColor (Color& v) = 0;

    public: virtual bool ReadValueByte (noz_byte& v) = 0;

    public: virtual bool ReadValueBytesSize (noz_uint32& size) = 0;

    public: virtual bool ReadValueBytes (noz_byte* b) = 0;

    public: virtual bool ReadValueObject (Object*& o, Type* type) = 0;
    
    public: virtual bool ReadValueGuid (Guid& guid) = 0;

    public: virtual void ReportError (const char* msg) = 0;

    public: virtual void ReportWarning (const char* msg) = 0;
  };


} // namespace noz


#endif //__noz_Deserializer_h__

