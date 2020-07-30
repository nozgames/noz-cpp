///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/Stream.h>
#include <noz/Render/BinPacker.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Render/Imaging/ResizeFilter.h>
#include "FontFile.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H
#include <wchar.h>

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
const struct {
    noz_int32          code;
    const char*  message;
} FT_Errors[] =
#include FT_ERRORS_H


using namespace noz;
using namespace noz::Editor;

noz_byte ComputeRadialSDF(noz_byte *fontmap,noz_int32 w, noz_int32 h,noz_int32 x, noz_int32 y,noz_int32 max_radius );


noz_byte SampleBilinear(Image* image, noz_int32 sx, noz_int32 sy, noz_int32 sw, noz_int32 sh) {
NOZ_FIXME()
#if 0
  // Calculate the ratio of the new image size to old image size
  noz_float tx = (noz_float)(sw-1)/(noz_float)image->GetWidth(); 
  noz_float ty = (noz_float)(sh-1)/(noz_float)image->GetHeight(); 

  noz_int32 x = (noz_int32)(tx * sx);
  noz_int32 y = (noz_int32)(ty * sy);
 
  noz_float x_diff = ((tx * sx) - x);
  noz_float y_diff = ((ty * sy) - y);
  
  noz_int32 a = y*image->GetStride() + x;
  noz_int32 b = a + 1;
  noz_int32 c = a + image->GetStride();
  noz_int32 d = a + image->GetStride() + 1;

  return (noz_byte)(
    image->GetBuffer()[a] * (1.0f-x_diff)*(1.0f-y_diff) +
    image->GetBuffer()[b] * (x_diff)*(1.0f-y_diff) +
    image->GetBuffer()[c] * (1.0f-x_diff)*(y_diff) +
    image->GetBuffer()[d] * (x_diff)*(y_diff)
  );
#else
  return 0;
#endif
}

struct Point
{
	int dx, dy;

	int DistSq() const { return dx*dx + dy*dy; }
};

struct Grid
{
  Grid(noz_int32 width, noz_int32 height) {
    width_ = width;
    height_ = height;
    grid = new Point[height*width];
  }
  noz_int32 width_;
  noz_int32 height_;
	Point* grid;
};

Point inside = { 0, 0 };
Point empty = { 9999, 9999 };

Point Get( Grid &g, int x, int y )
{
	// OPTIMIZATION: you can skip the edge check code if you make your grid 
	// have a 1-pixel gutter.
	if ( x >= 0 && y >= 0 && x < g.width_ && y < g.height_ )
		return g.grid[y*g.width_+x];
	else
		return empty;
}

void Put( Grid &g, int x, int y, const Point &p )
{
	g.grid[y*g.width_+x] = p;
}

void Compare( Grid &g, Point &p, int x, int y, int offsetx, int offsety )
{
	Point other = Get( g, x+offsetx, y+offsety );
	other.dx += offsetx;
	other.dy += offsety;

	if (other.DistSq() < p.DistSq())
		p = other;
}

void GenerateSDF( Grid &g )
{
	// Pass 0
	for (int y=0;y<g.height_;y++)
	{
		for (int x=0;x<g.width_;x++)
		{
			Point p = Get( g, x, y );
			Compare( g, p, x, y, -1,  0 );
			Compare( g, p, x, y,  0, -1 );
			Compare( g, p, x, y, -1, -1 );
			Compare( g, p, x, y,  1, -1 );
			Put( g, x, y, p );
		}

		for (int x=g.width_-1;x>=0;x--)
		{
			Point p = Get( g, x, y );
			Compare( g, p, x, y, 1, 0 );
			Put( g, x, y, p );
		}
	}

	// Pass 1
	for (int y=g.height_-1;y>=0;y--)
	{
		for (int x=g.width_-1;x>=0;x--)
		{
			Point p = Get( g, x, y );
			Compare( g, p, x, y,  1,  0 );
			Compare( g, p, x, y,  0,  1 );
			Compare( g, p, x, y, -1,  1 );
			Compare( g, p, x, y,  1,  1 );
			Put( g, x, y, p );
		}

		for (int x=0;x<g.width_;x++)
		{
			Point p = Get( g, x, y );
			Compare( g, p, x, y, -1, 0 );
			Put( g, x, y, p );
		}
	}
}








namespace noz {

  struct FT_Glyph {
    FT_Glyph(void) {index=0; offset_x=0; offset_y=0; ax=0;}

    /// Glyph index within FT_Face
    FT_UInt index;

    /// Rectangle of glyph within packed image
    BinPacker::BinRect packed;

    /// Offset of actual bitmap within the packed rectangle
    noz_int32 offset_x;
    noz_int32 offset_y;

    /// Advance on x-axis
    noz_int32 ax;
  };

  static bool FT_LoadFace(FT_Library library, FT_Face* face, noz_float size, noz_int32 sdf_scale, Stream* ttf) {
    FT_Error error;
    FT_Matrix matrix = { (noz_int32)((1.0/64) * 0x10000L),
                         (noz_int32)((0.0)    * 0x10000L),
                         (noz_int32)((0.0)    * 0x10000L),
                         (noz_int32)((1.0)    * 0x10000L) };

    noz_assert(library);
    noz_assert(face);
    noz_assert(size);

    // Load face into memory
    noz_uint32 ttfsize = ttf->GetLength();
    char* ttfdata = new char[ttfsize];
    ttf->Read(ttfdata,0,ttf->GetLength());
    error = FT_New_Memory_Face(library, (const FT_Byte*)ttfdata, ttfsize, 0, face);
    //delete[] ttfdata;

    if( error ) {
      Console::WriteLine("FT_Error: 0x%02x: %s", FT_Errors[error].code, FT_Errors[error].message);
      return false;
    }

    // Select charmap
    error = FT_Select_Charmap(*face, FT_ENCODING_UNICODE );
    if(error) {
      // fprintf( stderr, "FT_Error (line %d, code 0x%02x) : %s\n", __LINE__, FT_Errors[error].code, FT_Errors[error].message );
      FT_Done_Face(*face);
      return false;
    }

    // Set Char size
    error = FT_Set_Char_Size(*face, (noz_int32)(size*64*(sdf_scale==0?1:sdf_scale)), 0, 72, 72 );
    if(error) {
      ///fprintf( stderr, "FT_Error (line %d, code 0x%02x) : %s\n", __LINE__, FT_Errors[error].code, FT_Errors[error].message );
      FT_Done_Face(*face);
      return false;
    }

    // Set transform matrix
    //FT_Set_Transform(*face, &matrix, NULL);

    return true;
  }

  std::vector<FT_Glyph> FT_GetGlyphs(FT_Face face, const FontDef& def, wchar_t* codes, size_t numCodes, noz_int32 sdf_scale, noz_int32 sdf_scan) {
    std::vector<FT_Glyph> glyphs;
    glyphs.resize(numCodes);

    for(size_t g=0; g<numCodes; g++) {
      FT_Glyph& glyph = glyphs[g];
      
      glyph.index = FT_Get_Char_Index(face, codes[g]);

      FT_Load_Glyph(face, glyph.index, FT_LOAD_RENDER);

      if(face->glyph->bitmap.width && face->glyph->bitmap.rows) {
        glyph.packed.w = face->glyph->bitmap.width / sdf_scale + sdf_scan*2 + 4;
        glyph.packed.h = face->glyph->bitmap.rows / sdf_scale + sdf_scan*2 + 4;
        glyph.offset_x = face->glyph->bitmap_left / sdf_scale - sdf_scan;
        glyph.offset_y = face->glyph->bitmap_top / sdf_scale - sdf_scan;
      }
      glyph.ax = (face->glyph->advance.x / sdf_scale) >> 6;
    }

    return glyphs;
  }

  void FT_PackGlyphs(BinPacker& packer, std::vector<FT_Glyph>& glyphs) {
    for(noz_int32 size = 64; ; size=size<<1) {
      packer.Resize(size,size);
      
      size_t g;
      for(g=0; g<glyphs.size(); g++) {
        FT_Glyph& glyph = glyphs[g];
        if(glyph.packed.w == 0 || glyph.packed.h == 0) continue;

        BinPacker::BinRect rect = packer.Insert(BinPacker::BinSize(glyph.packed.w,glyph.packed.h),BinPacker::Method::BestLongSideFit);
        if(rect.h == 0) {
          break;
        }
        noz_assert(rect.w == glyph.packed.w && rect.h == glyph.packed.h);
        glyph.packed.x = rect.x;
        glyph.packed.y = rect.y;
      }

      if(g>=glyphs.size()) {
        break;
      }
    }
  }

  static noz_byte ComputeRadialSDF(noz_byte* fontmap,noz_int32 w, noz_int32 h,noz_int32 x, noz_int32 y,noz_int32 max_radius ) {
	  //	hideous brute force method
	  noz_double d2 = max_radius*max_radius+1.0;
	  noz_byte v = fontmap[x+y*w];
	  for( noz_int32 radius = 1; (radius <= max_radius) && (radius*radius < d2); ++radius ) {
		  noz_int32 line, lo, hi;
		  //	north
		  line = y - radius;
		  if ((line >= 0) && (line < h)) {
			  lo = x - radius;
			  hi = x + radius;
			  if( lo < 0 ) { lo = 0; }
			  if( hi >= w ) { hi = w-1; }
			  noz_int32 idx = line * w + lo;
			  for( noz_int32 i = lo; i <= hi; ++i ) {
				  //	check this pixel
				  if(fontmap[idx] != v) {
					  noz_double nx = i - x;
					  noz_double ny = line - y;
					  noz_double nd2 = nx*nx+ny*ny;
					  if (nd2 < d2) d2 = nd2;
				  }
				  //	move on
				  ++idx;
			  }
		  }
		  //	south
		  line = y + radius;
		  if ((line >= 0) && (line < h)) {
			  lo = x - radius;
			  hi = x + radius;
			  if( lo < 0 ) { lo = 0; }
			  if( hi >= w ) { hi = w-1; }
			  noz_int32 idx = line * w + lo;
			  for(noz_int32 i = lo; i <= hi; ++i) {
				  //	check this pixel
				  if( fontmap[idx] != v ) {
					  noz_double nx = i - x;
					  noz_double ny = line - y;
					  noz_double nd2 = nx*nx+ny*ny;
					  if( nd2 < d2 ) d2 = nd2;
				  }
				  //	move on
				  ++idx;
			  }
		  }
		  //	west
		  line = x - radius;
		  if ((line >= 0) && (line < w)) {
			  lo = y - radius + 1;
			  hi = y + radius - 1;
			  if( lo < 0 ) { lo = 0; }
			  if( hi >= h ) { hi = h-1; }
			  noz_int32 idx = lo * w + line;
			  for(noz_int32 i = lo; i <= hi; ++i) {
				  //	check this pixel
				  if(fontmap[idx] != v) {
					  noz_double nx = line - x;
					  noz_double ny = i - y;
					  noz_double nd2 = nx*nx+ny*ny;
					  if(nd2 < d2) d2 = nd2;
				  }
				  //	move on
				  idx += w;
			  }
		  }
		  //	east
		  line = x + radius;
		  if ((line >= 0) && (line < w)) {
			  lo = y - radius + 1;
			  hi = y + radius - 1;
			  if( lo < 0 ) { lo = 0; }
			  if( hi >= h ) { hi = h-1; }
			  noz_int32 idx = lo * w + line;
			  for (noz_int32 i = lo; i <= hi; ++i) {
				  //	check this pixel
				  if(fontmap[idx] != v) {
					  noz_double nx = line - x;
					  noz_double ny = i - y;
					  noz_double nd2 = nx*nx+ny*ny;
					  if( nd2 < d2 ) d2 = nd2;
				  }
				  //	move on
				  idx += w;
			  }
		  }
	  }
	  d2 = Math::Sqrt(d2);
	  if(v==0) d2 = -d2;
	  d2 *= 127.5 / max_radius;
	  d2 += 127.5;
	  if (d2 < 0.0) d2 = 0.0;
	  if (d2 > 255.0) d2 = 255.0;
	  return noz_byte(d2 + 0.5);
  }

  static void RenderGlyphs(FT_Face face, Image* image, std::vector<FT_Glyph>& glyphs, const FontDef& def) {    
    noz_byte* out = image->Lock();
    noz_int32 out_stride = image->GetStride();

    for(size_t g=0; g<glyphs.size(); g++) {
      FT_Glyph& glyph = glyphs[g];
      if(glyph.packed.w == 0 || glyph.packed.h == 0) continue;      

      FT_Load_Glyph(face,glyph.index,FT_LOAD_RENDER);
      
      // Copy the glpyh bitmap into the final image
      noz_byte* in = face->glyph->bitmap.buffer;
      for(noz_int32 r=0; r<face->glyph->bitmap.rows; r++) {
        memcpy(
          out + out_stride * (r+glyph.packed.y+1) + glyph.packed.x + 1,
          in + face->glyph->bitmap.pitch * r,
          face->glyph->bitmap.pitch
        );
      }
    }

    image->Unlock();
  }

  /**
   * Render all the glyphs to the target image
   */
  static void RenderSDFGlyphs(FT_Face face, Image* image, noz_int32 sdf_scale, noz_int32 sdf_scan, std::vector<FT_Glyph>& glyphs) {    
    noz_byte* buffer = (noz_byte*)image->Lock();
    memset(buffer,0,image->GetWidth() * image->GetHeight());

    noz_uint32 buffer_pitch = image->GetWidth();

    for(size_t g=0; g<glyphs.size(); g++) {
      FT_Glyph& glyph = glyphs[g];
      if(glyph.packed.w == 0 || glyph.packed.h == 0) continue;      

      // Load the glyph and render it.
      // TODO: check error
      FT_Load_Glyph(face, glyph.index, FT_LOAD_RENDER|FT_LOAD_NO_HINTING);      

#if 0
      // Bigger bitmap
      noz_int32 sdf_padding = sdf_scan * sdf_scale;
      noz_int32 sdf_padding_x2 = sdf_padding * 2;
      noz_int32 sdf_pitch = (face->glyph->bitmap.width+sdf_padding_x2);
      noz_int32 sdf_height = (face->glyph->bitmap.rows+sdf_padding_x2);
      noz_int32 sdf_size = sdf_height * sdf_pitch;

      noz_byte* sdf_bitmap = new noz_byte[sdf_size];
      memset(sdf_bitmap,0,sdf_size);

      noz_byte* out = sdf_bitmap + sdf_padding * sdf_pitch + sdf_padding;
      noz_byte* in = face->glyph->bitmap.buffer;
      for(noz_int32 row=0;row<face->glyph->bitmap.rows;row++) {
        memcpy(out,in,face->glyph->bitmap.pitch);
        out += sdf_pitch;
        in += face->glyph->bitmap.pitch;
      }

      out = buffer + (glyph.packed.y + 1) * buffer_pitch + glyph.packed.x + 1;
      for(noz_int32 y=0; y<glyph.packed.h-2; y++) {
        noz_float yy = (noz_float(y) / noz_float(glyph.packed.h-2)) * sdf_height;
        for(noz_int32 x=0; x<glyph.packed.w-2; x++) {
          noz_float xx = (noz_float(x) / noz_float(glyph.packed.w-2)) * sdf_pitch;          
          out[x] = ComputeRadialSDF(
            sdf_bitmap, 
            sdf_pitch, 
            sdf_height, 
            noz_int32(xx),
            noz_int32(yy),
            sdf_scan*sdf_scale
          );
        }
        out += buffer_pitch;
      }

      delete[] sdf_bitmap;

#else
      // Copy the glpyh bitmap into the final image
      noz_byte* out = (noz_byte*)buffer;
      noz_byte* in = face->glyph->bitmap.buffer;
      for(noz_int32 r=0; r<face->glyph->bitmap.rows; r++) {
        memcpy(
          out + buffer_pitch * (r+glyph.packed.y+1) + glyph.packed.x + 1,
          in + face->glyph->bitmap.pitch * r,
          face->glyph->bitmap.pitch
        );
      }

#if 0
      Grid grid1(glyph.packed.w-2, glyph.packed.h-2);
      Grid grid2(glyph.packed.w-2, glyph.packed.h-2);
      for(noz_int32 y=0; y<glyph.packed.h-2; y++) {
        noz_int32 yy = y + glyph.packed.y + 1;
        for(noz_int32 x=0; x<glyph.packed.w-2; x++) {
          noz_int32 xx = x + glyph.packed.x + 1;
          if(buffer[yy*buffer_pitch+xx]<128) {
				    Put( grid1, x, y, inside );
				    Put( grid2, x, y, empty );
			    } else {
				    Put( grid2, x, y, inside );
				    Put( grid1, x, y, empty );
			    }
        }
      }

	    for(noz_int32 y=0;y<image->GetHeight();y++ ) {
        noz_int32 yy = y + glyph.packed.y + 1;
		    for (noz_int32 x=0;x<image->GetWidth();x++ ) {
          noz_int32 xx = x + glyph.packed.x + 1;

			    // Calculate the actual distance from the dx/dy
			    int dist1 = (int)( sqrt( (double)Get( grid1, x, y ).DistSq() ) );
			    int dist2 = (int)( sqrt( (double)Get( grid2, x, y ).DistSq() ) );
			    int dist = dist1 - dist2;

			    // Clamp and scale it, just for display purposes.
			    int c = dist*3 + 128;
			    if ( c < 0 ) c = 0;
			    if ( c > 255 ) c = 255;

          buffer[yy*buffer_pitch+xx] = (noz_byte)c;
		    }
	    }
#endif
#endif

      image->Unlock();

    }
  }

} // namespace noz


Asset* FontFile::Import (void) {
  Font* font = new Font;
  if(!Reimport(font)) {
    delete font;
    return nullptr;
  }

  return font;
}

bool FontFile::Reimport (Asset* asset) {
  Font* font = Cast<Font>(asset);
  if(nullptr==font) return false;

  FileStream fs;
  if(!fs.Open(GetPath(), FileMode::Open)) {
    return nullptr;
  }

  FontDef def;
  Object* o = JsonDeserializer().Deserialize(&fs,&def);
  if(!o->IsType(typeof(FontDef))) {
    delete o;
    return nullptr;
  }

  // Open stream to the source TTF  
  fs.Close();

  return Import(GetPath(), def, font);
}


bool FontFile::Import(const String& file_path, const FontDef& def, Font* font) {
  // Read the font data..
  MemoryStream data;
  if(Font::ReadFontData(def.family.ToCString(), def.style.ToCString(), def.height, &data)) {
    FileStream fs;    
    data.Seek(0,SeekOrigin::Begin);
    if(fs.Open(String::Format("%s.ttf", Path::ChangeExtension(file_path,"").ToCString()), FileMode::Truncate)) {
      fs.Write((char*)data.GetBuffer(), 0, data.GetLength());
      fs.Close();
    }    
  }

  FileStream ttf;
  if(!ttf.Open(String::Format("%s.ttf", Path::ChangeExtension(file_path,"").ToCString()), FileMode::Open)) {
    return false;
  }

  noz_int32 sdf_scale = def.sdf_scale ? def.sdf_scale : 1;
  noz_int32 sdf_scan  = def.sdf_scale ? def.sdf_scan : 0;

  noz_int32 glyph_min_ = 32;
  noz_int32 glyph_max_ = 127;
  wchar_t* codes = new wchar_t[glyph_max_-glyph_min_+2];
  memset(codes,0,sizeof(wchar_t) * (glyph_max_-glyph_min_+2));
  for(noz_int32 i=glyph_min_;i<=glyph_max_;i++) codes[i-glyph_min_]=i;

  // TODO: use chars

  
  // Initialzie FreeType library
  FT_Error error;
  FT_Library library = nullptr;
  if(0!=(error=FT_Init_FreeType(&library))) {
    Console::WriteLine("FT_Error: 0x%02x: %s", FT_Errors[error].code, FT_Errors[error].message);
    return nullptr;
  }

  noz_float height = (noz_float)def.height;
  if(def.sdf) {
    height -= 8.0f;
  }

  // Scale up the size by the SDF scale factor 
  noz_int32 size = noz_int32(height * 96.0f / 72.0f);
  FT_Face face = nullptr;
  if(!FT_LoadFace(library, &face, (noz_float)size, def.sdf_scale, &ttf)) {
    FT_Done_FreeType(library);
    return nullptr;
  }

  // Generate the list of all FT glyphs
  noz_uint32 num_codes = wcslen(codes);
  std::vector<FT_Glyph> ft_glyphs = FT_GetGlyphs(face,def,codes,num_codes,sdf_scale,0);

  // Find a bin packing solution that fits all of the glyphs.
  BinPacker packer;
  FT_PackGlyphs(packer,ft_glyphs);

  // Create the final image for the font
  Image* image = new Image(packer.GetSize().w,packer.GetSize().h,def.sdf_scale ? ImageFormat::SDF : ImageFormat::A8);
  image->SetFilterMode(ImageFilterMode::Automatic);

  // Render the glyphs to the sheet in their respective locations.
  if(def.sdf) {
    RenderSDFGlyphs(face,image,def.sdf_scale,def.sdf_scan,ft_glyphs);
  } else {
    RenderGlyphs(face,image,ft_glyphs,def);
  }

  // Create the font to return..
  font->glyph_min_ = glyph_min_;
  font->glyph_max_ = glyph_max_;
  font->glyphs_.resize(glyph_max_-glyph_min_+1);
  font->image_ = image;
  font->distance_field_ = def.sdf_scale != 0;

  FT_Size_Metrics metrics = face->size->metrics; 
  font->height_ = noz_float((metrics.height>>6)/sdf_scale);
  //font->ascent_ = (noz_float(metrics.ascender) / noz_float(metrics.height)) * font->height_;
  font->ascent_ = noz_float((metrics.ascender)>>6); //  / noz_float(metrics.height)) * font->height_;
  font->descent_ = noz_float((metrics.descender>>6)/sdf_scale);

  noz_float image_w = noz_float(image->GetWidth());
  noz_float image_h = noz_float(image->GetHeight());

  noz_int32 max_h = 0;

  for(size_t g=0; g<ft_glyphs.size(); g++) {
    FT_Glyph& ft_glyph = ft_glyphs[g];

    // Initialize the glyph
    Font::Glyph* glyph = &font->glyphs_[g];
    glyph->w = ft_glyph.packed.w - (ft_glyph.packed.w > 0 ? 2 : 0);
    glyph->h = ft_glyph.packed.h - (ft_glyph.packed.h > 0 ? 2 : 0);
    glyph->ox = ft_glyph.offset_x;
    glyph->oy = ft_glyph.offset_y;
    glyph->ax = ft_glyph.ax;

    glyph->s.x = (noz_float(ft_glyph.packed.x+1)) / image_w;
    glyph->s.y = (noz_float(ft_glyph.packed.y+1)) / image_h;
    glyph->t.x = (noz_float(ft_glyph.packed.x+1+ft_glyph.packed.w-2)) / image_w;
    glyph->t.y = (noz_float(ft_glyph.packed.y+1+ft_glyph.packed.h-2)) / image_h;

    max_h = Math::Max(max_h,glyph->oy);
  }

  font->line_spacing_ = font->ascent_ - max_h;

#if 0
  for(noz_uint32 i=0; i<num_codes; ++i) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, codes[i]);
    for(noz_uint32 j=0; j<num_codes; ++j) {
      FT_UInt prev_index = FT_Get_Char_Index (face, codes[j]);
      FT_Vector kerning;
      FT_Get_Kerning(face, prev_index, glyph_index, FT_KERNING_DEFAULT, &kerning );
      if(kerning.x != 0) {
        Font::Kerning k;
        k.code = Font::MakeKerningCode(codes[j],codes[i]);
        k.x = noz_int32(kerning.x / 064.0f / SDFScaleFactor);
        font->kerning_.push_back(k);
      }
    }
  }

  font->UpdateKerningMap();
#endif

  FT_Done_Face(face);
  FT_Done_FreeType(library);

  return true;
}

bool FontFile::CreateEmpty (void) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Truncate)) return nullptr;

  FontDef def;
  def.family = "Arial";
  def.style = "Regular";
  def.height = 12;
  JsonSerializer().Serialize(&def, &fs);
  fs.Close();
  
  return true;
}

void FontFile::OnDeleted (void) {
  File::Delete(Path::ChangeExtension(GetPath(),".ttf"));
}

void FontFile::OnMoved (const String& from, const String& to) {
  File::Move(Path::ChangeExtension(from,".ttf"), Path::ChangeExtension(to,".ttf"));
}
