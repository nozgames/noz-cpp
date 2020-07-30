///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_EditableTextNode_h__
#define __noz_EditableTextNode_h__

#include "TextNode.h"

namespace noz {

  class EditableTextNode : public TextNode {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=SelectionColor,Set=SetSelectionColor)
    private: Color selection_color_;

    NOZ_PROPERTY(Name=CaretColor,Set=SetCaretColor)
    private: Color caret_color_;

    private: noz_int32 selection_start_;

    private: noz_int32 selection_end_;

    private: RenderMesh selection_mesh_;

    public: EditableTextNode(void);

    public: ~EditableTextNode(void);
    
    public: String GetSelectedText (void) const;

    public: noz_int32 GetSelectionStart(void) const {return selection_start_;}
    public: noz_int32 GetSelectionEnd(void) const {return selection_end_;}

    public: noz_int32 GetSelectionMin(void) const {return Math::Min(selection_start_,selection_end_);}
    public: noz_int32 GetSelectionMax(void) const {
      if(selection_start_ > selection_end_) return selection_start_-1;
      return selection_end_-1;
    }

    public: Vector2 GetPositionFromCaret (void);

    public: bool HasSelection(void) const {return selection_start_ != selection_end_;}

    /// Replace the current selection with the given text
    public: void ReplaceText (const char* text);
    public: void ReplaceText (char c);

    public: void ClearSelection (void);

    public: void Select (noz_int32 sel);
    public: void Select (noz_int32 ss, noz_int32 se);
    public: void SelectAll (void) {Select(0,GetText().GetLength());}
    public: void SetSelectionStart(noz_int32 sel);
    public: void SetSelectionEnd(noz_int32 len);
    public: void SetSelectionColor(Color col);
    public: void SetCaretColor (Color col);

    protected: virtual bool DrawMesh (RenderContext* rc) override;

    protected: virtual void UpdateMesh (const Rect& r) override;

    protected: virtual Vector2 MeasureMesh (const Vector2& a) override;

    private: void UpdateSelectionRenderState (void);

    protected: virtual void OnSetText (void) override;
  };

} // namespace noz


#endif // __noz_EditableTextNode_h__


