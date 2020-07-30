///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Graphics_Font_h__
#define __noz_Graphics_Font_h__

namespace noz {

  class Image;
  class ImageSheet;
  
  namespace Editor { class FontFile; }
  
  class Font : public Asset {
    NOZ_OBJECT(Managed,EditorIcon="{1A836100-9CF6-42E6-972F-46AD780ECD9A}")

    friend class Editor::FontFile;

    public: class Glyph : public Object {
      NOZ_OBJECT()

      public: NOZ_PROPERTY(Name=AdvanceX) noz_byte ax;
      public: NOZ_PROPERTY(Name=OffsetX) noz_int32 ox;
      public: NOZ_PROPERTY(Name=OffsetY) noz_int32 oy;
      public: NOZ_PROPERTY(Name=W) noz_byte w;
      public: NOZ_PROPERTY(Name=H) noz_byte h;
      public: NOZ_PROPERTY(Name=S) Vector2 s;
      public: NOZ_PROPERTY(Name=T) Vector2 t;

      public: Glyph(void) {ax=0;ox=0;oy=0;w=0;h=0;}
    };

    public: class Kerning : public Object {
      NOZ_OBJECT()

      public: NOZ_PROPERTY(Name=Code) noz_uint32 code;
      public: NOZ_PROPERTY(Name=X) noz_float x;
    };

    NOZ_PROPERTY(Name=Ascent)
    protected: noz_float ascent_;

    NOZ_PROPERTY(Name=Descent)
    protected: noz_float descent_;

    NOZ_PROPERTY(Name=Height)
    protected: noz_float height_;

    NOZ_PROPERTY(Name=LineSpacing)
    protected: noz_float line_spacing_;

    NOZ_PROPERTY(Name=Glyphs)
    protected: std::vector<Glyph> glyphs_;
    
    NOZ_PROPERTY(Name=GlyphMin)
    protected: noz_uint32 glyph_min_;

    NOZ_PROPERTY(Name=GlyphMax)
    protected: noz_uint32 glyph_max_;

    NOZ_PROPERTY(Name=Image)
    protected: ObjectPtr<Image> image_;

    NOZ_PROPERTY(Name=Kerning)
    protected: std::vector<Kerning> kerning_;

    NOZ_PROPERTY(Name=DistanceField)
    protected: bool distance_field_;

    protected: std::map<noz_uint32,Kerning*> kerning_map_;

    public: Font(void);

    public: ~Font(void);

    /// Return the height of the font in pixels
    public: noz_float GetHeight(void) const {return height_;}

    /// Return the descent of the font in pixels
    public: noz_float GetDescent(void) const {return descent_;}

    /// Return the ascent of the font in pixels
    public: noz_float GetAscent(void) const {return ascent_;}

    /// Return the line spacing (Note this is the spacing included in the height)
    public: noz_float GetLineSpacing(void) const {return line_spacing_;}

    /// Return the glyph representing the given character
    public: const Glyph* GetGlyph(char c) const {
      if((noz_byte)c<glyph_min_||(noz_byte)c>glyph_max_) return nullptr;
      return &glyphs_[c-glyph_min_];
    }

    /// Return the advance on the x-axis between character 'c' and character 'n'
    public: noz_float GetAdvanceX (char c, char n=0);

    /// Return the image used by the font.
    public: Image* GetImage(void) const {return image_;}

    /// Return true if the font is a distance field font.
    public: bool IsDistanceField(void) const {return distance_field_;}

    /// Measure the given text in pixels
    public: void MeasureText (const String& txt, noz_float& cx, noz_float& cy, noz_float scale=1.0f) {MeasureText(txt.ToCString(),txt.GetLength(),cx,cy,scale);}

    /// Measure the given text in pixels
    public: void MeasureText (const char* txt, noz_int32 len, noz_float& cx, noz_float& cy, noz_float scale=1.0f);

    protected: void UpdateKerningMap(void);

    protected: static noz_uint32 MakeKerningCode(noz_uint16 c, noz_uint16 n) {return (((noz_uint32)c)<<16) + n;}


    public: static std::vector<String> GetFontFamilies (void);

    public: static std::vector<String> GetFontStyles (const char* family);

    public: static bool ReadFontData (const char* family, const char* style, noz_uint32 size, Stream* stream);

    public: static Font* GetDefaultFont (void);
  };

} // namespace noz

#endif // __noz_Graphics_Font_h__

