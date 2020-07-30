///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_GridLayout_h__
#define __noz_GridLayout_h__

namespace noz {

  class GridRow : public Object {
    NOZ_OBJECT()

    public: NOZ_PROPERTY(Name=Height) noz_float height_;
    public: NOZ_PROPERTY(Name=HeightIsRatio) bool height_is_ratio_;
    public: NOZ_PROPERTY(Name=MinHeight) noz_float min_height_;

    public: GridRow (void) : height_(0.0f), min_height_ (0.0f), height_is_ratio_(false) {}
  };

  class GridColumn : public Object {
    NOZ_OBJECT()

    public: NOZ_PROPERTY(Name=Width) noz_float width_;
    public: NOZ_PROPERTY(Name=WidthIsRatio) bool width_is_ratio_;    
    public: NOZ_PROPERTY(Name=MinWidth) noz_float min_width_;    

    public: GridColumn(void) : width_(0.0f), min_width_(0.0f), width_is_ratio_(false) {}
  };

  class GridLayout : public Layout {
    NOZ_OBJECT(EditorIcon="{3F6F1463-B45C-4D35-9CCF-E17EF453D05F}")

    private: struct GridTrack {
      noz_float size_;
      bool size_is_ratio_;
      noz_float actual_size_;
      noz_float measured_size_;
      noz_float min_size_;
      noz_float offset_;
    };

    private: struct GridCell {
      noz_int32 row;
      noz_int32 col;
    };

    NOZ_PROPERTY(Name=Rows)
    private: std::vector<GridRow> rows_;

    NOZ_PROPERTY(Name=Columns)
    private: std::vector<GridColumn> cols_;
    
    NOZ_PROPERTY(Name=Orientation,Set=SetOrientation)
    private: Orientation orientation_;

    NOZ_PROPERTY(Name=CellPadding,Set=SetCellPadding)
    private: noz_float cell_padding_;

    NOZ_PROPERTY(Name=CellSpacing)
    private: noz_float cell_spacing_;

    private: bool tracks_invalid_;

    private: std::vector<GridTrack> tracks_[2];

    private: std::vector<GridCell> cells_;

    public: GridLayout(void);

    public: void SetOrientation (Orientation o);

    public: void AddRow (noz_float height, bool height_is_ratio=false);

    public: void AddColumn (noz_float width, bool width_is_ratio=false);

    public: void SetRow (noz_int32 row, noz_float height, bool height_is_ratio=false);

    public: void SetColumn (noz_int32 col, noz_float width, bool width_is_ratio=false);

    public: void SetCellPadding (noz_float v);

    /// Remove all rows and columns from the grid
    public: void Clear (void);

    public: virtual Vector2 MeasureChildren (const Vector2& available_size) override;
    
    public: virtual void ArrangeChildren (const Rect& r) override;

    private: void UpdateTracks (void);

    /// Arrange uses measure size of children to wrap them around arrange rect
    public: virtual bool IsArrangeDependentOnChildren (void) const override {return true;}
  };

} // namespace noz


#endif //__noz_GridLayout_h__


