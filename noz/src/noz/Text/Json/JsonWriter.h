///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonWriter_h__
#define __noz_JsonWriter_h__

namespace noz {

  class TextWriter;

  class JsonWriter {
    private: TextWriter* writer_;

    private: noz_uint32 depth_;

    private: bool write_comma_;

    private: char indent_char_;

    private: noz_int32 indentation_;

    /// Create a json writer from the given text writer. Note the JsonWriter
    /// will not take ownership of the writer.
    public: JsonWriter (TextWriter* writer);

    public: void WriteStartArray (void);

    public: void WriteStartObject (void);

    public: void WriteEndArray (void);

    public: void WriteEndObject (void);

    public: void WriteMember (const char* name);
    public: void WriteMember (const String& name) {WriteMember(name.ToCString());}

    public: void WriteValueString (const char* name);
    public: void WriteValueString (const String& name) {WriteValueString(name.ToCString());}

    public: void WriteValueInt32 (noz_int32 value);
    public: void WriteValueUInt32 (noz_uint32 value);
    public: void WriteValueFloat (noz_float value);
    public: void WriteValueBoolean (bool value);
    public: void WriteValueByte (noz_byte value);
    public: void WriteValueNull (void);

    private: void WriteComma (void);
    private: void WriteIndentation (void);

  };

} // namespace noz

#endif // __noz_JsonWriter_h__
