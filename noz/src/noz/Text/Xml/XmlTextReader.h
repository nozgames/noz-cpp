///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Xml_XmlTextReader_h__
#define __noz_Xml_XmlTextReader_h__

namespace noz {

  enum class XmlNodeType {
    Attribute,
    CDATA,
    Comment,
    Document,
    DocumentFragment,
    DocumentType,
    Element,
    EndElement,
    EndEntity,
    Entity,
    EntityReference,
    None,
    Notation,
    ProcessingInstruction,
    SignificantWhitespace,
    Text,
    Whitespace,
    XmlDeclaration
  };

  class XmlTextReader : public Object {
    NOZ_OBJECT(NoAllocator)

    private: Stream* stream_;

    private: void* reader_;

    private: void* buffer_;

    private: bool eof_;

    public: XmlTextReader(Stream* stream);

    public: ~XmlTextReader(void);

    public: bool Read(void);

    public: bool ReadNextSibling(void);

    public: String GetName(void) const;

    public: String GetValue(void) const;

    public: String GetAttribute(const char* name);

    public: String GetAttribute(noz_int32 index);

    public: noz_int32 GetDepth(void) const;

    public: bool MoveToContent (void);

    public: bool MoveToElement (void);

    public: bool MoveToFirstAttribute (void);

    public: bool MoveToNextAttribute (void);

    public: bool IsEOF(void) {return eof_;}

    public: bool IsEmptyElement(void) const;

    public: bool IsElement (void) const {return GetNodeType()==XmlNodeType::Element;}

    public: bool IsEndElement (void) const {return GetNodeType()==XmlNodeType::EndElement;}

    public: XmlNodeType GetNodeType(void) const;

    private: static int InputReadCallback (void * context, char * buffer, int len);
  };

} // namespace noz


#endif //__noz_Xml_XmlTextReader_h__

