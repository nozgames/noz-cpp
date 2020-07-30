///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Text/StringLexer.h>

using namespace noz;

namespace noz {
  static std::map<String,Type*>* types_by_qualified_name_ = nullptr;
  static std::map<String,Type*>* types_by_name_ = nullptr;
  static std::vector<Type*>* types_ = nullptr;
  static noz_uint32 next_prime_index = 1;
}

Type::Type(const char* name, TypeAttributes attr, TypeCode code, Type* base) : qualified_name_(name) {
  attr_ = attr;
  code_ = code;
  base_ = base;
  property_mask_ = 0;
  defaults_ = nullptr;
  allocator_ = nullptr;

  // Initially set the identifier to the index within its base class
  if(name[0] != '{') {
    data_defined_type_ = false;
    id_mask_ = 0;
    child_mask_ = 0;
    if(base_) {
      id_ = ++base_->id_mask_;
    } else {
      id_ = 0;
    }
  } else {    
    data_defined_type_ = true;
    id_ = base_->id_;
    id_mask_ = base_->id_mask_;
    child_mask_ = base->child_mask_;
    code = base_->GetTypeCode();
  }

  // Find short name..
  String temp = name;
  noz_int32 colon = temp.LastIndexOf(':');

  // Set editor only flag
  const String editor_namespace("noz::Editor");
  if(String::Compare(temp,0,editor_namespace,0,editor_namespace.GetLength())==0) {
    attr_ = attr_ | TypeAttributes::EditorOnly;
  }

  if(data_defined_type_) {
    noz_assert(colon != -1);
    name_ = base_->name_;
  } else if(colon != -1) {
    name_ = temp.Substring(colon+1);
  } else {
    name_ = qualified_name_;
  }

#if defined(NOZ_EDITOR)
  if(data_defined_type_) {
    editor_name_ = base_->editor_name_;
  } else {
    editor_name_ = name_;
  }
#endif

  if(code==TypeCode::Class) {
    for(Type* base=this; base; base=base->base_) {
      if(base==typeof(Object)) {
        code = TypeCode::Object;
        break;
      }
    }
  }
}

Type::~Type(void) {
  for(auto itm=method_map_.begin(); itm!=method_map_.end(); itm++) {
    delete itm->second;
  }
  method_map_.clear();

  for(auto it=properties_.begin(); it!=properties_.end(); it++) {
    delete *it;
  }
  properties_.clear();
  delete allocator_;

  noz_assert(nullptr==defaults_);
}

const std::vector<Type*>& Type::GetTypes(void) {
  return *types_;
}

void Type::RegisterType(Type* type) {
  if(types_by_qualified_name_==nullptr) {
    types_by_qualified_name_ = new std::map<String,Type*>;
    types_by_name_ = new std::map<String,Type*>;
    types_ = new std::vector<Type*>;
  }
  (*types_by_qualified_name_)[type->qualified_name_] = type;

  // Do not data defined types by just name
  if(!type->data_defined_type_) (*types_by_name_)[type->name_] = type;

  // Create defaults for data defined type
  if(type->data_defined_type_) type->defaults_ = type->CreateInstance();

  (*types_).push_back(type);
}

Type* Type::RegisterTypeFromAsset (const Name& name) {
  // Type name must start with '{' symbol
  if(name.ToString()[0] != '{') {
    Console::WriteError("data defined type name '%s' must start with an '{'", name.ToCString());
    return nullptr;
  }

  // Extract the asset type
  noz_int32 separator = name.ToString().LastIndexOf('@');
  if(-1 == separator) {
    Console::WriteError("missing '@' in data defined type name '%s'", name.ToCString());
    return nullptr;
  }

  Name asset_type_name = name.ToString().Substring(1,separator-1);
  if(asset_type_name.IsEmpty()) {
    Console::WriteError("invalid asset type '%s' in data defined type name '%s'", asset_type_name.ToCString(), name.ToCString());
    return nullptr;
  }

  // Find the asset type
  Type* asset_type = FindType(asset_type_name);
  if(nullptr == asset_type) {
    Console::WriteError("unknown asset type '%s' in data defined type name '%s'",  asset_type_name.ToCString(), name.ToCString());
    return nullptr;
  }

  // Extract the asset guid
  Guid asset_guid = Guid::Parse(name.ToCString()+separator+1);

  // Load the asset
  Asset* asset = AssetManager::LoadAsset(asset_type,asset_guid);
  if(nullptr == asset) {
    Console::WriteError("%s: failed to load asset for data defined type '%s'", asset_guid.ToString().ToCString(), name.ToCString());
    return nullptr;
  }

  // Now that the asset has been loaded recheck the type for existance. We need to directly
  // check the qualified name hash since calling FindType could get us into an infinite
  // recursion if the asset failed to register the type for some reason.
  auto it = types_by_qualified_name_->find(name);
  if(it != types_by_qualified_name_->end()) {
    return it->second;
  }
    
  return nullptr;
}

Type* Type::FindType(const Name& name) {
  auto it = types_by_qualified_name_->find(name);
  if(it != types_by_qualified_name_->end()) {
    return it->second;
  }
  it = types_by_name_->find(name);
  if(it != types_by_name_->end()) {
    return it->second;
  }

  // If the type was not found and the type definition is a dynamic definition
  // registered within an asset then register it now.
  if(name.ToString()[0] == '{') {
    return RegisterTypeFromAsset(name);
  }

  return nullptr;
}

Object* Type::CreateInstance(ObjectAllocatorAttributes attr) {
  if(allocator_==nullptr) return nullptr;
  return allocator_->CreateInstance(attr);
}

void Type::SetAllocator(ObjectAllocator* allocator) {
  delete allocator_;
  allocator_ = allocator;
}

void Type::UnregisterAllTypes(void) {
  if(nullptr==types_by_qualified_name_) {
    return;
  }
  // Delete all defaults first before the types are gone 
  for(auto it=types_by_qualified_name_->begin(); it!=types_by_qualified_name_->end(); it++) {
    delete it->second->defaults_;
    it->second->defaults_ = nullptr;
  }
  for(auto it=types_by_qualified_name_->begin(); it!=types_by_qualified_name_->end(); it++) {
    delete it->second;
  }
  delete types_by_qualified_name_;
  delete types_by_name_;
  types_by_qualified_name_ = nullptr;
  types_by_name_ = nullptr;
}

bool Type::RegisterMeta (const Name& name, const char* value) {
#if defined(NOZ_EDITOR)
  static String EditorName("EditorName");
  if(name==EditorName) {
    editor_name_ = value;
    return true;
  }
#endif

  meta_[name] = value;
  return true;
}

String Type::GetMeta (const Name& name) const {
  auto it = meta_.find(name);
  if(it != meta_.end()) {
    return it->second;
  }

  return String::Empty;
}

bool Type::RegisterMethod(Name name,Method* method) {
  if(nullptr==method) {
    return false;
  }

  method->parent_type_ = this;
  method->SetName(name);
  method_map_[name] = method;
  methods_.push_back(method);
  return true;
}

bool Type::RegisterProperty(Name name,Property* prop) {
  if(property_mask_==0) {
    for(Type* type=this; type && property_mask_==0; type=type->base_) {
      noz_assert((type->property_mask_ & (noz_uint64(1)<<63))==0);
      property_mask_ = (type->property_mask_<<1) & (~type->property_mask_);
    }
    if(property_mask_==0) {
      property_mask_ = 1;
    }
  } else {
    noz_assert((property_mask_ & (noz_uint64(1)<<63))==0);
    property_mask_ = (property_mask_<<1) | property_mask_;
  }
  
  prop->type_ = this;
  prop->name_ = name;
  property_map_[name] = prop;
  properties_.push_back(prop);
  return true;
}


Method* Type::GetMethod(const Name& name) {
  auto it = method_map_.find(name);
  if(it != method_map_.end()) {
    return it->second;
  }
  if(base_) {
    return base_->GetMethod(name);
  }

  return nullptr;
}

Property* Type::GetProperty(const Name& name) const {
  auto it = property_map_.find(name);
  if(it != property_map_.end()) {
    return it->second;
  }

  if(base_) {
    return base_->GetProperty(name);
  }

  return nullptr;
}

void Type::GenerateMasks(void) {
  if(nullptr==types_) return;

  Type* t = typeof(Component);
  Type* tt = typeof(Object);

  typeof(Object)->child_mask_ = typeof(Object)->id_mask_ == 0 ? 0 : NOZ_BIT((64-Math::CountLeadingZeroBits(typeof(Object)->id_mask_))) - 1;
  typeof(Object)->id_mask_ = 0;

  for(noz_uint32 i=1;i<types_->size(); i++) {
    Type* type = (*types_)[i];
    noz_assert(type);

    if(type->base_ == nullptr) continue;

    // The id_mask_ value is the number of sub classes before GenerateMask is called.  Convert
    // that count to a mask that encompasses the entire count..

    if(type->id_mask_==0) {
      type->child_mask_ = 0;
    } else {
      type->child_mask_ = NOZ_BIT((64-Math::CountLeadingZeroBits(type->id_mask_))) - 1;
    }
    type->child_mask_ <<= (64-Math::CountLeadingZeroBits(type->base_->child_mask_));
    type->child_mask_ |= type->base_->child_mask_;

    type->id_mask_ = type->base_->child_mask_;

    type->id_ <<= (64-Math::CountLeadingZeroBits(type->base_->id_mask_));
    type->id_ |= type->base_->id_;
  }

  // Create defaults
  for(noz_uint32 i=1;i<types_->size(); i++) {
    Type* type = (*types_)[i];
    noz_assert(type);

    type->defaults_ = type->CreateInstance();
  }
}

std::vector<Type*> Type::GetTypes (Type* base, const char* contains) {
  std::vector<Type*> result;
  for(noz_uint32 i=0,c=types_->size();i<c;i++) {
    Type* t = (*types_)[i];
    noz_assert(t);

    // Ensure the type is based on the given type
    if(!t->IsCastableTo(base)) continue;

    // Check contains text.
    if(contains && -1==base->GetName().ToString().IndexOf(contains,0,StringComparison::OrdinalIgnoreCase)) continue;

    result.push_back(t);
  }
  return result;
}
