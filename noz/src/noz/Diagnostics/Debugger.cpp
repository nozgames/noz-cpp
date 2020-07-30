///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Debugger.h"

using namespace noz;

Debugger* Debugger::this_ = nullptr;

Debugger::Debugger (void) {
}

void Debugger::Initialize(void) {
  if(this_) return;

  this_ = new Debugger;
}

void Debugger::Uninitialize (void) {
  if(nullptr==this_) return;

  delete this_;
  this_ = nullptr;
}

void Debugger::ExecuteHook (Hook* hook, Object* o) {
#if 0
  // TODO: do all sorts of fun things..
  if(o && o->IsTypeOf(typeof(Component))) {    
    Name name = GetNodeDisplayName(((Component*)o)->GetNode());
    Console::Write("%08d: %s: [%08x] %s", 
      (noz_int32)Application::GetFrameNumber(), 
      hook->function_.ToCString(),
      o, 
      name.ToCString()
    );
    Console::WriteLine("");
  }
#endif  
}

Name Debugger::GetNodeDisplayName (Node* node) {
  Name name = node->GetName();
  if(name.IsEmpty()) {
    name = "Node";
  }

  return name;  
}


