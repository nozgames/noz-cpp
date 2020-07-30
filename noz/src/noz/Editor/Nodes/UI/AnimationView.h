///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AnimationView_h__
#define __noz_Editor_AnimationView_h__

#include <noz/Animation/EventKeyFrame.h>
#include <noz/Nodes/UI/DropDownList.h>
#include <noz/Nodes/UI/TreeView.h>
#include <noz/Nodes/UI/ScrollBar.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/ObjectPtrPropertyEditor.h>

#include "AnimationViewTrack.h"
#include "AnimationViewDopeSheet.h"
#include "AnimationTimeline.h"
#include "AnimationCurveEditor.h"

namespace noz { class TreeView; class ListView; class ToggleButton; }

namespace noz {
namespace Editor {

  class GridNode;
  class SetKeyFrameAction;
  class SetAnimatorTargetAction;
  class AddAnimationTargetAction;
  class SetKeyFrameTangentsAction;
  class RemoveKeyFrameAction;

  class AnimationView : public Control {
    NOZ_OBJECT(DefaultStyle="{6E0F0A15-CE89-40A2-8BD9-25D2E0A6BC97}")

    friend class AnimationViewKeyFrame;
    friend class AnimationViewDopeSheet;
    friend class AnimationCurveEditor;
    friend class AnimationViewTrack;

    private: struct SelectedKeyFrame {
      AnimationViewTrack* av_track_;
      noz_uint32 kf_index_;
      KeyFrame* kf_;
    };

    private: NOZ_CONTROL_PART(Name=AddEventButton) ObjectPtr<Button> add_event_button_;
    private: NOZ_CONTROL_PART(Name=AddFrameButton) ObjectPtr<Button> add_frame_button_;
    private: NOZ_CONTROL_PART(Name=PlayButton) ObjectPtr<ToggleButton> play_button_;
    private: NOZ_CONTROL_PART(Name=RecordButton) ObjectPtr<ToggleButton> record_button_;
    private: NOZ_CONTROL_PART(Name=NextFrameButton) ObjectPtr<Button> next_frame_;
    private: NOZ_CONTROL_PART(Name=PrevFrameButton) ObjectPtr<Button> prev_frame_;
    private: NOZ_CONTROL_PART(Name=FirstFrameButton) ObjectPtr<Button> first_frame_;
    private: NOZ_CONTROL_PART(Name=LastFrameButton) ObjectPtr<Button> last_frame_;
    private: NOZ_CONTROL_PART(Name=TracksContainer) ObjectPtr<TreeView> tracks_container_;
    private: NOZ_CONTROL_PART(Name=DropDownList) ObjectPtr<DropDownList> animation_list_;
    private: NOZ_CONTROL_PART(Name=SourceName) ObjectPtr<TextNode> source_name_;
    private: NOZ_CONTROL_PART(Name=Timeline) ObjectPtr<AnimationTimeline> timeline_;
    private: NOZ_CONTROL_PART(Name=DopeSheet) ObjectPtr<AnimationViewDopeSheet> dope_sheet_;
    private: NOZ_CONTROL_PART(Name=DopeSheetScrollBar) ObjectPtr<ScrollBar> dope_sheet_scrollbar_;
    private: NOZ_CONTROL_PART(Name=TrackOptionsPopup) ObjectPtr<Popup> track_options_popup_;
    private: NOZ_CONTROL_PART(Name=LinkTrackMenuItem) ObjectPtr<Button> link_track_menu_item_;
    private: NOZ_CONTROL_PART(Name=UnlinkTrackMenuItem) ObjectPtr<Button> unlink_track_menu_item_;
    private: NOZ_CONTROL_PART(Name=DeleteTrackMenuItem) ObjectPtr<Button> delete_track_menu_item_;
    private: NOZ_CONTROL_PART(Name=EditEventPopup) ObjectPtr<Popup> edit_event_popup_;
    private: NOZ_CONTROL_PART(Name=EventMethod) ObjectPtr<DropDownList> event_method_;
    private: NOZ_CONTROL_PART(Name=EventAnimationState) ObjectPtr<DropDownList> event_animation_state_;
    private: NOZ_CONTROL_PART(Name=EventAudioClip) ObjectPtr<ObjectPtrPropertyEditor> event_audio_clip_;
    private: NOZ_CONTROL_PART(Name=DopeSheetButton) ObjectPtr<ToggleButton> dope_sheet_button_;
    private: NOZ_CONTROL_PART(Name=CurveEditorButton) ObjectPtr<ToggleButton> curve_editor_button_;
    private: NOZ_CONTROL_PART(Name=CurveEditor) ObjectPtr<AnimationCurveEditor> curve_editor_;
    private: NOZ_CONTROL_PART(Name=KeyFramePopup) ObjectPtr<Popup> key_frame_popup_;
    private: NOZ_CONTROL_PART(Name=DeleteKeyFrameMenuItem) ObjectPtr<Button> delete_key_frame_menu_item_;

    private: NOZ_CONTROL_PART(Name=TimelineScrollbar) ObjectPtr<ScrollBar> timeline_scrollbar_;

    private: ObjectPtr<Animator> source_;

    private: std::set<Animation*> modified_animations_;

    /// Animator used to to animate the current animation
    private: Animator* animator_;

    /// Currently selected animation
    private: Animation* animation_;

    private: noz_float zoom_;

    private: noz_float drag_start_;

    private: noz_float drag_delta_;

    private: noz_float drag_min_;

    private: noz_float drag_max_;

    private: noz_uint32 visible_track_count_;

    private: std::vector<SelectedKeyFrame> selected_key_frames_;

    private: AnimationViewKeyFrame* edit_event_key_;

    private: AnimationViewTrack* options_track_;

    /// Vector of all selected tracks.
    private: std::vector<AnimationViewTrack*> selected_tracks_;

    public: AnimationView (void);

    public: ~AnimationView (void);

    public: void SetSource (Animator* animator);

    public: void SetTime (noz_float time, bool force=false);

    public: void Refresh (void);

    public: bool IsRecording (void) const;

    public: bool IsPlaying (void) const;

    public: bool IsDopeSheetActive (void) const {return dope_sheet_->GetVisibility() == Visibility::Visible;}
    public: bool IsCurveEditorActive (void) const {return curve_editor_->GetVisibility() == Visibility::Visible;}

    public: void BeginRecording (void);

    public: void RefreshAnimationFrame (void);

    public: void Save (void);

    /// Return the current animation
    public: Animation* GetAnimation (void) const;

    public: Animator* GetSource (void) const {return source_;}

    public: noz_float GetTime (void) const {return timeline_->GetTime();}

    public: noz_uint32 GetVisibleTrackCount (void) const {return visible_track_count_;}

    //public: void Link (AnimationViewTargetItem* av_target);

    public: static AnimationView* GetAnimationView(Node* descendant);

    public: noz_float GetZoom (void) const {return zoom_;}
    public: noz_float GetPixelsPerSecond (void) const {return timeline_->GetPixelsPerSecond();}
    public: noz_float GetLabelInterval (void) const {return timeline_->GetLabelInterval();}
    public: AnimationTimeline* GetTimeline (void) const {return timeline_;}
    public: noz_uint32 GetTrackCount (void) const {return tracks_container_->GetLogicalChildCount();}
    public: AnimationViewTrack* GetTrack (noz_uint32 i) const {return (AnimationViewTrack*)tracks_container_->GetLogicalChild(i);}
    public: AnimationViewTrack* GetSelectedTrack (noz_uint32 i) const {return selected_tracks_[i];}
    public: noz_uint32 GetSelectedTrackCount (void) const {return selected_tracks_.size();}
    public: noz_uint32 GetSelectedKeyFrameCount (void) const {return selected_key_frames_.size();}
    public: const SelectedKeyFrame& GetSelectedKeyFrame(noz_uint32 i) const {return selected_key_frames_[i];}

    public: bool ExecuteAction (Action* action);

    private: void RefreshAnimation (void);

    private: void SetAnimationModified (void);

    //private: AnimationViewItem* AddEventTrack (AnimationTrack* track);

    private: AnimationViewTrack* AddTrack (AnimationTarget* animation_target);
    private: AnimationViewTrack* FindTrack (AnimationTarget* animation_target);
    private: AnimationViewTrack* FindTrack (AnimatorTarget* animator_target);
    private: AnimationViewTrack* FindTrack (AnimationTarget* animation_target, noz_uint32 track_id);
    private: void RemoveTrack (AnimationTarget* animation_target);

    private: void AddKeyFrames (AnimationViewTrack* av_track);
    private: AnimationViewKeyFrame* AddKeyFrame (AnimationViewTrack* av_track, KeyFrame* kf);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnKeyDown(SystemEvent* e) override;
    protected: virtual void OnMouseWheel(SystemEvent* e) override;
    protected: virtual void Animate (void) override;
    protected: virtual void ArrangeChildren (const Rect& r) override;

    protected: void OnMouseDown (SystemEvent* e, AnimationViewDopeSheet* dope_sheet);
    protected: void OnMouseDown (SystemEvent* e, AnimationViewKeyFrame* av_key);
    protected: void OnMouseDown (SystemEvent* e, AnimationViewTrack* av_track);
    protected: void OnMouseUp (SystemEvent* e, AnimationViewKeyFrame* key);
    protected: void OnMouseOver (SystemEvent* e, AnimationViewKeyFrame* key);

    private: void UpdateAnimatorState (void);
    private: void UpdateZoom (void);
    private: void UpdateTrackIndicies (void);
    private: void UpdateDopeSheetVisibility (void);
    private: void UpdateTimelineScrollBar (void);

    private: void RefreshTracks (void);
    private: void CleanAnimation (Animation* animation);
    private: void CleanAnimator (Animator* animator);
    private: void SetZoom (noz_float zoom);
    private: void UnselectAllTracks (void);
    private: void UnselectTrack (AnimationViewTrack* av_track);
    private: void SelectTrack (AnimationViewTrack* av_track);
    private: void SelectKeyFrame (AnimationViewTrack* av_track, noz_uint32 kf_index);
    private: void UnselectKeyFrame (AnimationViewTrack* av_track, noz_uint32 kf_index);
    private: void UnselectAllKeyFrames (void);
    private: void RemoveSelectedKeyFrames (void);
    private: bool IsKeyFrameSelected (AnimationViewTrack* av_track, noz_uint32 kf_index) const;
    private: void SetTrackSelected (AnimationViewTrack* av_track, bool selected);
    private: void BeginEditEvent (AnimationViewKeyFrame* av_key);
    private: void SetLayoutTransformWidth (Node* n, noz_float w);
    private: void SetLayoutTransformHeight (Node* n, noz_float h);
    private: void GetMethods (Object* o, std::set<Method*>& methods);

    private: void OnAnimationListSelectionChanged (UINode* sender);
    private: void OnPlayButtonClicked (UINode* sender);
    private: void OnRecordButtonClicked (UINode* sender);
    private: void OnEventStateChanged (UINode* sender);
    private: void OnEventMethodChanged (UINode* sender);    
    private: void OnFirstFrame (UINode*);
    private: void OnLastFrame (UINode*);
    private: void OnNextFrame (UINode*);
    private: void OnPrevFrame (UINode*);
    private: void OnAddEvent (UINode*);
    private: void OnAddFrame (UINode*);
    private: void OnPropertyChanged (PropertyChangedEventArgs* args);
    private: void OnNamedPropertyChanged (Node* n);
    private: void OnOptionsButton (AnimationViewTrack* av_track);
    private: void OnLinkTrackMenuItem (UINode*);
    private: void OnUnlinkTrackMenuItem (UINode*);
    private: void OnDeleteTrackMenuItem (UINode*);
    private: void OnDeleteKeyFrameMenuItem (UINode*);
    private: void OnDopeSheetButton (UINode*);
    private: void OnCurveEditorButton (UINode*);
    private: void OnTimelineScroll (UINode*);
    private: void OnTimelineTimeChanged (UINode*);
    private: void OnTrackExpanded (AnimationViewTrack* av_track);

    private: void OpenKeyFramePopup (AnimationViewTrack* av_track, noz_uint32 kf_index, const Vector2& offset);

    public: void OnAction (Action* action, bool undo);
    public: void OnAction (SetKeyFrameAction* action, bool undo);
    public: void OnAction (SetAnimatorTargetAction* action, bool undo);
    public: void OnAction (AddAnimationTargetAction* action, bool undo);
    public: void OnAction (SetKeyFrameTangentsAction* action, bool undo);    
    public: void OnAction (RemoveKeyFrameAction* action, bool undo);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AnimationView_h__

