///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"

using namespace noz;
using namespace noz::Editor;

const Name GlueMeta::NameAbstract("Abstract");
const Name GlueMeta::NameTypeCode("TypeCode");
const Name GlueMeta::NameManaged("Managed");
const Name GlueMeta::NameReadOnly("ReadOnly");
const Name GlueMeta::NameWriteOnly("WriteOnly");
const Name GlueMeta::NameType("Type");
const Name GlueMeta::NameName("Name");
const Name GlueMeta::NameSet("Set");
const Name GlueMeta::NameGet("Get");
const Name GlueMeta::NameSetSize("SetSize");
const Name GlueMeta::NameGetSize("GetSize");
const Name GlueMeta::NameAdd("Add");
const Name GlueMeta::NameElement("Element");
const Name GlueMeta::NameSetElement("SetElement");
const Name GlueMeta::NameNonSerialized("NonSerialized");

std::set<Name> GlueMeta::reserved_names_;

void GlueMeta::InitializeReservedNames(void) {
  reserved_names_.insert(NameAbstract);
  reserved_names_.insert(NameTypeCode);
  reserved_names_.insert(NameManaged);
  reserved_names_.insert(NameReadOnly);
  reserved_names_.insert(NameWriteOnly);
  reserved_names_.insert(NameType);
  reserved_names_.insert(NameName);
  reserved_names_.insert(NameSet);
  reserved_names_.insert(NameSetSize);
  reserved_names_.insert(NameGet);
  reserved_names_.insert(NameGetSize);
  reserved_names_.insert(NameAdd);
  reserved_names_.insert(NameElement);
  reserved_names_.insert(NameSetElement);  
  reserved_names_.insert(NameNonSerialized);
}

