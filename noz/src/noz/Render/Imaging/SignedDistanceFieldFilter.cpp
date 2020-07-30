///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "SignedDistanceFieldFilter.h"

using namespace noz;

#if 0
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



Image* SignedDistanceFieldFilter::Filter (Image* image) {
  noz_assert(image);

  static const noz_int32 INSIDE = 0;
  static const noz_int32 EMPTY = 9999;

  // Cache width / height / stride
  noz_uint32 w = image->GetWidth();
  noz_uint32 h = image->GetHeight();
  noz_uint32 s = image->GetStride();

  noz_uint32 pm_w = w + 2;
  noz_uint32 pm_h = h + 2;

  // Size of an individual pixel map
  noz_int32 pm_size = pm_w * pm_h;

  // Raw pixel data needed for filter
  Pixel* pixels = new Pixel[pm_size*2];

  // Intialize both pixel grids.
  PixelMap pm[2] = {
    {pixels, pm_w, pm_h},
    {pixels + pm_size, pm_w, pm_h}
  };
    
  noz_uint32 x;
  noz_uint32 y;
  Pixel* out[2];
  noz_byte* in;

  // Initialize the Left edge
  out[0] = pm[0].p_;
  out[1] = pm[1].p_;
  for(y=0;y<pm_h;y++,out[0]+=pm_w,out[1]+=pm_w) {
    out[0]->dx_ = out[0]->dy_ = INSIDE;
    out[1]->dx_ = out[1]->dy_ = EMPTY;
  }    

  // Initialize the right edge
  out[0] = pm[0].p_+pm_w-1;
  out[1] = pm[1].p_+pm_w-1;
  for(y=0;y<pm_h;y++,out[0]+=pm_w,out[1]+=pm_w) {
    out[0]->dx_ = out[0]->dy_ = INSIDE;
    out[1]->dx_ = out[1]->dy_ = EMPTY;
  }    

  // Initialize the top edge
  out[0] = pm[0].p_+1;
  out[1] = pm[1].p_+1;
  for(x=0;x<w;x++,out[0]++,out[1]++) {
    out[0]->dx_ = out[0]->dy_ = INSIDE;
    out[1]->dx_ = out[1]->dy_ = EMPTY;
  }    

  // Initialize the bottom edge.
  out[0] = pm[0].p_+1+pm_w*(h+1);
  out[1] = pm[1].p_+1+pm_w*(h+1);
  for(x=0;x<w;x++,out[0]++,out[1]++) {
    out[0]->dx_ = out[0]->dy_ = INSIDE;
    out[1]->dx_ = out[1]->dy_ = EMPTY;
  }    
    
  // Intialite the bitmap content.
  noz_int32 in_depth = image->GetDepth();
  in = image->Lock() + (in_depth-1);
  for(y=0;y<h;y++) {
    out[0] = pm[0].p_ + 1 + (y+1) * pm_w;
    out[1] = pm[1].p_ + 1 + (y+1) * pm_w;

    for(x=0; x<w; x++,out[0]++,out[1]++,in+=in_depth) {
      if(*in < 128) {
        out[0]->dx_ = out[0]->dy_ = INSIDE; // ((noz_float)*in) / 255.0f * 1.0f; // INSIDE;
        out[1]->dx_ = out[1]->dy_ = EMPTY;
      } else {
        out[0]->dx_ = out[0]->dy_ = EMPTY;
        out[1]->dx_ = out[1]->dy_ = INSIDE; //((noz_float)(*in-128)) / 128.0f; // INSIDE;
      }
    }
  }

  // Compute the SDF for both pixel maps
  Compute(&pm[0]);
  Compute(&pm[1]);

  // Create the output image
  Image* result = new Image(w, h, ImageFormat::SDF);
  
  noz_byte* buffer = result->Lock();
	for(y=0;y<h;y++) {
		for (x=0;x<w;x++, buffer++) {
			// Calculate the actual distance from the dx/dy
			noz_double dist1 = Math::Sqrt((noz_double)pm[0].Get(x+1,y+1)->DistanceSqr());
			noz_double dist2 = Math::Sqrt((noz_double)pm[1].Get(x+1,y+1)->DistanceSqr());
			noz_int32 dist = (noz_int32)(dist2 - dist1);

			// Clamp value.
      *buffer = (noz_byte)Math::Clamp(128+dist*3, 0, 255);
		}
	}

  result->Unlock();
  image->Unlock();

  delete[] pixels;

  return result;
}


void SignedDistanceFieldFilter::Compute(PixelMap* pm, Pixel* p, noz_uint32 x, noz_uint32 y, noz_int32 ox, noz_int32 oy) {
  noz_assert(x>=0 && x<=pm->w_);
  noz_assert(y>=0 && y<=pm->h_);

  Pixel o = *pm->Get(x+ox,y+oy);
  o.dx_ += ox;
  o.dy_ += oy;
  if(o.DistanceSqr() < p->DistanceSqr()) {
    *p = o;
  }
}

void SignedDistanceFieldFilter::Compute (PixelMap* pm) {
  noz_uint32 x;
  noz_uint32 y;

	// Pass 1
	for (y=1;y<pm->h_-1;y++) {
		for (x=1;x<pm->w_-1;x++) {
      Pixel* p = pm->Get(x,y);
			Compute(pm, p, x, y, -1,  0 );
			Compute(pm, p, x, y,  0, -1 );
			Compute(pm, p, x, y, -1, -1 );
			Compute(pm, p, x, y,  1, -1 );
		}

		for (x=pm->w_-2;x>=1;x--) {
			Pixel* p = pm->Get(x, y);
			Compute (pm, p, x, y, 1, 0 );
		}
	}

	// Pass 2
	for (y=pm->h_-2;y>=1;y--)	{
		for (x=pm->w_-2;x>=1;x--) {
      Pixel* p = pm->Get(x,y);
			Compute (pm, p, x, y,  1,  0 );
			Compute (pm, p, x, y,  0,  1 );
			Compute (pm, p, x, y, -1,  1 );
			Compute (pm, p, x, y,  1,  1 );
		}

		for (x=1;x<pm->w_-1;x++) {
      Pixel* p = pm->Get(x,y);
			Compute(pm, p, x, y, -1, 0 );
		}
	}
}