/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#ifdef HAVE_LIBPNG

#include <cstdio>
#include <cstring>

#include <toad/toad.hh>
#include <toad/bitmap.hh>
#include <toad/bitmapfilter.hh>
#include <toad/filter_png.hh>

#include <toad/io/urlstream.hh>

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
  if (setjmp(png_ptr->jmpbuf)) {
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

  if (info_ptr->bit_depth <= 8) {
    png_set_expand(png_ptr);  // expand to 8 bit per pixel
    
    png_read_update_info(png_ptr, info_ptr);
    
    createBuffer(info_ptr->width,info_ptr->height, TBITMAP_TRUECOLOR);
    png_bytep row_pointers[info_ptr->height];
    for (unsigned row = 0; row < info_ptr->height; row++)
      row_pointers[row] = (png_bytep)malloc(info_ptr->rowbytes);
    png_read_image(png_ptr, row_pointers);
    
    unsigned x,y;
    TRGB c;
    unsigned char *ptr;
    for(y=0; y<info_ptr->height; y++) {
      ptr = row_pointers[y];
      for(x=0; x<info_ptr->width; x++) {
        c.r = *(ptr++);
        c.g = *(ptr++);
        c.b = *(ptr++);
        if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA)
          ptr++;
        setColorPixel(x,y,c);
      }
    }
    ok = true;
    
    for (unsigned row = 0; row < info_ptr->height; row++)
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
  png_structp png_ptr = png_create_write_struct(
    PNG_LIBPNG_VER_STRING,
    NULL,   // user error pointer
    NULL,   // user error function
    NULL);  // user warning function
    
  if (!png_ptr) {
    setError("Couldn't create PNG write structure.");
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    setError("Couldn't create PNG info structure.");
    return false;
  }
  
  // set error handling
  //--------------------
  if (setjmp(png_ptr->jmpbuf)) {
    // Free all of the memory associated with the png_ptr and info_ptr
    png_destroy_write_struct(&png_ptr, &info_ptr);
    setError("Caught an error while writing PNG file.");
    return false;
  }

  png_set_write_fn(png_ptr, 
                   &stream, 
                   &user_write_data,
                   &user_flush_write);

  int palette_size;
  if (convertToIndexed(&palette_size)) {
    png_set_IHDR(png_ptr, 
                 info_ptr,
                 w,h,8,
                 PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    png_colorp palette = (png_colorp)png_malloc(png_ptr, 
                                                palette_size*sizeof(png_color));
    TRGB c;
    for(int i=0;i<palette_size;i++) {
      getIndexColor(i,&c);
      palette[i].red  = c.r;
      palette[i].green= c.g;
      palette[i].blue = c.b;
    }

    png_set_PLTE(png_ptr, info_ptr, palette, palette_size);

    png_write_info(png_ptr, info_ptr);
    
    png_bytep row_pointers[info_ptr->height];
    for (unsigned row = 0; row < info_ptr->height; row++) {
      png_bytep ptr = row_pointers[row] = (png_bytep)malloc(info_ptr->rowbytes);
      for(int x=0; x<w; x++) {
        *ptr = getIndexPixel(x,row); ptr++;
      }
    }
    
    png_write_image(png_ptr, row_pointers);
    
    for (unsigned row = 0; row < info_ptr->height; row++)
      free(row_pointers[row]);

    png_write_end(png_ptr, info_ptr);
    
    free(palette);
    
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return true;
  } else {
    png_set_IHDR(png_ptr, 
                 info_ptr,
                 w,h,8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);
    
    png_bytep row_pointers[info_ptr->height];
    TRGB c;
    for (unsigned row = 0; row < info_ptr->height; row++) {
      png_bytep ptr = row_pointers[row] = (png_bytep)malloc(info_ptr->rowbytes);
      for(int x=0; x<w; x++) {
        getColorPixel(x,row,&c);
        *ptr = c.r; ptr++;
        *ptr = c.g; ptr++;
        *ptr = c.b; ptr++;
      }
    }
    
    png_write_image(png_ptr, row_pointers);
    
    for (unsigned row = 0; row < info_ptr->height; row++)
      free(row_pointers[row]);

    png_write_end(png_ptr, info_ptr);
    
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return true;
  }
  return false;
}

} // namespace toad

#endif
