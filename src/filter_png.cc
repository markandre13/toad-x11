/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <cstdio>
#include <cstring>

#include <toad/toad.hh>
#include <toad/bitmap.hh>
#include <toad/bitmapfilter.hh>
#include <toad/filter_png.hh>

#include <toad/io/urlstream.hh>

#ifdef HAVE_LIBPNG

extern "C" {
#include <png.h>
};

namespace toad {

int TFilterPNG::editSpecific()
{
  return TMessageBox::OK;
}

// Load
//---------------------------------------------------------------------------
static void user_read_data(png_structp png_ptr, 
                           png_bytep data, 
                           png_size_t length)
{
  istream *is = (istream*) png_get_io_ptr(png_ptr);
  is->read((char*)data, length);
  if (is->gcount() != length)
    png_error(png_ptr, "Read Error");
}

static void user_write_data(png_structp png_ptr, 
                            png_bytep data, 
                            png_size_t length)
{
  ostream *os = (ostream*) png_get_io_ptr(png_ptr);
  os->write((char*)data, length);
}

static void user_flush_write(png_structp png_ptr)
{
}

TFilterPNG::EResult 
TFilterPNG::load(istream &stream)
{
  // check for PNG file
  //--------------------
  unsigned char header[10];
  stream.read((char*)header,8);
  if (stream.gcount()!=8 || 
      !png_check_sig(header,8)) 
  {
    setError("%s is not a PNG file.");
    return WRONG;
  }
  stream.seekg(0);
  
  // create PNG data structures
  //----------------------------
  png_struct *png_ptr = png_create_read_struct(
    PNG_LIBPNG_VER_STRING,
    (void*)NULL,          // user_error_ptr,
    (png_error_ptr)NULL,  // user_error_fn,
    (png_error_ptr)NULL   // user_warning_fn
  );
  if (!png_ptr) {
    setError("couldn't allocate PNG read structure");
    return ERROR;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    setError("couldn't allocate PNG info pointer");
    return ERROR;
  }
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    setError("couldn't allocate PNG end info");
    return ERROR;
  }

  // set error handling
  //--------------------
  if (setjmp(png_jmpbuf(png_ptr))) {
    // Free all of the memory associated with the png_ptr and info_ptr
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    setError("Caught an error while reading PNG file.");
    return ERROR;
  }
  
  // set i/o
  //---------------------
  png_set_read_fn(png_ptr, &stream, &user_read_data);

  // read file information
  //-----------------------
  png_read_info(png_ptr, info_ptr);
#if 0
  const char *str;
  switch(info_ptr->color_type) {
    case PNG_COLOR_TYPE_PALETTE:
      str="PNG_COLOR_TYPE_PALETTE";
      break;
    case PNG_COLOR_TYPE_GRAY:
      str="PNG_COLOR_TYPE_GRAY";
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      str="PNG_COLOR_TYPE_ALPHA";
      break;
    case PNG_COLOR_TYPE_RGB:
      str="PNG_COLOR_TYPE_RGB";
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
      str="PNG_COLOR_TYPE_RGB_ALPHA";
      break;
    default:
      str="(unknown)";
  }
  printf("color type: %s\n",str);
#endif
  bool ok = false;

  if (png_get_bit_depth(png_ptr, info_ptr) <= 8) {
    png_set_expand(png_ptr);  // expand to 8 bit per pixel
    
    png_read_update_info(png_ptr, info_ptr);
    
    png_uint_32 png_width = png_get_image_width(png_ptr, info_ptr);
    png_uint_32 png_height = png_get_image_height(png_ptr, info_ptr);
    
    createBuffer(png_width, png_height, TBITMAP_TRUECOLOR);
    png_bytep row_pointers[png_height];
    for (unsigned row = 0; row < png_height; row++)
      row_pointers[row] = (png_bytep)malloc(png_get_rowbytes(png_ptr, info_ptr));
    png_read_image(png_ptr, row_pointers);
    
    unsigned x,y;
    TRGB24 c;
    unsigned char *ptr;
    for(y=0; y<png_height; y++) {
      ptr = row_pointers[y];
      for(x=0; x<png_width; x++) {
        c.r = *(ptr++);
        c.g = *(ptr++);
        c.b = *(ptr++);
        if (png_get_color_type(png_ptr, info_ptr) & PNG_COLOR_MASK_ALPHA)
          ptr++;
        setColorPixel(x,y,c);
      }
    }
    ok = true;
    
    for (unsigned row = 0; row < png_height; row++)
      free(row_pointers[row]);
  }
  
  if (!ok) {
    setError("Can't read this type of PNG files yet");
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return ERROR;
  }

  png_read_end(png_ptr, end_info);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  return OK;
}

bool TFilterPNG::save(ostream &stream)
{
  return false;
}

} // namespace toad

#endif
