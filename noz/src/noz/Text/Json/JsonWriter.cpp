///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/TextWriter.h>
#include "JsonWriter.h"

using namespace noz;

JsonWriter::JsonWriter(TextWriter* writer) {
  writer_ = writer;
  indentation_ = 1;
  indent_char_ = '\t';
  write_comma_=false; 
}

void JsonWriter::WriteIndentation(void) {
  if(indentation_>0) writer_->Write(indent_char_,indentation_ * depth_);
}

void JsonWriter::WriteStartArray(void) {  
  WriteComma();
  writer_->Write('[');
  depth_++;
  write_comma_=false;
}

void JsonWriter::WriteStartObject(void) {
  WriteComma();
  writer_->Write('{');
  depth_++;
  write_comma_ = false;
}

void JsonWriter::WriteEndArray(void) {
  writer_->Write(']');
  write_comma_ = true;
}

void JsonWriter::WriteEndObject(void) {
  writer_->Write('}');
  write_comma_ = true;
}

void JsonWriter::WriteMember(const char* name) {
  WriteComma();
  writer_->Write('\"');
  writer_->Write(name);
  writer_->Write('\"');
  writer_->Write(": ");
  write_comma_ = false;
}

void JsonWriter::WriteValueNull(void) {
  WriteComma();
  writer_->Write("null");
  write_comma_ = true;
}

void JsonWriter::WriteValueInt32(noz_int32 value) {
  WriteComma();
  writer_->Write(Int32(value).ToString());
  write_comma_ = true;
}

void JsonWriter::WriteValueBoolean(bool value) {
  WriteComma();
  writer_->Write(Boolean(value).ToString());
  write_comma_ = true;
}

void JsonWriter::WriteValueByte(noz_byte value) {
  WriteComma();
  writer_->Write(Byte(value).ToString());
  write_comma_ = true;
}

void JsonWriter::WriteValueUInt32(noz_uint32 value) {
  WriteComma();
  writer_->Write(UInt32(value).ToString());
  write_comma_ = true;
}

void JsonWriter::WriteValueFloat(noz_float value) {
  if(value==Float::NaN) {
    WriteValueString("NaN");
    return;
  }

  if(value==Float::Infinity) {
    WriteValueString("Infinity");
    return;
  }

  WriteComma();
  writer_->Write(Float(value).ToString());
  write_comma_ = true;
}

void JsonWriter::WriteValueString(const char* value) {
  WriteComma();
  writer_->Write('\"');

  for(const char* s=value; *s; s++) {
    switch(*s) {
      case '\\': writer_->Write("\\\\"); continue;
      case '\"': writer_->Write("\\\""); continue;
      case '/': writer_->Write("\\/"); continue;
      case '\b': writer_->Write("\\b"); continue;
      case '\f': writer_->Write("\\f"); continue;
      case '\n': writer_->Write("\\n"); continue;
      case '\r': writer_->Write("\\r"); continue;
      case '\t': writer_->Write("\\t"); continue;
      default: 
        writer_->Write(*s);
        break;
    }      
  }

  writer_->Write('\"');
  write_comma_ = true;
}

void JsonWriter::WriteComma(void) {
  if(write_comma_) writer_->Write(',');
}


