///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RenderMesh_h__
#define __noz_RenderMesh_h__

namespace noz {

  class Image;
  class BinaryReader;
  class BinaryWriter;

  class RenderMesh {
    public: struct Vertex {
      Vertex(void) {color=Color::White;}
      Vertex(const Vector2& _xy, const Vector2& _uv, Color _color) : xy(_xy), uv(_uv), color(_color) {}

      Vector2 xy;
      Vector2 uv;
      Color color;      
    };

    public: struct Triangle {
      noz_uint16 index[3];

      Triangle(void) {index[0]=index[1]=index[2]=0;}
      Triangle(noz_uint16 i1, noz_uint16 i2, noz_uint16 i3) {index[0]=i1;index[1]=i2;index[2]=i3;}
    };

    private: std::vector<Vertex> verts_;
    private: std::vector<Triangle> tris_;
    private: ObjectPtr<Image> image_;

    /// Creates an empty render mesh
    public: RenderMesh(void);

    /// Constructs a render mesh with initial vertex and triangle capacities
    public: RenderMesh(noz_uint32 vcapacity, noz_uint32 tcapacity);
    
    public: void Clear (void) {verts_.clear(); tris_.clear();}

    public: void SetImage(Image* image) {image_ = image;}

    public: void SetCapacity (noz_uint32 vcapacity, noz_uint32 tcapacity);

    public: const std::vector<Vertex>& GetVerticies (void) const {return verts_;}

    public: const std::vector<Triangle>& GetTriangles (void) const {return tris_;}

    public: Image* GetImage(void) const {return image_;}

    public: void AddQuad (const Vector2& tl, const Vector2& br, const Vector2& s, const Vector2& t, Color color);

    public: void AddVertex (const Vector2& xy, const Vector2& uv, Color color ) {
      verts_.push_back(Vertex(xy,uv,color));
    }

    public: void AddTriangle (noz_uint16 i1, noz_uint16 i2, noz_uint16 i3) {
      tris_.push_back(Triangle(i1,i2,i3));
    }

    public: void SetColor (Color color);

    public: void Serialize (BinaryWriter& writer) const;

    public: void Deserialize (BinaryReader& reader);

    public: bool IsEmpty (void) const {return tris_.empty();}
  };

} // namespace noz


#endif //__noz_RenderMesh_h__

