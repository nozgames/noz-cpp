///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AnimationTimeline_h__
#define __noz_Editor_AnimationTimeline_h__

namespace noz {
namespace Editor {

  class AnimationView;

  class AnimationTimeline : public UINode {
    NOZ_OBJECT()

    friend class AnimationView;

    private: ValueChangedEventHandler ValueChanged;

    private: NOZ_PROPERTY(Name=Font,Set=SetFont) ObjectPtr<Font> font_;
    private: NOZ_PROPERTY(Name=TextColor) Color text_color_;
    private: NOZ_PROPERTY(Name=BackgroundColor) Color background_color_;
    private: NOZ_PROPERTY(Name=LineColor) Color line_color_;
    private: NOZ_PROPERTY(Name=LineSpacing) noz_float spacing_;
    private: NOZ_PROPERTY(Name=SnapTime) noz_float snap_time_;
    private: NOZ_PROPERTY(Name=Margin) noz_float margin_;
    private: NOZ_PROPERTY(Name=TimeMarker) ObjectPtr<Sprite> time_marker_;
    private: NOZ_PROPERTY(Name=TimeMarkerColor) Color time_marker_color_;

    private: RenderMesh label_mesh_;
    private: RenderMesh mesh_;
    private: RenderMesh time_mesh_;
    private: noz_float zoom_;
    private: noz_float time_offset_;
    private: noz_float time_;
    private: noz_float label_interval_;

    private: AnimationView* animation_view_;

    public: AnimationTimeline (void);

    public: noz_float GetMargin (void) const {return margin_;}
    public: noz_float GetPixelsPerSecond(void) const {return zoom_ * spacing_;}
    public: noz_float GetTime (void) const {return time_;}
    public: noz_float GetTimeOffset (void) const {return time_offset_;}
    public: noz_float GetLabelInterval (void) const {return label_interval_;}

    public: void SetFont (Font* font);
    public: void SetTime (noz_float t);
    public: void SetZoom (noz_float z);
    public: void SetTimeOffset (noz_float o);

    public: virtual void RenderOverride (RenderContext* rc) override;
    public: virtual void ArrangeChildren (const Rect& r) override;
    public: virtual void OnMouseDown (SystemEvent* e) override;
    public: virtual void OnMouseOver (SystemEvent* e) override;
    public: virtual void OnMouseUp (SystemEvent* e) override;

    private: void AddTimeText (noz_float x, noz_float y, const char* text);

    private: void SetTimeFromPosition (const Vector2& position);
  };

} // namespace Editor
} // namespace noz

#endif //__noz_Editor_AnimationTimeline_h__

