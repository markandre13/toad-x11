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
#include <toad/filter_jpeg.hh>

#ifdef HAVE_LIBJPEG

#include <setjmp.h>
extern "C" {
  #include <jpeglib.h>
  #include <jerror.h>
};

using namespace toad;

int TFilterJPEG::editSpecific()
{
  return TMessageBox::OK;
}

static jmp_buf setjmp_buffer;

// Load Picture
//---------------------------------------------------------------------------
static void 
read_image(TFilterJPEG *fltr, struct jpeg_decompress_struct* cinfo)
{
  TRGB rgb;
  int row_stride = cinfo->output_width * cinfo->output_components;
  JSAMPARRAY buffer = (*cinfo->mem->alloc_sarray) ((j_common_ptr) cinfo, JPOOL_IMAGE, row_stride, 1);
  while (cinfo->output_scanline < cinfo->output_height) {
    (void) jpeg_read_scanlines(cinfo, buffer, 1);
    JSAMPROW row = buffer[0];
    for (unsigned int x = 0; x < cinfo->output_width; x++) {
      switch(cinfo->jpeg_color_space) {
        case JCS_GRAYSCALE:
          rgb.r = rgb.g = rgb.b = *row++;
          break;
/*
        case JCS_UNKNOWN:
        case JCS_YCbCr:
        case JCS_CMYK:
        case JCS_YCCK:
        case JCS_RGB:
*/
        default:
          rgb.r = *row++;
          rgb.g = *row++;
          rgb.b = *row++;
          break;
      }
      fltr->setColorPixel(x, cinfo->output_scanline, rgb);
    }
  }
}

static void 
my_error_exit(j_common_ptr cinfo) {
  longjmp(setjmp_buffer, 1);
}

// jdatasrc.c

struct toad_source {
  struct jpeg_source_mgr jlib;
  istream *stream;
  JOCTET buffer[4096];
};

static void
toad_jpeg_init_source(j_decompress_ptr cinfo)
{
  toad_source* src = (toad_source*)cinfo->src;
  src->stream->seekg(0);
}

static boolean
toad_jpeg_fill_input_buffer(j_decompress_ptr cinfo)
{
  toad_source* src = (toad_source*)cinfo->src;
  size_t nbytes = src->stream->readsome((char*)src->buffer, 4096);
  if (src->stream->eof()) {
    cerr << "warning: no eof mark in jpeg file" << endl;
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }
  src->jlib.next_input_byte = src->buffer;
  src->jlib.bytes_in_buffer = nbytes;
  
  return TRUE;
}

static void
toad_jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  if (num_bytes > 0) {
    toad_source* src = (toad_source*)cinfo->src;
    while (num_bytes > (long) src->jlib.bytes_in_buffer) {
      num_bytes -= (long) src->jlib.bytes_in_buffer;
      toad_jpeg_fill_input_buffer(cinfo);
    }
    src->jlib.next_input_byte += (size_t) num_bytes;
    src->jlib.bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void
toad_jpeg_term_source (j_decompress_ptr cinfo)
{
//  toad_source* src = (toad_source*)cinfo->src;
}

static void
jpeg_stream_src(j_decompress_ptr cinfo, istream *stream)
{
  if (cinfo->src == NULL) {
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) (
        (j_common_ptr) cinfo, 
        JPOOL_PERMANENT,
        sizeof(toad_source) );
  }
  toad_source* src = (toad_source*)cinfo->src;
  src->jlib.init_source       = toad_jpeg_init_source;
  src->jlib.fill_input_buffer = toad_jpeg_fill_input_buffer;
  src->jlib.skip_input_data   = toad_jpeg_skip_input_data;
  src->jlib.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->jlib.term_source       = toad_jpeg_term_source;
  src->stream = stream;
  src->jlib.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->jlib.next_input_byte = NULL; /* until buffer loaded */
}

TFilterJPEG::EResult 
TFilterJPEG::load(istream &stream)
{
  struct jpeg_decompress_struct* cinfo = new jpeg_decompress_struct;

  jpeg_error_mgr em;
  cinfo->err = jpeg_std_error(&em);
  em.error_exit = my_error_exit;

  if (setjmp(setjmp_buffer)) {
    jpeg_destroy_decompress(cinfo);
    delete cinfo;
    setError("Error while loading JPEG file");
    return WRONG;
  }

  jpeg_create_decompress(cinfo);

  jpeg_stream_src(cinfo, &stream);

  jpeg_read_header(cinfo, TRUE);
  if (jpeg_has_multiple_scans(cinfo)) {
    cinfo->buffered_image = TRUE;
  }
  jpeg_start_decompress(cinfo);
  
  createBuffer(cinfo->output_width, cinfo->output_height, TBITMAP_TRUECOLOR);

  if (jpeg_has_multiple_scans(cinfo)) {
    while (! jpeg_input_complete(cinfo)) {
      jpeg_start_output(cinfo, cinfo->input_scan_number);
      read_image(this, cinfo);
      jpeg_finish_output(cinfo);
    }
  } else {
    read_image(this, cinfo);
  }

  jpeg_finish_decompress(cinfo);

  jpeg_destroy_decompress(cinfo);
  delete cinfo;
  
  return OK;
}

bool TFilterJPEG::save(ostream &os)
{
#if 0
  // indexed
  //---------
  if (!ConvertToIndexed(&palette_size))
    return false;
    
  for(int i=0;i<256;i++)
    file << GetIndexColor(i,&c);
    
  for(int x=0; x<w; x++)
    for(int y=0; y<h; y++)
      file << GetIndexPixel(x,y);

  // true color
  //------------
  for(int x=0; x<w; x++)
    for(int y=0; y<h; y++)
      file << GetColorPixel(x,y,&c);
#endif
  return false;
}

#endif
