///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "NamePropertyEditor.h"

using namespace noz;
using namespace noz::Editor;

String NamePropertyEditor::ReadString (Object* t, Property* p) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::NameProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  return ((NameProperty*)p)->Get(t);
}

void NamePropertyEditor::WriteString (Object* t, Property* p, const String& value) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::NameProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  ((NameProperty*)p)->Set(t,value);
}
