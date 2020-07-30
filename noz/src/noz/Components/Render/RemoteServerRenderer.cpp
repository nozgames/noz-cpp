///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RemoteServerRenderer.h"

using namespace noz;

RemoteServerRenderer::RemoteServerRenderer(void) {
}

void RemoteServerRenderer::RenderBegin(RenderContext* rc) {
  Renderer::RenderBegin(rc);
}

NOZ_FIXME()
/*
void RemoteServerRenderer::Render(RenderContext* rc) {
}
*/

void RemoteServerRenderer::RenderEnd(RenderContext* rc) {
  Renderer::RenderEnd(rc);
}
