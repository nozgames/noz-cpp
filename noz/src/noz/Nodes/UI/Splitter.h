///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Splitter_h__
#define __noz_Splitter_h__

namespace noz {

  class Thumb;

  class Splitter : public UINode {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Split, Set=SetSplit)
    private: noz_float split_;

    NOZ_PROPERTY(Name=SplitAlignment, Set=SetSplitAlignment)
    private: Alignment split_align_;

    NOZ_PROPERTY(Name=Orientation,Set=SetOrientation)
    private: Orientation orientation_;

    NOZ_PROPERTY(Name=ThumbSize, Set=SetThumbSize)
    private: noz_float thumb_size_;

    private: noz_float drag_split_;

    /// Cached pointer to the thumb that controls the splitter.
    private: ObjectPtr<Thumb> thumb_;

    /// Default constructor
    public: Splitter (void);

    public: void SetSplit (noz_float split);

    public: void SetSplitAlignment (Alignment align);

    public: void SetThumbSize (noz_float thumb_size);

    public: void SetOrientation (Orientation orientation);

    public: noz_float GetSplit (void) const {return split_;}

    public: Alignment GetSplitAlignment (void) const {return split_align_;}

    public: noz_float GetThumbSize (void) const {return thumb_size_;}

    /// Return the thumb that controls the splitter
    public: Thumb* GetThumb (void) const {return thumb_;}

    public: Orientation GetOrientation (void) const {return orientation_;}

    protected: void OnThumbDragStarted (UINode*);

    protected: void OnThumbDragDelta (DragDeltaEventArgs* args);

    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnChildRemoved (Node* child) override;
    protected: virtual void ArrangeChildren (const Rect& r) override;    
    protected: virtual Vector2 MeasureChildren (const Vector2& available_size) override;
  };

} // namespace noz


#endif //__noz_Splitter_h__

