///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "PerformanceCounter.h"

using namespace noz;

std::map<Name,PerformanceCounter*>* PerformanceCounter::counters_ = nullptr;

PerformanceCounter::PerformanceCounter (const Name& name) {
  value_ = 0;
  name_ = name;  

  if(counters_ == nullptr) {
    counters_ = new std::map<Name,PerformanceCounter*>;
  }
  (*counters_)[name] = this;
}

PerformanceCounter::~PerformanceCounter(void) {
  auto it = counters_->find(name_);
  if(it!=counters_->end()) {
    counters_->erase(it);
  }
  if(counters_->empty()) {
    delete counters_;
    counters_ = nullptr;
  }
}

PerformanceCounter* PerformanceCounter::GetPerformanceCounter(const Name& name) {
  auto it = counters_->find(name);
  if(it!=counters_->end()) {
    return it->second;
  }
  return nullptr;
}

void PerformanceCounter::ClearAll (void) {
  if(nullptr==counters_) return;
  for(auto it=counters_->begin(); it!=counters_->end(); it++) {
    it->second->Clear();
  }
}

void PerformanceCounter::Clear(void) {
  value_ = 0;
}

void PerformanceCounter::Increment(void) {
  value_++;
}

void PerformanceCounter::IncrementBy(noz_uint64 inc) {
  value_ += inc;
}

void PerformanceCounter::Decrement(void) {
  value_ --;
}

void PerformanceCounter::DecrementBy(noz_uint64 dec) {
  value_ -= dec;
}

void PerformanceCounter::PrintAll (void) {
  if(nullptr==counters_) return;

  StringBuilder sb;

  bool printed = false;
  for(auto it=counters_->begin(); it!=counters_->end(); it++) {
    if(it->second->value_ != 0 ) {
      sb.Append(String::Format("[%s: %lld] ", it->second->name_.ToCString(), it->second->value_));
    }
  }

  if(sb.GetLength()>0) Console::WriteLine(sb.ToString().ToCString());
}

void PerformanceCounter::Print (void) {
  //Console::Write ("[%s: %lld] ", name_.ToCString(), value_);
}

