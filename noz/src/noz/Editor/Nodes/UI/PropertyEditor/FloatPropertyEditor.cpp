///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Actions/SetPropertyAction.h>
#include "FloatPropertyEditor.h"

using namespace noz;
using namespace noz::Editor;

String FloatPropertyEditor::ReadString (Object* t, Property* p) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::FloatProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  return Float(((FloatProperty*)p)->Get(t)).ToString();
}

void FloatPropertyEditor::WriteString (Object* t, Property* p, const String& value) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::FloatProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  EditorDocument::GetActiveDocument(this)->ExecuteAction(new SetFloatPropertyAction(t,p,Float::Parse(value)));
}
