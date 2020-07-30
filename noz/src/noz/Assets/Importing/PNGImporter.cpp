///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/IO/File.h>
#include "PNGImporter.h"

extern "C" {
#include <external/lpng163/pngpriv.h>
}

using namespace noz;

static void PNGRead (png_structp png_ptr, png_bytep data, png_size_t size) {
  Stream* stream = (Stream*)png_ptr->io_ptr;
  stream->Read ((char*)data, 0, size);
}

void PNGWrite ( png_structp png_ptr, png_bytep data, png_size_t size) {
  Stream* stream = (Stream*)png_ptr->io_ptr;
  stream->Write((char*)data, 0, size);
}
  
void PNGFlush (png_structp png_ptr ) {
}

Image* noz::PNGImporterOld::Import(Stream* stream) {
	png_structp		  png_ptr;
	png_infop		    info_ptr;
	unsigned int	  sig_read;
  
  stream->Seek(0,SeekOrigin::Begin);

	sig_read = 0;

	// Create and initialize the png_struct with the desired error handler
    // functions.  If you want to use the default stderr and longjump method,
    // you can supply nullptr for the last three parameters.  We also supply the
    // the compiler header file version, so that we know if the application
    // was compiled with a compatible version of the library.  REQUIRED
	png_ptr = png_create_read_struct ( PNG_LIBPNG_VER_STRING,
									    nullptr, 
									    nullptr, 
									    nullptr );

	if (png_ptr == nullptr) {
		return nullptr;
	}

	// Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr) {
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		return nullptr;
	}

	// Set error handling if you are using the setjmp/longjmp method (this is
    // the normal method of doing things with libpng).  REQUIRED unless you
    // set up your own error handlers in the png_create_read_struct() earlier.
	if (setjmp(png_jmpbuf(png_ptr))) {
		// Free all of the memory associated with the png_ptr and info_ptr 
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

		// If we get here, we had a problem reading the file 
		return nullptr;
	}

	// Set the read function
	png_set_read_fn ( png_ptr, (void *)stream, PNGRead );

	// If we have already read some of the signature 
	png_set_sig_bytes(png_ptr, sig_read);

    // If you have enough memory to read in the entire image at once,
    // and you need to specify only transforms that can be controlled
    // with one of the PNG_TRANSFORM_* bits (this presently excludes
    // dithering, filling, setting background, and doing gamma
    // adjustment), then you can read the entire image (including
    // pixels) into the info structure with this call:
	png_read_png(png_ptr, info_ptr, 0, nullptr);

  int stride = info_ptr->width * info_ptr->channels;
  ImageFormat format = ImageFormat::R8G8B8;
  if(info_ptr->channels==4) {
    format = ImageFormat::R8G8B8A8;
  }

  Image* image = new Image(info_ptr->width, info_ptr->height, format);
    
  unsigned char* out = image->GetBuffer();
  int row;
  for ( row = 0; row < (int)info_ptr->height; row ++, out += stride ) {
	  png_bytep in = info_ptr->row_pointers[ row ];
	  memcpy ( out, in, stride );
	}

  // clean up after the write, and free any memory allocated 
	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);    

  return image;
}



Asset* PNGImporterOld::Import (const String& path) {
  FileStream fs;
  if(!fs.Open(path,FileMode::Open,FileAccess::Read)) {
    return nullptr;
  }

  return Import(&fs);
}
