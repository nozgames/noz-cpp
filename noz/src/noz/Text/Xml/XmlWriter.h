///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Xml_XmlWriter_h__
#define __noz_Xml_XmlWriter_h__

namespace noz {

  class XmlWriter {
    private: Stream* stream_;
    private: TextWriter* writer_;
    private: char indent_char_;
    private: noz_int32 indentation_;
    private: std::vector<String> element_stack_;
    private: bool open_element_;

    public: XmlWriter(TextWriter* writer);

    public: ~XmlWriter(void);

    public: void WriteStartDocument (void);

    public: void WriteStartElement (const char* name);
    public: void WriteStartElement (const String& name) {WriteStartElement(name.ToCString());}

    public: void WriteEndElement (void);

    public: void WriteString (const char* value);
    public: void WriteString (const String& value) {WriteString(value.ToCString());}

    public: void WriteAttributeString (const char* name, const char* value);
    public: void WriteAttributeString (const String& name, const String& value) {WriteAttributeString(name.ToCString(), value.ToCString());}

    private: void WriteIndentation (void);
  };

} // namespace noz


#endif //__noz_Xml_XmlWriter_h__

