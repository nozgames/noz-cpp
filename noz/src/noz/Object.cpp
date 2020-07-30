///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

static std::vector<void(*)(Object*)> object_destructor_hooks_;


ObjectManager* ObjectManager::this_ = nullptr;

void ObjectManager::Initialize(void) {
  if(this_) return;

  this_ = new ObjectManager;
}

void ObjectManager::Uninitialize(void) {
  if(!this_) return;

  delete this_;
  this_ = nullptr;
}

ObjectManager::ObjectManager(void) {
  next_free_slot_ = 0;
  slots_.emplace_back();
  slots_.back().refs_ = 0;
  slots_.back().object_ = nullptr;
}

noz_uint32 ObjectManager::AllocSlot (Object* o) {
  if(nullptr==this_) return 0;

  Slot* slot = nullptr;
  noz_uint32 slot_id = 0;
  if(this_->next_free_slot_ !=0) {
    slot_id = this_->next_free_slot_;
    slot = &this_->slots_[slot_id];
    this_->next_free_slot_ = slot->refs_;
  } else {
    slot_id = this_->slots_.size();
    this_->slots_.emplace_back();
    slot = &this_->slots_.back();
  }
  slot->refs_ = 1;
  slot->object_ = o;
  return slot_id;
}

void ObjectManager::SetSlotObject (noz_uint32 slot_id, Object* o) {
  if(this_==nullptr || slot_id==0) return;

  Slot& slot = this_->slots_[slot_id];
  noz_assert(slot.object_==nullptr || o==nullptr);
  if(o) {
    slot.object_ = o;
    slot.refs_ = 1;
  } else {
    slot.object_ = nullptr;
    ReleaseSlot(slot_id);
  }
}

void ObjectManager::AddSlotRef (noz_uint32 slot_id) {
  if(this_==nullptr || slot_id==0) return;
  Slot& slot = this_->slots_[slot_id];
  slot.refs_++;
}

void ObjectManager::ReleaseSlot (noz_uint32 slot_id) {
  if(this_==nullptr || slot_id==0) return;

  Slot& slot = this_->slots_[slot_id];
  noz_assert(slot.refs_>0);
  slot.refs_--;
  if(slot.refs_==0) {
    slot.refs_ = this_->next_free_slot_;
    this_->next_free_slot_ = slot_id;
  }
}

Object::Object (const Object& o) {
  id_ = ObjectManager::AllocSlot (this);
  deserializing_ = false;
}

Object::Object(void) {
  id_ = ObjectManager::AllocSlot (this);
  deserializing_ = false;
}

Object::~Object(void) {
  if(!object_destructor_hooks_.empty()) {
    for(auto it=object_destructor_hooks_.begin(); it!= object_destructor_hooks_.end(); it++) {
      (*it)(this);
    }
  }

  ObjectManager::SetSlotObject(id_, nullptr);
}

String Object::ToString(void) const {
  return String::Empty;
}


bool Object::IsTypeOf(Type* _type) const{
  return GetType()->IsCastableTo(_type);
}

Property* Object::GetProperty(const Name& name) const {
#if defined(NOZ_LEAN)
  return nullptr;
#else 
  return GetType()->GetProperty(name);
#endif
}

void Object::OnDeserializing (void) {
  deserializing_ = true; 
}

void Object::OnDeserialized (void) {
  deserializing_ = false;
}

