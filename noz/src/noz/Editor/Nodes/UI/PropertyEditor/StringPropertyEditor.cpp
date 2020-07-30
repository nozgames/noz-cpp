///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "StringPropertyEditor.h"

using namespace noz;
using namespace noz::Editor;

String StringPropertyEditor::ReadString (Object* t, Property* p) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::StringProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  return ((StringProperty*)p)->Get(t);
}

void StringPropertyEditor::WriteString (Object* t, Property* p, const String& value) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::StringProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  ((StringProperty*)p)->Set(t,value);
}
