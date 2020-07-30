///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <string.h>
#include <ctype.h>

using namespace noz;

#if defined(NOZ_IOS) || defined(NOZ_OSX)
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

String String::Empty;

String::String(void) {
  length_ = 0;
  value_ = (char*)"";
}

String::String(const char* value) {
  if(nullptr==value || *value == 0) {
    length_ = 0;
    value_ = (char*)"";
    return;
  }
  length_ = (noz_int32)strlen(value);
  value_ = new char[length_+1];
  memcpy(value_,value,length_+1);  
}

String::String(const char* value,noz_int32 start, noz_int32 length) {
  length = length == -1 ? strlen(value+start): length;

  length_ = length;
  value_ = new char[length_+1];
  memcpy(value_,value+start,length);
  value_[length_] = 0;
}

String::String(Stream* stream, noz_int32 length) {
  value_ = new char[length+1];
  length_ = stream->Read(value_,0,length);
  value_[length_] = 0;
}

String::String(const String& value) {
  length_ = value.length_;
  value_ = new char[length_+1];
  memcpy(value_,value.value_,length_+1);  
}

String& String::operator= (const String& value) {
  length_ = value.length_;
  value_ = new char[length_+1];
  memcpy(value_,value.value_,length_+1);  
  return *this;
}

String::~String(void) {
  if(length_) {
    delete[] value_;
  }
}

noz_int32 String::CompareTo(const char* s, StringComparison comparison) const {
  switch(comparison) {
    case StringComparison::OrdinalIgnoreCase: return _stricmp(value_,s);
    default:
      break;
  }

  return strcmp(value_,s); 
}

noz_int32 String::CompareTo(const String& compare, StringComparison comparison) const {  
  switch(comparison) {
    case StringComparison::OrdinalIgnoreCase: return _stricmp(value_,compare.value_); 
    default:
      break;
  }

  // TODO: quick compare..
  return strcmp(value_,compare.value_); 
}

noz_int32 String::Compare(const String& compare1, noz_int32 index1, const String& compare2, noz_int32 index2, noz_int32 length) {
  return strncmp(compare1.value_+index1,compare2.value_+index2,length);
}


bool String::StartsWith (const String& s, StringComparison comparison) const {
  if(s.GetLength() > GetLength()) return false;
  if(IsEmpty()) return s.IsEmpty();
  switch(comparison) {
    case StringComparison::OrdinalIgnoreCase: return !_strnicmp(value_,s.value_,s.GetLength());
    default: break;
  }
  return !strncmp(value_,s.value_,s.GetLength());
}

noz_int32 String::IndexOf (char value, noz_int32 start) const {
  const char* r=strchr(value_+start,value);
  return (noz_int32)(r==nullptr?-1:r-value_);
}

noz_int32 String::IndexOf (const char* find, noz_int32 start, StringComparison comparison) const { 
  switch(comparison) {
    case StringComparison::Ordinal: {
      char* result = strstr(value_+start,find);
      if(result == nullptr) return -1;
      return result - (value_+start);
    }

    case StringComparison::OrdinalIgnoreCase: {
      for (const char* p=value_+start; *p; ++p ) {
        if (toupper(*p) == toupper(*find)) {
          const char* h;
          const char* n;
          for (h = p, n = find; *h && *n; ++h, ++n) {
            if ( toupper(*h) != toupper(*n) ) break;
          }
          if (!*n) return p-(value_+start);
        }
      }
      return -1;
    }
  }

  return -1;  
}

noz_int32 String::LastIndexOf (char value) const {
  const char* r=strrchr(value_,value);
  return (noz_int32)(r==nullptr?-1:r-value_);
}

String String::Substring(noz_int32 startIndex) const {
  return String(value_,startIndex,length_-startIndex);
}

String String::Substring(noz_int32 startIndex,noz_int32 length) const {
  return String(value_,startIndex,length);
}

String String::Trim (void) const {
  const char* s = value_;
  while(*s && (*s==' '||*s=='\n'||*s=='\t'||*s=='\r')) s++;
  if(!*s) return String::Empty;
  const char* e = value_ + length_ - 1;
  while(e > s && (*e==' '||*e=='\n'||*e=='\t'||*e=='\r')) e--;
  return Substring(s-value_, e-s+1);
}

String String::Format (const char* fmt, va_list args1, va_list args2) {
#ifdef NOZ_WINDOWS
  int len=_vscprintf(fmt,args1);
#else
  int len=vsnprintf(NULL,0,fmt,args1);
#endif

  String result;
  result.length_ = len;
  result.value_ = new char[len+1];

#ifdef NOZ_WINDOWS
  vsprintf_s ((char*)result.value_, len+1, fmt, args2);
#else
	vsprintf ((char*)result.value_, fmt, args2);
#endif	

  return result;
}


String String::Format (const char* fmt, ...) {
  va_list args1;
  va_list args2;
	va_start(args1,fmt);
  va_start(args2,fmt);

#ifdef NOZ_WINDOWS
  int len=_vscprintf(fmt,args1);
#else
  int len=vsnprintf(NULL,0,fmt,args1);
#endif

  String result;
  result.length_ = len;
  result.value_ = new char[len+1];

#ifdef NOZ_WINDOWS
  vsprintf_s ((char*)result.value_, len+1, fmt, args2);
#else
	vsprintf ((char*)result.value_, fmt, args2);
#endif	

  va_end(args1);
  va_end(args2);

  return result;
}

String String::Lower (void) const {
  String result = *this;
  for(char* c=result.value_; *c; c++) {
    if(*c >= 'A' && *c <= 'Z') *c = 'a' + (*c-'A');
  }
  return result;
}

String String::Upper (void) const {
  String result = *this;
  for(char* c=result.value_; *c; c++) {
    if(*c >= 'a' && *c <= 'z') *c = 'A' + (*c-'a');
  }
  return result;
}

noz_int32 String::GetLength (const char* s) {
  if(s==nullptr) return 0;
  return strlen(s);
}

noz_uint32 String::GetHashCode(const char* s) {
  // DJB2 Hash algorithm
  noz_uint32 hash = 5381;
  noz_int32 c;

  while ((c = *s++)) hash = ((hash << 5) + hash) + c;

  return hash;
}
