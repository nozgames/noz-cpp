///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonSerializer_h__
#define __noz_JsonSerializer_h__

#include "Serializer.h"


namespace noz {

  class JsonWriter;

  class JsonSerializer : public Serializer {
    private: struct SerializedObject {
      /// File identifier of the object
      noz_uint32 id_;

      /// Pointer to actual serialized object
      ObjectPtr<Object> object_;

      /// Type of the object..
      Type* type_;
    };

    /// Pointer to current JsonWriter
    private: JsonWriter* writer_;

    /// Map of file identifier to serialized object
    private: std::map<noz_uint32,SerializedObject> objects_;

    /// Stack for processing objects for either read or write
    private: std::vector<SerializedObject*> queue_;

    private: bool Serialize (JsonWriter& writer, SerializedObject* so);

    /// Serialize the given object to the given stream
    public: virtual bool Serialize (Object* o, Stream* stream) override;

    public: virtual bool WriteStartObject (void) override;

    public: virtual bool WriteEndObject (void) override;

    public: virtual bool WriteStartSizedArray (noz_uint32 size) override;
    public: virtual bool WriteStartArray (void) override;

    public: virtual bool WriteEndArray (void) override;

    public: virtual bool WriteMember (const char* v) override;

    public: virtual bool WriteValueColor (Color v) override;

    public: virtual bool WriteValueName (const Name& v) override;
    public: virtual bool WriteValueString (const String& v) override;

    public: virtual bool WriteValueInt32 (noz_int32 v) override;

    public: virtual bool WriteValueUInt32 (noz_uint32 v) override;

    public: virtual bool WriteValueFloat (noz_float v) override;

    public: virtual bool WriteValueNull (void) override;

    public: virtual bool WriteValueBoolean (bool v) override;

    public: virtual bool WriteValueByte (noz_byte v) override;

    public: virtual bool WriteValueBytes (noz_byte* b, noz_uint32 size) override;

    public: virtual bool WriteValueObject (Object* o, bool inline_assets=false) override;

    public: virtual bool WriteValueGuid (const Guid& guid) override;
  };


} // namespace noz


#endif //__noz_JsonSerializer_h__

