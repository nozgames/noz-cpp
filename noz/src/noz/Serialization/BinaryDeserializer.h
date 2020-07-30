///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_BinaryDeserializer_h__
#define __noz_BinaryDeserializer_h__

#include "DeSerializer.h"
#include "BinarySerializer.h"

namespace noz {

  class BinaryReader;

  class BinaryDeserializer : public Deserializer {
    private: struct SerializedObject {
      /// If the object is a managed asset this is the identifier of the asset
      Guid asset_guid_;

      /// Pointer to actual serialized object
      ObjectPtr<Object> object_;

      /// Type of the object..
      Type* type_;

      /// Offset of the object in the stream
      noz_uint32 offset_;
    };

    private: std::vector<SerializedObject> objects_;

    private: std::vector<SerializedObject*> queue_;

    private: std::vector<Name> names_;

    private: BinaryReader* reader_;

    private: String filename_;

    private: BinarySerializer::DataType peek_;

    private: noz_uint32 bytes_size_;

    private: bool error_;

    public: BinaryDeserializer (void);

    public: ~BinaryDeserializer (void);

    public: bool HasError(void) const {return error_;}

    /// Deserialize an object from the stream
    public: Object* Deserialize (Stream* stream, Object* root=nullptr, const char* filename=nullptr);

    public: virtual bool PeekEndObject (void) override;
    
    public: virtual bool PeekEndArray (void) override;

    public: virtual bool PeekValueNull(void) override;

    public: virtual bool PeekValueFloat (void) override;

    public: virtual bool ReadStartObject (void) override;

    public: virtual bool ReadEndObject (void) override;

    public: virtual bool ReadStartArray (void) override;

    public: virtual bool ReadStartSizedArray (noz_uint32& size) override;

    public: virtual bool ReadEndArray (void) override;

    public: virtual bool ReadMember (Name& v) override;

    public: virtual bool ReadValueString (String& v) override;

    public: virtual bool ReadValueName (Name& v) override;

    public: virtual bool ReadValueInt32 (noz_int32& v) override;

    public: virtual bool ReadValueUInt32 (noz_uint32& v) override;

    public: virtual bool ReadValueFloat (noz_float& v) override;

    public: virtual bool ReadValueNull (void) override;

    public: virtual bool ReadValueBoolean (bool& v) override;

    public: virtual bool ReadValueColor (Color& v) override;

    public: virtual bool ReadValueByte (noz_byte& v) override;

    public: virtual bool ReadValueBytesSize (noz_uint32& size) override;

    public: virtual bool ReadValueBytes (noz_byte* b) override;

    public: virtual bool ReadValueObject (Object*& o, Type* type) override;

    public: virtual bool ReadValueGuid (Guid& guid) override;

    private: BinarySerializer::DataType ReadDataType(void);

    private: BinarySerializer::DataType PeekDataType(void);

    private: const Name& ReadName (void);

    private: Object* ReadObject (Object* o);

    private: SerializedObject* GetSerializedObject(noz_uint32 id, Object* o);

    public: virtual void ReportError (const char* msg) override;

    public: virtual void ReportWarning (const char* msg) override;
  };


} // namespace noz


#endif //__noz_BinaryDeserializer_h__

