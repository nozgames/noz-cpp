///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_FontFile_h__
#define __noz_Editor_FontFile_h__

#include "AssetFile.h"

namespace noz {
namespace Editor {

  class FontDef : public Object {
    NOZ_OBJECT()

    public: FontDef(void) {sdf = false; sdf_scale = 0; sdf_scan = 4; height=16;}
    public: FontDef(noz_int32 _height) {sdf = false; height=_height;sdf_scale=0;sdf_scan=0;}
    public: FontDef(noz_int32 _height, noz_int32 _sdf_scale, noz_int32 _sdf_scan) {sdf = false; height=_height;sdf_scale=_sdf_scale;sdf_scan=_sdf_scan;}

    /// Height of the font in pixels
    NOZ_PROPERTY(Name=Height)
    public: noz_int32 height;

    /// Scale to render signed distance field glyphs at (zero for non-distance field fonts)
    NOZ_PROPERTY(Name=SDFScale)
    noz_int32 sdf_scale;

    /// Maximum number of pixels to scan for distance field data.
    NOZ_PROPERTY(Name=SDFScan)
    noz_int32 sdf_scan;

    /// Maximum number of pixels to scan for distance field data.
    NOZ_PROPERTY(Name=SDF)
    bool sdf;

    NOZ_PROPERTY(Name=Family)
    String family;

    NOZ_PROPERTY(Name=Style)
    String style;

    /// Characters to include in font.  If empty the font will include the standard 
    /// ASCII character set.
    NOZ_PROPERTY(Name=Chars)
    String chars;

    // TODO: FX
    // TODO:   - bold
    // TODO:   - ittalic
    // TODO:   - underline
    // TODO:   - dropshadow
  };

  class FontFile : public AssetFile {
    NOZ_OBJECT(EditorFileExt=nozfont, EditorAssetType=noz::Font)

    public: virtual Asset* Import (void) override;

    public: virtual bool Reimport (Asset* asset) override;

    public: static bool Import (const String& file_path, const FontDef& def, Font* font);
    
    public: virtual bool CreateEmpty (void) override;

    protected: virtual void OnDeleted (void) override;

    protected: virtual void OnMoved (const String& from, const String& to) override;
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_FontFile_h__

