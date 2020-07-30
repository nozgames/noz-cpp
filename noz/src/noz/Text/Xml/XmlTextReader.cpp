///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "XmlTextReader.h"

using namespace noz;

#include <external/libxml2-2.7.2/include/libxml/xmlreader.h>


XmlTextReader::XmlTextReader(Stream* stream) {
  eof_ = false;
  stream_ = stream;

  buffer_ = xmlParserInputBufferCreateIO(InputReadCallback, nullptr,this, xmlCharEncoding::XML_CHAR_ENCODING_UTF8);

  if(buffer_ != nullptr) {
    reader_ = xmlNewTextReader((xmlParserInputBufferPtr)buffer_,"");
  }
}

XmlTextReader::~XmlTextReader(void) {
  if(reader_) xmlFreeTextReader((xmlTextReaderPtr)reader_);
  if(buffer_) xmlFreeParserInputBuffer((xmlParserInputBufferPtr)buffer_);
}

String XmlTextReader::GetName(void) const {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  noz_assert(reader);

  return (const char*)xmlTextReaderConstName(reader);
}

String XmlTextReader::GetValue(void) const {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  noz_assert(reader);

  return (const char*)xmlTextReaderValue(reader);
}

noz_int32 XmlTextReader::GetDepth(void) const {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  noz_assert(reader);
  return xmlTextReaderDepth(reader);
}

bool XmlTextReader::Read(void) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  if(1==xmlTextReaderRead(reader))  {
    return true;
  }    
  eof_ = true;
  return false;
}

bool XmlTextReader::ReadNextSibling(void) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  noz_assert(reader);
  return (1==xmlTextReaderNextSibling(reader));
}

int XmlTextReader::InputReadCallback (void * context, char * buffer, int len) {
  XmlTextReader* this_ = (XmlTextReader*)context;
  noz_assert(this_);

  return this_->stream_->Read(buffer,0,len);
}

String XmlTextReader::GetAttribute(const char* name) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  return (char*)xmlTextReaderGetAttribute(reader,(const xmlChar*)name);
}

String XmlTextReader::GetAttribute(noz_int32 index) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  return (char*)xmlTextReaderGetAttributeNo(reader,index);
}

bool XmlTextReader::MoveToElement (void) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  if(1!=xmlTextReaderMoveToElement(reader)) {
    return false;
  }
  return true;
}

bool XmlTextReader::MoveToContent (void) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  noz_assert(reader);

  int nt = xmlTextReaderNodeType(reader);
  while(nt==XML_READER_TYPE_SIGNIFICANT_WHITESPACE || nt==XML_READER_TYPE_WHITESPACE || nt==XML_READER_TYPE_PROCESSING_INSTRUCTION || nt==XML_READER_TYPE_DOCUMENT_TYPE || nt==XML_READER_TYPE_COMMENT) {
    if(!Read()) {
      return false;
    }
    nt = xmlTextReaderNodeType(reader);
  }
  return true;
}

bool XmlTextReader::MoveToFirstAttribute(void) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  if(1!=xmlTextReaderMoveToFirstAttribute(reader)) {
    return false;
  }
  return true;
}

bool XmlTextReader::MoveToNextAttribute(void) {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  if (1!=xmlTextReaderMoveToNextAttribute(reader)) {
    return false;
  }
  return true;
}

bool XmlTextReader::IsEmptyElement(void) const {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  return 1==xmlTextReaderIsEmptyElement(reader);
}


XmlNodeType XmlTextReader::GetNodeType(void) const {
  xmlTextReaderPtr reader = (xmlTextReaderPtr)reader_;
  noz_assert(reader);

  switch(xmlTextReaderNodeType(reader)) {
    case XML_READER_TYPE_NONE: return XmlNodeType::None;
    case XML_READER_TYPE_ELEMENT: return XmlNodeType::Element;
    case XML_READER_TYPE_ATTRIBUTE: return XmlNodeType::Attribute;
    case XML_READER_TYPE_TEXT: return XmlNodeType::Text;
    case XML_READER_TYPE_CDATA: return XmlNodeType::CDATA;
    case XML_READER_TYPE_ENTITY_REFERENCE: return XmlNodeType::EntityReference;
    case XML_READER_TYPE_ENTITY: return XmlNodeType::Entity;
    case XML_READER_TYPE_PROCESSING_INSTRUCTION: return XmlNodeType::ProcessingInstruction;
    case XML_READER_TYPE_COMMENT: return XmlNodeType::Comment;
    case XML_READER_TYPE_DOCUMENT: return XmlNodeType::Document;
    case XML_READER_TYPE_DOCUMENT_TYPE: return XmlNodeType::DocumentType;
    case XML_READER_TYPE_DOCUMENT_FRAGMENT: return XmlNodeType::DocumentFragment;
    case XML_READER_TYPE_NOTATION: return XmlNodeType::Notation;
    case XML_READER_TYPE_WHITESPACE: return XmlNodeType::Whitespace;
    case XML_READER_TYPE_SIGNIFICANT_WHITESPACE: return XmlNodeType::SignificantWhitespace;
    case XML_READER_TYPE_END_ELEMENT: return XmlNodeType::EndElement;
    case XML_READER_TYPE_END_ENTITY: return XmlNodeType::EndEntity;
    case XML_READER_TYPE_XML_DECLARATION: return XmlNodeType::XmlDeclaration;
  }

  return XmlNodeType::None;
}

