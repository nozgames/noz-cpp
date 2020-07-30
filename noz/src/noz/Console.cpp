///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////
#include <noz.pch.h>

#if !defined(NOZ_LEAN)
#include <noz/Assets/Asset.h>
#endif

#if defined(NOZ_WINDOWS)
#include <Windows.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#elif defined(NOZ_IOS)
#endif

using namespace noz;

namespace noz {
#if defined(NOZ_WINDOWS)
  static void ConsolePrint(const char* value) {
    OutputDebugString(value);
    printf(value);
  }
#else
  void ConsolePrint(const char* value);
#endif
}

Console* Console::this_ = nullptr;
ConsoleMessageEventHandler Console::ConsoleMessageWritten;

Console::~Console(void) {
  for(auto it=vars_.begin(); it!=vars_.end(); it++) {
    delete it->second;
  }
  vars_.clear();
}

void Console::Initialize(void) {
  if(this_!=nullptr) return;

  this_ = new Console;
}

void Console::Uninitialize(void) {
  if(this_==nullptr) return;

  delete this_;
  this_ = nullptr;
}

Int32ConsoleVariable* Console::RegisterInt32Variable (const Name& name, noz_int32 value, noz_int32 value_min, noz_int32 value_max) {
  if(this_==nullptr) return nullptr;

  auto it=this_->vars_.find(name);
  if(it!=this_->vars_.end()) {
    return nullptr;
  }

  Int32ConsoleVariable* var = new Int32ConsoleVariable(name, value, value_min, value_max);
  this_->vars_[var->name_] = var;
  return var;
}

void Console::WriteLine(ConsoleMessageType type, Object* context, const char* msg) {
  switch(type) {
    case ConsoleMessageType::Error: ConsolePrint ("error: "); break;
    case ConsoleMessageType::Warning: ConsolePrint ("warning: "); break;
    default: break;
  }

  ConsolePrint(msg);
  ConsolePrint("\n");

#if !defined(NOZ_LEAN)
  if(context) {
    if(context->GetType()->IsAsset()) {
      ConsolePrint(((Asset*)context)->GetName().ToCString());
    } else {
      ConsolePrint(context->GetType()->GetName().ToCString());
    }

    ConsolePrint(": ");
  }    
#endif

#if !defined(NOZ_LEAN)
  ConsoleMessageWritten(type,context,msg);
#endif
 
}

void Console::WriteLine(const char* format, ...) {
  va_list args1;	
  va_list args2;
	va_start(args1,format);
	va_start(args2,format);
  String msg = String::Format(format,args1,args2);
  va_end(args1);
  va_end(args2);
  WriteLine(ConsoleMessageType::Info,nullptr,msg.ToCString());
}

void Console::WriteLine(Object* context, const char* format, ...) {
  va_list args1;	
  va_list args2;
	va_start(args1,format);
	va_start(args2,format);
  String msg = String::Format(format,args1,args2);
  va_end(args1);
  va_end(args2);
  WriteLine(ConsoleMessageType::Info,context,msg.ToCString());
}

void Console::WriteError (const char* format, ...) {
  va_list args1;	
  va_list args2;
	va_start(args1,format);
	va_start(args2,format);
  String msg = String::Format(format,args1,args2);
  va_end(args1);
  va_end(args2);
  WriteLine(ConsoleMessageType::Error,nullptr,msg.ToCString());
}

void Console::WriteError (Object* context, const char* format, ...) {
  va_list args1;	
  va_list args2;
	va_start(args1,format);
	va_start(args2,format);
  String msg = String::Format(format,args1,args2);
  va_end(args1);
  va_end(args2);
  WriteLine(ConsoleMessageType::Error,context,msg.ToCString());
}

void Console::WriteWarning (const char* format, ...) {
  va_list args1;	
  va_list args2;
	va_start(args1,format);
	va_start(args2,format);
  String msg = String::Format(format,args1,args2);
  va_end(args1);
  va_end(args2);
  WriteLine(ConsoleMessageType::Warning,nullptr,msg.ToCString());
}

void Console::WriteWarning (Object* context, const char* format, ...) {
  va_list args1;	
  va_list args2;
	va_start(args1,format);
	va_start(args2,format);
  String msg = String::Format(format,args1,args2);
  va_end(args1);
  va_end(args2);
  WriteLine(ConsoleMessageType::Warning,context,msg.ToCString());
}

ConsoleVariable::ConsoleVariable(const Name& name) {  
  name_ = name;
}

noz_int32 Console::GetVariableInt32 (const Name& var) {
  auto it = this_->vars_.find(var);
  if(it != this_->vars_.end()) {
    return it->second->GetInt32();
  }
  return 0;
}

void Console::SetVariableInt32 (const Name& var, noz_int32 v) {
  auto it = this_->vars_.find(var);
  if(it == this_->vars_.end()) {
    return;
  }
  
  it->second->SetInt32(v);
}
