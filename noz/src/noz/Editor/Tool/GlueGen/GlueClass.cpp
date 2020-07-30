///////////////////////////////////////////////////////////////////////////////
// noZ Glue Compiler
// Copyright (C) 2013-2014 Bryan Dube / Radius Software
// http://www.radius-software.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"

using namespace noz;
using namespace noz::Editor;

GlueClass::GlueClass(void) {
  gf_ = nullptr;
  gf_line_ = 0;
  excluded_ = true;
  reflected_ = false;
  abstract_ = false;
  base_ = nullptr;
  non_template_base_ = nullptr;
  interface_ = false;
  processed_ = false;
  template_ = false;
  sort_key_ = 0;
  nested_parent_ = nullptr;
  default_constructor_ = false;
}

bool GlueClass::ResolveBaseClasses (GlueState* gs) {
  // Patch up base classes.
  if(!base_names_.empty() && IsReflected()) {
    for(auto it_base_name=base_names_.begin(); nullptr == base_ && it_base_name!=base_names_.end(); it_base_name++) {
      base_ = gs->ResolveClass(*it_base_name,namespace_);
      if(nullptr==base_) {
        GlueGen::ReportError (gf_,gf_line_,"base class '%s' not found for class '%s'", (*it_base_name).ToCString(), name_.ToCString());
        return false;
      }

      base_->ResolveBaseClasses(gs);

/* TODO: interfaces ?
      // Add as an interface?
      if(base->is_interface) {
        interfaces.push_back(base);
      } else {
        // Ensure that there is only one non interface base class.
        if(parent != nullptr) {
          Console::WriteLine("%s(%d): error: class '%s' contains more than one non interface base class ('%s' and '%s')", 
            file->GetFullPath().ToCString(), line, name.c_str(),
            parent->qualified_name.c_str(), 
            base->qualified_name.c_str()
            );
          return false;
        }
        parent = base;
      }
*/
    }

    // Set non template to base initially.
    non_template_base_ = base_;

    // Ensure that there is a non-template base class that is reflected
    if(non_template_base_->IsTemplate()) {
      while(non_template_base_->IsTemplate()) {
        non_template_base_ = non_template_base_->base_;
        if(nullptr == non_template_base_) {
          GlueGen::ReportError(gf_, gf_line_, "class '%s' has no non-template base class", name_.ToCString());
          return false;
        }

        if(!non_template_base_->IsReflected()) {
          GlueGen::ReportError(non_template_base_->gf_,non_template_base_->gf_line_,"non-template base class '%s' is not exported for class '%s'", non_template_base_->GetQualifiedName().ToCString(), name_.ToCString());
          return false;
        }
      }

    // Ensure the base class is reflected
    } else if (!base_->IsReflected()) {
      GlueGen::ReportError(base_->gf_,base_->gf_line_,"base class '%s' not exported for class '%s'", base_names_[0].ToCString(), name_.ToCString());
      return false;
    }
  }

  return true;
}

bool GlueClass::Process (GlueState* gs) {
  // Do not repeat processing the class
  if(processed_) return true;

  // Mark the class as processed.
  processed_ = true;

  // Determine depth.
  noz_int32 depth = 0;
  for(GlueClass* p = base_; p; p=p->base_) {
    depth++;
  }

  sort_key_ |= (255-depth);

  if(!((String&)qualified_name_).CompareTo("noz::Object")) sort_key_ |= (1<<9);

  return true;
}

bool GlueClass::IsSubclassOf(GlueClass* gc) const {
  for(GlueClass* p = base_; p; p=p->base_) if(p == gc) return true;
  return false;
}

GlueMethod* GlueClass::AddConstructor(GlueMethod* gm) {
  constructors_.push_back(gm);

  // If constructor has no parameters or all of its parameters are default then
  // then  class has a default construtor.
  if(gm->GetParameters().empty() || gm->GetParameters()[0]->IsDefault() ) {
    default_constructor_ = true;
  }

  return gm;
}

