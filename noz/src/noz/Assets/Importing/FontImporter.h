///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Importers_FontImporter_h__
#define __noz_Importers_FontImporter_h__

#include <noz/Assets/Importing/AssetImporter.h>

namespace noz {

  class FontMeta : public Object {
    NOZ_OBJECT()

    FontMeta(void) {sdf = false; sdf_scale = 0; sdf_scan = 4; height=16;}
    FontMeta(noz_int32 _height) {sdf = false; height=_height;sdf_scale=0;sdf_scan=0;}
    FontMeta(noz_int32 _height, noz_int32 _sdf_scale, noz_int32 _sdf_scan) {sdf = false; height=_height;sdf_scale=_sdf_scale;sdf_scan=_sdf_scan;}

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

    /// Source of found data, if empty uses name of nozml file
    NOZ_PROPERTY(Name=Source)
    String source;

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

  class FontImporter : public AssetImporter {
    NOZ_OBJECT(ImportEXT=nozfont, ImportType=Font)

    public: virtual Asset* Import (const String& path) override;

    private: Font* Import (Stream* ttf, const FontMeta& meta);
  };

} // namespace noz

#endif // __noz_Importers_FontImporter_h__

