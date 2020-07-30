///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/RenderTargetHandle.h>
#include "RenderTarget.h"

using namespace noz;
using namespace noz::Platform;


RenderTarget::RenderTarget(void) {
  handle_ = RenderTargetHandle::CreateInstance(this);
}

RenderTarget::~RenderTarget(void) {
  delete handle_;
}

void RenderTarget::SetImage(Image* image) {
  image_ = image;
}
