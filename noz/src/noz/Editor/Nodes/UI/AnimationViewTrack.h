///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AnimationViewTrack_h__
#define __noz_Editor_AnimationViewTrack_h__

#include <noz/Nodes/UI/ContentControl.h>
#include <noz/Nodes/UI/ToggleButton.h>
#include <noz/Nodes/UI/Button.h>

namespace noz {
namespace Editor {

  class AnimationViewKeyFrame;

  class AnimationViewTrack : public ContentControl {
    NOZ_OBJECT(DefaultStyle="{2ED65769-B30A-4D2F-A559-6C88562E2B40}")

    friend class AnimationView;
    friend class AnimationViewKeyFrame;

    private: NOZ_CONTROL_PART(Name=TrackContainer) ObjectPtr<Node> tracks_container_;
    private: NOZ_CONTROL_PART(Name=ExpandButton) ObjectPtr<ToggleButton> expand_button_;
    private: NOZ_CONTROL_PART(Name=OptionsButton) ObjectPtr<Button> options_button_;

    private: noz_int32 render_index_;

    /// Animation target the track is associated with.
    private: AnimationTarget* animation_target_;

    /// Animator target associated with the track.
    private: AnimatorTarget* animator_target_;

    /// Animation track that the channel represents.. Can be nullptr if the
    /// track represents a multi-channel track.
    private: AnimationTrack* animation_track_;

    private: AnimationViewTrack* parent_track_;

    private: std::vector<AnimationViewTrack*> child_tracks_;

    private: std::set<noz_uint32> selected_key_frames_;

    private: bool expanded_;

    private: bool selected_;

    private: noz_uint32 track_id_;

    public: AnimationViewTrack (AnimationTarget* animation_target, AnimatorTarget* animator_target, AnimationTrack* animation_track);

    public: AnimationViewTrack (AnimationTarget* animation_target, AnimatorTarget* animator_target, noz_uint32 track_id);

    public: ~AnimationViewTrack(void);

    public: AnimationTrack* GetAnimationTrack(void) const {return animation_track_;}

    public: AnimationTarget* GetAnimationTarget(void) const {return animation_target_;}

    public: AnimatorTarget* GetAnimatorTarget(void) const {return animator_target_;}

    public: noz_uint32 GetTrackId (void) const {return track_id_;}

    public: void SetAnimationTrack(AnimationTrack* t);

    public: void SetAnimatorTarget(AnimatorTarget* t);

    public: noz_int32 GetRenderIndex (void) const {return render_index_;}

    public: void SetExpanded (bool v);

    public: bool IsExpanded (void) const {return expanded_;}
    public: bool IsSelected (void) const {return selected_;}
    public: bool IsParentTrack (void) const {return nullptr == parent_track_;}

    public: AnimationViewTrack* GetParentTrack (void) const {return parent_track_;}

    public: AnimationViewTrack* GetChildTrack (noz_uint32 i) const {return child_tracks_[i];}

    public: noz_uint32 GetChildTrackCount (void) const {return child_tracks_.size(); }

    public: noz_uint32 GetKeyFrameCount (void) const {return animation_track_ ? animation_track_->GetKeyFrameCount() : 0;}

    public: KeyFrame* GetKeyFrame (noz_uint32 i) const {return animation_track_->GetKeyFrame(i);}

    protected: virtual bool OnApplyStyle(void) override;
    protected: virtual void OnStyleChanged (void) override;
    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void UpdateAnimationState (void) override;
    protected: virtual void OnMouseDown(SystemEvent* e) override;

    private: void OnExpandButtonClick (UINode*);
    private: void OnOptionsButtonClick (UINode*);

    private: void Refresh (void);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AnimationViewTrackItem_h__

