///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

namespace noz {
  
  class NameValue {
    public: String string_;
    public: noz_uint32 refs_;
    public: noz_uint32 hash_;
    public: NameValue* next_;
    public: NameValue* prev_;
  };

  class NameHash {
    /// Static size of name hash.
    private: static const noz_uint32 Size = 4096;

    /// Hash entries
    private: std::vector<NameValue*> entries_;

    private: noz_uint32 count_;

    private: bool purge_unreferenced_;;

    public: NameHash(void) {
      entries_.resize(Size);
      memset(&entries_[0], 0, sizeof(NameValue*) * Size);
      count_= 0;
      purge_unreferenced_ = false;
    }

    public: ~NameHash(void) {
      for(noz_uint32 hash=0; hash<Size; hash++) {
        for(NameValue* value=entries_[hash]; value; ) {
          NameValue* temp = value;
          value = value->next_;
          delete temp;
        }
      }
      entries_.clear();
    }

    public: void SetPurgeOnUnreferenced(bool purge_unreferenced) {
      // First purge all unreferenced
      Purge();

      purge_unreferenced_ = purge_unreferenced;
    }

    public: void Purge(void) {
      for(noz_uint32 hash=0; hash<Size; hash++) {
        for(NameValue* value=entries_[hash]; value; ) {
          NameValue* temp = value;
          value = value->next_;
          if(temp->refs_ == 0) RemoveValueInternal(temp);
        }
      }
    }

    public: noz_uint32 GetCount(void) const {return count_;}

    public: NameValue* AddValue (const char* s) {
      noz_uint32 hash = String::GetHashCode(s) % Size;
      NameValue* value;
      for(value = entries_[hash];value;value=value->next_) {
        if(!value->string_.CompareTo(s)) {
          value->refs_ ++;
          return value;
        }
      }

      // Create a new name value.
      value = new NameValue;
      value->hash_ = hash;
      value->string_ = s;
      value->refs_ = 1;
      value->next_ = entries_[hash];
      value->prev_ = nullptr;
      if(value->next_) value->next_->prev_ = value;
      entries_[hash] = value;
      count_++;
      return value;      
    }

    public: void RemoveValue(NameValue* value) {
      noz_assert(value->refs_>0);
      value->refs_--;
      if(value->refs_>0 || !purge_unreferenced_) return;
      RemoveValueInternal(value);
    }

    private: void RemoveValueInternal(NameValue* value) {
      noz_assert(value->refs_ == 0);
      if(value->prev_) {
        value->prev_->next_ = value->next_;
      } else {
        entries_[value->hash_] = value->next_;
      }
      if(value->next_) value->next_->prev_ = value->prev_;            
      delete value;
      count_--;
    }
  };  
}

Name Name::Empty("");
NameHash* Name::hash_ = nullptr;

Name::Name(const char* s) {
  if(nullptr==hash_) hash_ = new NameHash;
  value_ = hash_->AddValue(s);
}

Name::Name(const String& value) : Name(value.ToCString()) {
}

Name::Name(const Name& value) {
  value_ = value.value_;
  value_->refs_++;
}

Name::~Name(void) {
  String v = value_->string_;
  hash_->RemoveValue(value_);
  //hash_->Print();
  if(hash_->GetCount() == 0) { 
    delete hash_; 
    hash_ = nullptr;
  }
}

Name& Name::operator= (const Name& v) {
  value_ = v.value_;
  value_->refs_++;
  return *this;
}

Name& Name::operator= (const String& v) {
  *this = v.ToCString();
  return *this;
}

Name& Name::operator= (const char* v) {
  noz_assert(hash_);
  value_ = hash_->AddValue(v);
  return *this;
}

bool Name::operator< (const Name& o) const {
  noz_assert(value_);
  noz_assert(o.value_);
  if(value_ == nullptr && o.value_ != nullptr) return true;
  if(value_ == nullptr || o.value_ == nullptr) return false;
  return value_ < o.value_;
}

bool Name::operator> (const Name& o) const {
  noz_assert(value_);
  noz_assert(o.value_);
  if(value_ != nullptr && o.value_ == nullptr) return true;
  if(value_ == nullptr || o.value_ == nullptr) return false;
  return value_ > o.value_;
}

const char* Name::ToCString(void) const {
  noz_assert(value_);
  return value_->string_.ToCString();
}

const String& Name::ToString(void) const {
  noz_assert(value_);
  return value_->string_;
}

void Name::Purge(void) {
  if(hash_) hash_->Purge();
  if(hash_->GetCount() == 0) {
    delete hash_;
    hash_ = nullptr;
  }
}

void Name::SetPurgeOnUnreferenced(bool purge_unreferenced) {
  if(hash_) hash_->SetPurgeOnUnreferenced(purge_unreferenced);
  if(hash_->GetCount() == 0) {
    delete hash_;
    hash_ = nullptr;
  }
}
