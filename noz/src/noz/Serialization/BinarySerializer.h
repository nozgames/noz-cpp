///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_BinarySerializer_h__
#define __noz_BinarySerializer_h__

#include "Serializer.h"

namespace noz {

  class BinaryWriter;

  class BinarySerializer : public Serializer {
    friend class BinaryDeserializer;

    private: struct Header {
      noz_uint32 fourcc_;
      noz_uint32 version_;
      noz_uint32 name_table_count_;
      noz_uint32 name_table_offset_;
      noz_uint32 object_table_offset_;
      noz_uint32 object_table_count_;
      noz_uint32 root_;
    };

    private: struct SerializedObject {
      /// File identifier of the object
      noz_uint32 id_;

      /// If the object is a managed asset this is the guid of the asset
      Guid asset_guid_;

      /// Pointer to actual serialized object
      ObjectPtr<Object> object_;

      /// Type of the object..
      Type* type_;

      /// Offset of the object in the stream
      noz_uint32 offset_;
    };

    private: enum class DataType : noz_byte {
      Unknown,
      EndArray,
      EndObject,
      Member,
      StartArray,
      StartObject,
      StartSizedArray,
      ValueByte,
      ValueBytes,
      ValueBytesSize,
      ValueBoolean,
      ValueColor,
      ValueFloat,
      ValueInt32,
      ValueName,
      ValueNull,
      ValueObject,
      ValueString,
      ValueUInt32,
      ValueGuid,
    };

    private: static const noz_uint32 FourCC = 0x425A4F4E; // NOZB

    private: std::vector<SerializedObject*> objects_;

    /// Map of file identifier to serialized object
    private: std::map<noz_uint32,SerializedObject> object_map_;

    private: std::vector<Name> names_;

    private: std::map<Name,noz_uint32> name_map_;

    private: MemoryStream temp_stream_;

    private: BinaryWriter* writer_;

    private: noz_uint32 header_position_;

    public: BinarySerializer(void);

    /// Serialize the given object to the given stream
    public: virtual bool Serialize (Object* o, Stream* stream) override;
   
    private: bool Serialize (Stream* stream, SerializedObject* so);

    public: virtual bool WriteStartObject (void) override;

    public: virtual bool WriteEndObject (void) override;

    public: virtual bool WriteStartSizedArray (noz_uint32 size) override;

    public: virtual bool WriteStartArray (void) override;

    public: virtual bool WriteEndArray (void) override;

    public: virtual bool WriteMember (const char* name) override;

    public: virtual bool WriteValueString (const String& v) override;

    public: virtual bool WriteValueName (const Name& v) override;

    public: virtual bool WriteValueInt32 (noz_int32 v) override;

    public: virtual bool WriteValueUInt32 (noz_uint32 v) override;

    public: virtual bool WriteValueFloat (noz_float v) override;

    public: virtual bool WriteValueNull (void) override;

    public: virtual bool WriteValueBoolean (bool v) override;

    public: virtual bool WriteValueColor (Color v) override;

    public: virtual bool WriteValueByte (noz_byte v) override;

    public: virtual bool WriteValueBytes (noz_byte* b, noz_uint32 size) override;

    public: virtual bool WriteValueObject (Object* o, bool inline_assets=false) override;

    public: virtual bool WriteValueGuid (const Guid& guid) override;

    private: void WriteDataType(DataType vt);

    private: noz_uint32 WriteObject (Object* o);
    
    private: noz_uint32 WriteName (const Name& name);
  };


} // namespace noz


#endif //__noz_BinarySerializer_h__

