///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryWriter.h>
#include "RemoteGraphicsContext.h"
#include "RemoteFrame.h"

using namespace noz;

RemoteGraphicsContext::RemoteGraphicsContext(RemoteFrame* frame) {
  frame_ = frame;
}

void RemoteGraphicsContext::SetTransform (const Matrix3& transform) {
  frame_->WriteCommand(RemoteFrame::Command::SetTransform);
  frame_->WriteMatrix(transform);
}

bool RemoteGraphicsContext::Draw (RenderMesh* mesh, noz_float opacity) {
  NOZ_TODO("Cull");
  frame_->WriteCommand(RemoteFrame::Command::DrawRenderMesh);
  frame_->WriteFloat(opacity);
  frame_->WriteRenderMesh(*mesh);
  return true;
}


