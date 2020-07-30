///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/TextWriter.h>
#include "XmlWriter.h"

using namespace noz;


XmlWriter::XmlWriter(TextWriter* writer) : writer_(writer) {
  indent_char_ = '\t';
  indentation_ = 1;
  open_element_= false;
}

XmlWriter::~XmlWriter(void) {
}

void XmlWriter::WriteStartDocument (void) {
  writer_->Write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
}

void XmlWriter::WriteStartElement (const char* name) {
  if(open_element_) {
    writer_->Write(">\n");
    open_element_= false;
  }

  WriteIndentation();
  writer_->Write("<");
  writer_->Write(name);

  element_stack_.push_back(name);

  open_element_ = true;
}

void XmlWriter::WriteAttributeString (const char* name, const char* value) {
  writer_->Write(' ');
  writer_->Write(name);
  writer_->Write("=\"");
  writer_->Write(value);
  writer_->Write('\"');
}

void XmlWriter::WriteString(const char* value) {
  if(open_element_) {
    writer_->Write(">");
    open_element_ = false;
  }

  writer_->Write(value);
}

void XmlWriter::WriteEndElement (void) {
  if(element_stack_.empty()) return;

  if(open_element_) {
    writer_->Write("/>\n");
    open_element_ = false;
  } else {
    WriteIndentation();
    writer_->Write("</");
    writer_->Write(element_stack_.back());
    writer_->Write(">\n");
    element_stack_.pop_back();
  }
}    

void XmlWriter::WriteIndentation(void) {
  if(indentation_) writer_->Write(indent_char_,indentation_*element_stack_.size());
}
