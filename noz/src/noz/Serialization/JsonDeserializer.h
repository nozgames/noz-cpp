///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonDeserializer_h__
#define __noz_JsonDeserializer_h__

#include "DeSerializer.h"
#include <noz/Text/Json/JsonReader.h>

namespace noz {

  class JsonDeserializer : public Deserializer {
    private: struct SerializedObject {
      /// File identifier of the object
      noz_uint32 id_;

      /// Pointer to actual serialized object
      ObjectPtr<Object> object_;

      /// Type of the object..
      Type* type_;

      /// Reader marker for the location of the object within the stream.
      noz_uint32 marker_;
    };

    /// Map of file identifier to serialized object
    private: std::map<noz_uint32,SerializedObject> objects_;

    /// Stack for processing objects for either read or write
    private: std::vector<SerializedObject*> queue_;

    /// Pointer to temporary reader used during Deserialize
    private: JsonReader* reader_;

    public: Object* Deserialize (Stream* stream, Object* root=nullptr);

    /// Deserialize an object from the given file identifier.  If the object has already
    /// been deserialized its fully deserialized pointer will be returned.  If the object
    /// has not yet been deserialized an allocated but not yet deserialized pointer will
    /// be returned and the object will be added to the queue for deserialization.
    public: Object* DeserializeObject (noz_uint32 id, Object* o=nullptr);


    private: noz_uint32 DeserializeObjectMap (JsonReader& reader);


    /// Deserialize the given serializable object
    private: bool Deserialize (JsonReader& reader, SerializedObject* so);
    
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

    public: virtual bool ReadValueObject (Object*& o,Type* type) override;

    public: virtual bool ReadValueGuid (Guid& guid) override;

    public: virtual void ReportError (const char* msg) override;

    public: virtual void ReportWarning (const char* msg) override;
  };


} // namespace noz


#endif //__noz_JsonDeserializer_h__

