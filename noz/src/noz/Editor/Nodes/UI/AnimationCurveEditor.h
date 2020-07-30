///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AnimationCurveEditor_h__
#define __noz_Editor_AnimationCurveEditor_h__

#include "AnimationTimeline.h"

namespace noz {
namespace Editor {

  class AnimationView;
  class AnimationViewTrack;

  class AnimationCurveEditor : public UINode {
    NOZ_OBJECT(DefaultStyle="{C6EFC5B6-3D4A-4382-9569-A77D37866917}")

    friend class AnimationView;
    friend class AnimationCurveEditorKeyFrame;

    private: struct CurveKey {
      AnimationViewTrack* av_track_;
      noz_uint32 kf_index_;
      KeyFrame* kf_;
      Color color_;
      Vector2 position_;
      Rect rect_;
      Rect handle_rect_[2];
    };

    private: enum class DragMode {
      None,
      TangentIn,
      TangentOut,
      Key
    };

    private: NOZ_CONTROL_PART(Name=Timeline) ObjectPtr<AnimationTimeline> timeline_;
    private: NOZ_CONTROL_PART(Name=CurrentTimeSpacer)ObjectPtr<Spacer> current_time_spacer_;
    private: NOZ_PROPERTY (Name=Margin) noz_float margin_;
    private: NOZ_PROPERTY (Name=PixelsPerSecond) noz_float pixels_per_second_;
    private: NOZ_PROPERTY (Name=LoopCurveColor) Color loop_curve_color_;
    private: NOZ_PROPERTY (Name=CurveColor1) Color curve_color_1_;
    private: NOZ_PROPERTY (Name=CurveColor2) Color curve_color_2_;
    private: NOZ_PROPERTY (Name=CurveColor3) Color curve_color_3_;
    private: NOZ_PROPERTY (Name=CurveColor4) Color curve_color_4_;
    private: NOZ_PROPERTY (Name=GridColor) Color grid_color_;
    private: NOZ_PROPERTY (Name=TimeMarkerColor) Color time_marker_color_;
    private: NOZ_PROPERTY (Name=KeyFrameSprite) ObjectPtr<Sprite> key_frame_sprite_;
    private: NOZ_PROPERTY (Name=SelectedKeyFrameColor) Color selected_key_frame_color_;
    private: NOZ_PROPERTY (Name=SelectedKeyFrameSprite) ObjectPtr<Sprite> selected_key_frame_sprite_;
    private: NOZ_PROPERTY (Name=HandleColor) Color handle_color_;
    private: NOZ_PROPERTY (Name=HandleSprite) ObjectPtr<Sprite> handle_sprite_;

    private: AnimationView* animation_view_;

    private: noz_float zoom_;

    private: noz_float value_min_;
    private: noz_float value_max_;
    private: noz_float value_scale_;
    private: noz_float value_rect_height_;
    private: noz_float time_offset_;
    private: noz_float grid_spacing_;
    private: noz_float time_;

    private: RenderMesh key_frame_mesh_;
    private: RenderMesh selected_key_frame_mesh_;
    private: RenderMesh handle_mesh_;

    private: std::map<KeyFrame*, CurveKey> keys_;

    private: DragMode drag_mode_;
    private: CurveKey* drag_key_;

    public: AnimationCurveEditor (void);

    public: ~AnimationCurveEditor (void);

    public: void SetZoom (noz_float zoom);
    public: void SetTimeOffset (noz_float o);
    public: void SetGridSpacing (noz_float s);
    public: void SetTime (noz_float t);

    public: void UpdateTracks (void);

    protected: virtual void OnMouseDown (SystemEvent* e) override;    
    protected: virtual void OnMouseOver (SystemEvent* e) override;    
    protected: virtual void OnMouseUp (SystemEvent* e) override;    
    protected: virtual void RenderOverride (RenderContext* rc) override;
    protected: virtual void OnLineageChanged (void) override;
    protected: virtual Vector2 MeasureChildren (const Vector2& a) override;
    protected: virtual void ArrangeChildren (const Rect& r) override;

    private: void UpdateValueRange (void);
    private: noz_float DrawCurve (RenderContext* rc, noz_float offset, KeyFrame* kf1, KeyFrame* kf2, Color color);
    private: Vector2 GetKeyFramePosition (const Rect& r, KeyFrame* kf) const;
    private: Vector2 GetKeyFramePosition (const Rect& r, noz_float t, noz_float v) const;
    private: Vector2 GetKeyFramePosition (const Rect& r, const KeyFrame* kf1, const KeyFrame* kf2, noz_float t) const;

    private: CurveKey* HitTestKey (const Vector2& pt);
    private: CurveKey* GetKey (AnimationViewTrack* av_track, noz_uint32 kf_index);

    private: void AddKey (AnimationViewTrack* av_track, noz_uint32 kf_index);

    private: void UpdateMeshes (void);
    private: void UpdateKeyMesh (void);
    private: void UpdateSelectionMesh (void);
    private: void UpdateHandleMesh (void);

    private: void InvalidateRender (void);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AnimationCurveEditor_h__

