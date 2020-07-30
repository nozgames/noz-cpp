///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TextRenderer_h__
#define __noz_TextRenderer_h__

#include "RenderNode.h"

namespace noz {

  NOZ_ENUM() enum class TextOverflowHorizontal {
    Wrap,
    Clip,
    Overflow,
    Ellipsis
  };

  NOZ_ENUM() enum class TextOverflowVertical {
    Clip,
    Overflow
  };

  class TextNode : public RenderNode {
    NOZ_OBJECT(EditorName="Text",EditorIcon="{4954905B-DBDF-4B7C-BEF5-D4AFF7C637B3}")

    /// Class that represents a single line of text
    protected: struct Line {
      /// Line bounds
      Rect bounds_;

      /// Minimum character index for the characters on the line (-1 if none)
      noz_uint32 char_min_;

      /// Maximum character index for the characters on the line (-1 if none)
      noz_uint32 char_max_;
    };

    /// Class which represents a single character within the text
    protected: struct Character {
      /// Font glyph to render the character with
      const Font::Glyph* glyph_;

      /// Index of the line the character is on
      noz_int32 line_;

      /// Bounds of the character in local coordinates
      Rect bounds_;

      /// Character index within the text
      noz_int32 char_index_;
    };

    /// Font to render text with
    NOZ_PROPERTY(Name=Font,Set=SetFont)
    private: ObjectPtr<Font> font_;

    /// Text to render
    NOZ_PROPERTY(Name=Text,Set=SetText)
    private: String text_;

    /// Color to render text 
    NOZ_PROPERTY(Name=Color,Set=SetColor)
    private: Color color_;

    NOZ_PROPERTY(Name=HorizontalAlignment,Set=SetHorizontalAlignment)
    private: Alignment horizontal_alignment_;

    NOZ_PROPERTY(Name=VerticalAlignment,Set=SetVerticalAlignment)
    private: Alignment vertical_alignment_;

    NOZ_PROPERTY(Name=HorizontalOverflow,Set=SetHorizontalOverflow)
    private: TextOverflowHorizontal horizontal_overflow_;

    NOZ_PROPERTY(Name=VerticalOverflow,Set=SetVerticalOverflow)
    private: TextOverflowVertical vertical_overflow_;

    protected: std::vector<Character> characters_;

    protected: std::vector<Line> lines_;

    /// Cached measure size of the text
    protected: Vector2 measure_size_;

    /// True if the measure size is dirty
    protected: bool measure_dirty_;

    protected: RenderMesh mesh_;

    /// Default constructor
    public: TextNode(void);

    /// Destructor
    public: ~TextNode (void);

    /// Set the font to render the text with
    public: void SetFont(Font* font);

    /// Set the text to render
    public: void SetText(const char* text);
    public: void SetText(const String& text) {SetText(text.ToCString());}

    /// Set the horizontal alignment of the text within the node rectangle
    public: void SetHorizontalAlignment(Alignment align);

    /// Set the vertical alignment of the text within the node rectangle
    public: void SetVerticalAlignment(Alignment align);

    public: void SetHorizontalOverflow (TextOverflowHorizontal overflow);

    public: void SetVerticalOverflow (TextOverflowVertical overflow);

    public: void SetColor (Color color);

    public: Font* GetFont (void) const {return font_;}

    public: Color GetColor (void) const {return color_;}

    /// Return the text being rendered
    public: const String& GetText(void) const {return text_;}

    /// Get the character position for the given world coordinate
    public: noz_int32 GetCharIndexFromPosition(const Vector2& pos);

    /// Retrieves the local position of the character at the given index
    public: Vector2 GetPositionFromCharIndex(noz_int32 index);

    /// Get the line index from the given character index
    public: noz_int32 GetLineFromCharIndex (noz_int32 index) const {return characters_[index].line_;}

    /// Get the last character in the given line
    public: noz_int32 GetLastCharIndexFromLine (noz_int32 line) const {return lines_[line].char_max_;}

    protected: virtual void OnSetText (void) {}

    /// Measure the size of the render content
    protected: virtual bool DrawMesh (RenderContext* rc) override;
    protected: virtual Vector2 MeasureMesh (const Vector2& a) override;
    protected: virtual void UpdateMesh (const Rect& r) override;
  };

} // namespace noz


#endif // __noz_TextRenderer_h__


