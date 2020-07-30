///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Int32PropertyEditor.h"

using namespace noz;
using namespace noz::Editor;

String Int32PropertyEditor::ReadString (Object* t, Property* p) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::Int32Property);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  return String::Format("%d", ((Int32Property*)p)->Get(t));
}

void Int32PropertyEditor::WriteString (Object* t, Property* p, const String& value) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::Int32Property);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  ((Int32Property*)p)->Set(t,Int32::Parse(value));  
}
