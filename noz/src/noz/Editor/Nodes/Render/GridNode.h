///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GridNode_h__
#define __noz_Editor_GridNode_h__

namespace noz {
namespace Editor {

  class GridNode : public Node {
    NOZ_OBJECT()

    /// Offset of grid blocks
    NOZ_PROPERTY(Name=Offset)
    private: Vector2 offset_;

    /// Size of a grid block
    NOZ_PROPERTY(Name=Size)
    private: noz_float size_;

    /// Grid scale
    NOZ_PROPERTY(Name=Scale)
    private: noz_float scale_;

    /// Color to render the grid
    NOZ_PROPERTY(Name=Color)
    private: Color color_;

    /// Horizontal enabled
    NOZ_PROPERTY(Name=Horizontal)
    private: bool horizontal_;

    /// Vertical enabled
    NOZ_PROPERTY(Name=Vertical)
    private: bool vertical_;

    NOZ_PROPERTY(Name=HorizontalAlignment,Set=SetHorizontalAlignment)
    private: Alignment horizontal_alignment_;

    NOZ_PROPERTY(Name=VerticalAlignment)
    private: Alignment vertical_alignment_;

    NOZ_PROPERTY(Name=Subdivisions,Set=SetSubdivisions)
    private: noz_float subdivisions_;

    NOZ_PROPERTY(Name=SubdivisionMinAlpha)
    private: noz_float subdivision_min_alpha_;

    NOZ_PROPERTY(Name=SubdivisionMaxAlpha)
    private: noz_float subdivision_max_alpha_;

    public: GridNode (void);

    public: ~GridNode (void);

    public: void SetSize (noz_float size) {size_ = size;}

    public: void SetScale (noz_float scale) {scale_ = scale;}

    public: void SetOffset (const Vector2& offset) {offset_ = offset;}

    public: void SetColor (Color color) {color_ = color;}

    public: void SetSubdivisions (noz_float subdivisions);

    public: void SetHorizontalAlignment (Alignment a) {
      horizontal_alignment_ = a;
    }

    public: Color GetColor (void) const {return color_;}

    public: const Vector2& GetOffset (void) const {return offset_;}

    /// Called to render the content.
    public: virtual bool Render (RenderContext* rc, Rect& r) override;

    private: void RenderGrid (RenderContext* rc, const Rect& r, Color color, noz_float size);
  };

} // namespace Editor
} // namespace noz

#endif //__noz_Editor_GridNode_h__

