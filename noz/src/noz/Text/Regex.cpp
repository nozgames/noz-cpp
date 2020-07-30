///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Regex.h"

#include <regex>

#include <external/libxml2-2.7.2/include/libxml/xmlregexp.h>

using namespace noz;


Regex::Regex(void) {
  handle_ = nullptr;
}

Regex::Regex(const char* regex) {
  handle_ = nullptr;
  Compile(regex);
}


Regex::~Regex(void) {
  if(nullptr != handle_) delete (std::regex*)handle_;
}

bool Regex::Compile (const char* regex) {
  try {
    handle_ = new std::regex(regex);
  } catch(std::regex_error e) {
  }

//s  handle_ = xmlRegexpCompile((const xmlChar*)regex);
  return (handle_ != nullptr);
}

bool Regex::Execute(const char* value) {  
  if(handle_==nullptr) return false;

  std::string v = value;
  auto it_begin = std::sregex_iterator(v.begin(), v.end(), *((std::regex*)handle_));
  auto it_end = std::sregex_iterator();

  for(auto it=it_begin; it!=it_end; it++) {
    Console::WriteLine((*it)[it->size()-1].str().c_str());
  }

  //return !!xmlRegexpExec((xmlRegexpPtr)handle_, (const xmlChar*)value);

  return false;
}


String Regex::Match(const char* value) {
  if(handle_==nullptr) return String::Empty;

  std::string v = value;
  auto it_begin = std::sregex_iterator(v.begin(), v.end(), *((std::regex*)handle_));
  auto it_end = std::sregex_iterator();

  if(it_begin == it_end) return "";

  if(it_begin->size()==1) {
    return (*it_begin)[0].str().c_str();
  }

  return (*it_begin)[1].str().c_str();
}

String Regex::Replace (const char* input,const char* replacement) {
  if(handle_==nullptr) return String::Empty;

  std::string i = input;
  std::string r = replacement;
  return std::regex_replace(i,*((std::regex*)handle_),r).c_str();
}

