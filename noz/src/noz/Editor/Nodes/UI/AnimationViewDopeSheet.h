///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AnimationViewDopeSheet_h__
#define __noz_Editor_AnimationViewDopeSheet_h__

namespace noz {
namespace Editor {

  class AnimationView;
  class AnimationViewTrack;

  class AnimationViewDopeSheet : public UINode {
    NOZ_OBJECT()

    friend class AnimationView;

    private: NOZ_PROPERTY (Name=TrackHeight) noz_float track_height_;
    private: NOZ_PROPERTY (Name=KeySprite) ObjectPtr<Sprite> key_sprite_;
    private: NOZ_PROPERTY (Name=KeyColor) Color key_color_;
    private: NOZ_PROPERTY (Name=SelectedKeySprite) ObjectPtr<Sprite> selected_key_sprite_;
    private: NOZ_PROPERTY (Name=SelectedKeyColor) Color selected_key_color_;
    private: NOZ_PROPERTY (Name=GridColor) Color grid_color_;
    private: NOZ_PROPERTY (Name=CurrentTimeColor) Color current_time_color_;
    private: NOZ_PROPERTY (Name=TrackColor) Color track_color_;

    /// Parent animation view..
    private: AnimationView* animation_view_;

    private: RenderMesh tracks_mesh_;

    /// Mesh used to render key frames
    private: RenderMesh keys_mesh_;

    /// Mesh used to render selected keys frames
    private: RenderMesh selected_keys_mesh_;

    public: AnimationViewDopeSheet (void);

    public: ~AnimationViewDopeSheet (void);

    public: AnimationView* GetAnimationView(void) const {return animation_view_;}

    protected: virtual void ArrangeChildren (const Rect& r) override;
    protected: virtual Vector2 MeasureChildren (const Vector2& a) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void RenderOverride (RenderContext* rc) override;
    protected: virtual void OnLineageChanged (void) override;

    private: void InvalidateRender (void);
    private: void InvalidateSelectionRender (void);

    private: void RenderKey (const Vector2& zp, noz_float kf_time, noz_int32 track_render_index, bool selected);

    private: void OnTrackSelected (AnimationViewTrack* av_track);
    
    private: Rect GetKeyFrameRect (noz_float kf_time, noz_int32 track_render_index) const;
    private: Rect GetKeyFrameRect (const Vector2& zp, noz_float kf_time, noz_int32 track_render_index) const;
    private: Vector2 GetZeroPoint (void);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AnimationViewDopeSheet_h__

