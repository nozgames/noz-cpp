///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>
#include "RenderMesh.h"

using namespace noz;


RenderMesh::RenderMesh(void) {
}

RenderMesh::RenderMesh(noz_uint32 vcapacity, noz_uint32 tcapacity) {
  SetCapacity(vcapacity,tcapacity);
}

void RenderMesh::SetColor(Color color) {
  if(verts_.empty()) return;
  
  Vertex* e = &verts_[verts_.size()-1];
  for(Vertex* v=&verts_[0]; v<=e; v++) {
    v->color = color;
  }
}

void RenderMesh::SetCapacity (noz_uint32 vcapacity, noz_uint32 tcapacity) {
  verts_.reserve(vcapacity);
  tris_.reserve(tcapacity);  
}

void RenderMesh::Serialize (BinaryWriter& writer) const {
  writer.WriteGuid(image_ ? image_->GetGuid() : Guid::Empty);
  writer.WriteUInt32(tris_.size());
  writer.WriteUInt32(verts_.size());
  writer.Write((char*)&tris_[0],0,sizeof(RenderMesh::Triangle) * tris_.size());
  writer.Write((char*)&verts_[0],0,sizeof(RenderMesh::Vertex) * verts_.size());
}

void RenderMesh::Deserialize (BinaryReader& reader) {
  Guid image_guid = reader.ReadGuid();
  noz_uint32 t_count = reader.ReadUInt32();
  noz_uint32 v_count = reader.ReadUInt32();

  image_ = AssetManager::LoadAsset<Image>(image_guid);

  Clear();
  verts_.resize(v_count);
  tris_.resize(t_count);
  reader.Read((char*)&tris_[0],0,sizeof(RenderMesh::Triangle) * t_count);
  reader.Read((char*)&verts_[0],0,sizeof(RenderMesh::Vertex) * v_count);
}

void RenderMesh::AddQuad (const Vector2& tl, const Vector2& br, const Vector2& s, const Vector2& t, Color color) {
  noz_uint32 v = verts_.size();
  verts_.push_back(Vertex(tl,s,color));
  verts_.push_back(Vertex(Vector2(br.x,tl.y),Vector2(t.x,s.y),color));
  verts_.push_back(Vertex(br,t,color));
  verts_.push_back(Vertex(Vector2(tl.x,br.y),Vector2(s.x,t.y),color));
  tris_.push_back(Triangle(v+0,v+1,v+2));
  tris_.push_back(Triangle(v+0,v+2,v+3));
}
