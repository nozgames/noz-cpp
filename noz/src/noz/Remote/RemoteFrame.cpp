///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>
#include "RemoteFrame.h"
#include "RemoteMessage.h"

using namespace noz;

RemoteFrame::RemoteFrame(noz_uint64 frame_id) {
  frame_id_ = frame_id;

  stream_.Write((char*)&frame_id_, 0, sizeof(frame_id_));
}

void RemoteFrame::WriteRectangle(const Rect& rect) {
  stream_.Write((char*)&rect.x, 0, sizeof(rect.x));
  stream_.Write((char*)&rect.y, 0, sizeof(rect.y));
  stream_.Write((char*)&rect.w, 0, sizeof(rect.w));
  stream_.Write((char*)&rect.h, 0, sizeof(rect.h));
}

void RemoteFrame::WriteCommand(Command cmd) {
  noz_byte b = (noz_byte)cmd;
  stream_.Write((char*)&b, 0, 1);
}

void RemoteFrame::WriteMatrix(const Matrix3& mat) {
  stream_.Write((char*)&mat.d[0], 0, sizeof(noz_float) * 9);
}

void RemoteFrame::WriteColor(Color color) {
  stream_.Write((char*)&color.raw, 0, sizeof(color.raw));
}

void RemoteFrame::WriteRenderMesh (const RenderMesh& mesh) {
  BinaryWriter writer(&stream_);
  mesh.Serialize(writer);
}

void RemoteFrame::WriteFloat(noz_float v) {
  stream_.Write((char*)&v, 0, sizeof(v));
}

RemoteFrame::Command RemoteFrame::ReadCommand(void) {
  noz_byte b = 0;
  stream_.Read((char*)&b, 0, 1);
  return (Command)b;
}

Rect RemoteFrame::ReadRectangle(void) {
  Rect rect;
  stream_.Read((char*)&rect.x, 0, sizeof(rect.x));
  stream_.Read((char*)&rect.y, 0, sizeof(rect.y));
  stream_.Read((char*)&rect.w, 0, sizeof(rect.w));
  stream_.Read((char*)&rect.h, 0, sizeof(rect.h));
  return rect;
}

Matrix3 RemoteFrame::ReadMatrix(void) {
  Matrix3 mat;
  stream_.Read((char*)&mat.d[0], 0, sizeof(noz_float) * 9);
  return mat;
}

Color RemoteFrame::ReadColor(void) {
  Color color;
  stream_.Read((char*)&color.raw, 0, sizeof(color.raw));
  return color;
}

void RemoteFrame::ReadRenderMesh (RenderMesh& mesh) {
  BinaryReader reader(&stream_);
  mesh.Deserialize(reader);
}

noz_float RemoteFrame::ReadFloat (void) {
  noz_float v = 0.0f;
  stream_.Read((char*)&v, 0, sizeof(v));
  return v;
}

RemoteMessage* RemoteFrame::ToMessage (void) {
  if(stream_.GetLength()==0) {
    return nullptr;
  }
    
  return new RemoteMessage(RemoteMessageType::Frame, (noz_byte*)stream_.GetBuffer(), stream_.GetLength());
}

RemoteFrame* RemoteFrame::FromMessage (RemoteMessage* msg) {
  if(msg->GetMessageType() != RemoteMessageType::Frame) {
    return nullptr;
  }

  Stream* stream = msg->GetStream();
  stream->Seek(0,SeekOrigin::Begin);

  noz_uint64 frame_id = 0;
  stream->Read((char*)&frame_id, 0, sizeof(noz_uint64));

  RemoteFrame* frame = new RemoteFrame(frame_id);

  noz_byte data[4096];
  noz_int32 size;

  while(0 != (size=stream->Read((char*)data, 0, 4096))) {
    frame->stream_.Write((char*)data,0,size);
  }
  
  frame->stream_.Seek(0,SeekOrigin::Begin);

  return frame;
}

void RemoteFrame::SeekBegin(void) {
  stream_.Seek(sizeof(noz_uint64),SeekOrigin::Begin);
}


