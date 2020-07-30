///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RemoteClientRenderer.h"

using namespace noz;

RemoteClientRenderer::RemoteClientRenderer(void) {
}

void RemoteClientRenderer::RenderBegin(RenderContext* rc) {
  Renderer::RenderBegin(rc);
}

NOZ_FIXME()
/*
void RemoteClientRenderer::Render(RenderContext* rc) {
}
*/

void RemoteClientRenderer::RenderEnd(RenderContext* rc) {
  Renderer::RenderEnd(rc);
}
