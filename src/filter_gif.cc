/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-AndrÃ© Hopf <mhopf@mark13.org>
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

/*
    TFilterGIF is based upon the work of the following persona and is
    Copyright (C) 1984,1989,1990,1991,1993,1995 and 1996 by

    - Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
    - Jim McKie               (decvax!mcvax!jim)
    - Steve Davies            (decvax!vax135!petsd!peora!srd)
    - Ken Turkowski           (decvax!decwrl!turtlevax!ken)
    - James A. Woods          (decvax!ihnp4!ames!jaw)
    - Joe Orost               (decvax!vax135!petsd!joe)
      wrote 'compress.c' - File compression ala IEEE Computer, June 1984

    - David Rowley (mgardi@watdcsu.waterloo.edu)
      wrote GIFENCOD, a Lempel-Ziv compression based on 'compress' for GIF

    - Marcel Wijkstra (wijkstra@fwi.uva.nl)
      modified GIFENCOD

    - Jef Poskanzer
      wrote "ppmtogif.c" (read a portable pixmap and produce a GIF file)

    - David Koblas (koblas@netcom.com)
      has written the "netpbm" package

    - Peter Mattis
      wrote the GIMP GIF plug-in

    - Mark-André Hopf (mhopf@mark13.de.de) 1997,98
      wrote this nasty TFilterGIF class
    
    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose and without fee is hereby granted, provided
    that the above copyright notice appear in all copies and that both that
    copyright notice and this permission notice appear in supporting
    documentation.  This software is provided "as is" without express or
    implied warranty.     
    
    The Graphics Interchange Format(c) is the Copyright property of
    CompuServe Incorporated.  GIF(sm) is a Service Mark property of
    CompuServe Incorporated
*/

// TOAD will be able to write GIFs starting at June 20 2003 when the
// LZW patent expires.

//#ifdef GIF_PATENT_EXPIRED

#include <toad/toad.hh>
#include <toad/bitmap.hh>
#include <toad/bitmapfilter.hh>
#include <toad/filter_gif.hh>
#include <toad/io/binstream.hh>

#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <toad/toad.hh>
#include <toad/bitmap.hh>
#include <toad/bitmapfilter.hh>
#include <toad/filter_gif.hh>

#include <toad/dialog.hh>
#include <toad/checkbox.hh>
#include <toad/pushbutton.hh>
#include <toad/scrollbar.hh>

namespace toad {

#define MAX_LWZ_BITS 12

#ifndef OLD

#define TRUE             true
#define FALSE            false

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define LM_to_uint(a,b)         (((b)<<8)|(a))

static int GetDataBlock (TInBinStream&, unsigned char *);
static int GetCode (TInBinStream&, int, bool);
static int LWZReadByte (TInBinStream&, int, int);
static bool ReadExtension (TInBinStream&, int);

#endif

struct TFilterGIF::TColormap
{
  unsigned size;
  TRGB24 map[256];
  void read(TInBinStream &in)
  {
    for(unsigned i=0; i<size; i++) {
      map[i].r = in.readByte();
      map[i].g = in.readByte();
      map[i].b = in.readByte();
    }
  }
  void read(TInBinStream &in, unsigned size)
  {
    this->size = size;
    read(in);
  }
};

TFilterGIF::TColormap global_colormap;

int TFilterGIF::editSpecific()
{
  return TMessageBox::OK;
  /*
  TOADBase::MessageBox(NULL, "TOAD PNG Filter",
  "this message is going to be replaced by dialog for "
  "editing things like 'interlace' and 'background color'",
  MB_OK | MB_ICONINFORMATION);
  */
}

// Load GIF Picture
//---------------------------------------------------------------------------
TFilterGIF::EResult 
TFilterGIF::load(istream &stream)
{
  bool bLoop;
  
  // check for GIF file
  //--------------------
  char buffer[3];
  stream.read(buffer, 3);
  if (stream.gcount()!=3 || strncmp(buffer, "GIF", 3)!=0) {
    setError("%s is not a GIF file.");
    return WRONG;
  }
  
  // check for GIF version
  //-----------------------
  type = GIF_TYPE_NONE;
  stream.read(buffer,3);
  if(strncmp(buffer, "87a", 3)==0)
    type = GIF_TYPE_87A;
  if(strncmp(buffer, "89a", 3)==0)
    type = GIF_TYPE_89A;
  if (type==GIF_TYPE_NONE) {
    setError("%s is an unknown GIF type.");
    return ERROR;
  }
  
  // load the first block of header informations
  //---------------------------------------------
  TInBinStream in(&stream);
  width         = in.readWord();
  height        = in.readWord();
  unsigned char byte  = in.readByte();
  global_colormap.size= 2 << (byte & 0x07);
  color_resolution    = ((byte&0x70) >> 3)+1;
  has_global_colormap = byte & 0x80;
  background    = in.readByte();
  aspect_ratio  = in.readByte();

#if 0
  printf("size                : %i,%i\n",width,height);
  printf("global colormap size: %i\n",global_colormap.size);
  printf("color resolution    : %i\n",color_resolution);
  printf("background          : %i\n",background);
  printf("aspect ratio        : %i\n",aspect_ratio);
  printf("global colormap     : %s\n",has_global_colormap?"yes":"no");
#endif

  // read global colormap
  //----------------------
  if (has_global_colormap) {
    global_colormap.read(in);
  }
  
  // read gif thunks
  //-----------------
  image_count = 0;
  bLoop = true;
  while(bLoop) {
    unsigned char byte;
    byte = in.readByte();
    switch(byte) {
      case ';':
        // printf("GIF terminator\n");
        bLoop = false;
        break;
      case '!':
        // printf("GIF extension\n");
        byte = in.readByte();
        ReadExtension(in, byte);
        break;
      default:
        if (byte != ',')
          break;
        image_count++;
        TImage *img = new TImage;
        in.readWord();                  // left
        in.readWord();                  // top
        img->len = in.readWord();       // width
        img->height = in.readWord();    // height
        byte = in.readByte();
        img->interlace = byte & 0x40;
        TColormap local_colormap;
        if (byte & 0x80) {
          local_colormap.read(in, 1 << ((byte&0x07)+1) );
          img->colormap = &local_colormap;
        } else {
          img->colormap = &global_colormap;
        }
        
        // img->print();

        unsigned char code_size;
        code_size = in.readByte();
        // printf("LZW minimum code size: %u\n",(unsigned)code_size);
        
        #ifdef OLD
        InitLWZ(code_size);
        #else
        int c = code_size;
        LWZReadByte(in, TRUE, c);
        #endif
        
        createBuffer(width,height,TBITMAP_INDEXED);
        for(int i=0; i<256; i++)
          setIndexColor(i,img->colormap->map[i]);
          
        unsigned xpos=0, ypos=0;
        int pass=0;
        int v;
        while ((v = LWZReadByte(in, FALSE, c)) >= 0) {
          setIndexPixel(xpos,ypos,v);
          xpos++;
          
          if (xpos==img->len) {
            xpos=0;
            if (img->interlace) {
              static const int step[]={8,8,4,2};
              ypos+=step[pass];
              if (ypos >= height) {
                pass++;
                if (pass>=4)
                  goto fin;
                static const int start[]={0,4,2,1};
                ypos=start[pass];
              }
            } else {
              ypos++;
            }
          }
          if (ypos >= height)
            break;
        } 
        fin:
        LWZReadByte(in, FALSE, c);
        delete img;
return OK; // the code can't handle more than one image yet, so stop here
        break;
    }
  }
  return OK;
}

static bool ZeroDataBlock = false;

static int GetDataBlock (TInBinStream &in, unsigned char *buf)
{
  unsigned char count = in.readByte();

  ZeroDataBlock = count == 0;

  if (count != 0)
    in.readString(buf, count);

  return count;
}

static int GetCode(TInBinStream &in, int code_size, bool flag)
{
  // code.*
  static unsigned char buf[280];
  static int curbit, lastbit, done, last_byte;
  int i, j, ret;
  unsigned char count;

  if (flag) {
    curbit = 0;
    lastbit = 0;
    done = FALSE;
    return 0;
  }

  if ((curbit + code_size) >= lastbit) {
    if (done) {
      if (curbit >= lastbit)
        throw runtime_error("TFilterGIF: ran off the end of by bits");
      return -1;
    }
    buf[0] = buf[last_byte - 2];
    buf[1] = buf[last_byte - 1];

    if ((count = GetDataBlock (in, &buf[2])) == 0)
      done = TRUE;

    last_byte = 2 + count;
    curbit = (curbit - lastbit) + 16;
    lastbit = (2 + count) * 8;
  }

  ret = 0;
  for (i = curbit, j = 0; j < code_size; ++i, ++j)
    ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

  curbit += code_size;

  return ret;
}

static int LWZReadByte(TInBinStream &in, int flag, int input_code_size)
{
  static bool fresh = false;
  int code, incode;
  static int code_size, set_code_size;
  static int max_code, max_code_size;
  static int firstcode, oldcode;
  static int clear_code, end_code;
  static const int table_size = 1 << MAX_LWZ_BITS;
  static int table[2][table_size];
  static const int stack_size = (1 << (MAX_LWZ_BITS)) * 2;
  static int stack[stack_size], *sp;
  static const int *end_of_stack = stack + stack_size;
  register int i;

  if (flag) {
    set_code_size = input_code_size;
    code_size = set_code_size + 1;
    clear_code = 1 << set_code_size;
    end_code = clear_code + 1;
    max_code_size = 2 * clear_code;
    max_code = clear_code + 2;

    GetCode(in, 0, true);

    fresh = true;

    for (i = 0; i < clear_code; ++i) {
      table[0][i] = 0;
      table[1][i] = i;
    }
    for (; i < (1 << MAX_LWZ_BITS); ++i) {
      table[0][i] = table[1][0] = 0;
    }
    sp = stack;

    return 0;
  } else if (fresh) {
    fresh = FALSE;
    do {
      firstcode = oldcode = GetCode(in, code_size, false);
    } while (firstcode == clear_code);
    return firstcode;
  }

  if (sp > stack)
    return *--sp;

  while ((code = GetCode(in, code_size, false)) >= 0) {
    if (code == clear_code) {
      for (i = 0; i < clear_code; ++i) {
        table[0][i] = 0;
        table[1][i] = i;
      }
      for (; i < (1 << MAX_LWZ_BITS); ++i) {
        table[0][i] = table[1][i] = 0;
      }
      code_size = set_code_size + 1;
      max_code_size = 2 * clear_code;
      max_code = clear_code + 2;
      sp = stack;
      firstcode = oldcode =
        GetCode(in, code_size, false);
      return firstcode;
    } else if (code == end_code) {
      int count;
      unsigned char buf[260];

      if (ZeroDataBlock)
        return -2;

      while ((count = GetDataBlock(in, buf)) > 0)
        ;

      if (count != 0)
        cerr << "TFilterGIF: missing EOD in data stream (common occurence)\n";
      return -2;
    }

    incode = code;

    if (code >= max_code) {
      *sp++ = firstcode;
      code = oldcode;
    }


    while (code >= clear_code) {
      if (code >= table_size ||
          sp >= end_of_stack )
        throw runtime_error("TFilterGIF: file corrupt");
      *sp++ = table[1][code];
      if (code == table[0][code])
        throw runtime_error("TFilterGIF: circular table entry BIG ERROR");
      code = table[0][code];
    }

    if (sp >= end_of_stack )
      throw runtime_error("TFilterGIF: file corrupt");

    *sp++ = firstcode = table[1][code];

    if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
      table[0][code] = oldcode;
      table[1][code] = firstcode;
      ++max_code;
      if ((max_code >= max_code_size) &&
          (max_code_size < (1 << MAX_LWZ_BITS)))
      {
        max_code_size *= 2;
        ++code_size;
      }
    }

    oldcode = incode;

    if (sp > stack)
      return *--sp;
  }
  return code;
}

static struct
{
  int transparent;
  int delayTime;
  int inputFlag;
  int disposal;
} Gif89 = { -1, -1, -1, 0 };

static bool ReadExtension (TInBinStream &in, int label)
{
  static char buf[256];

  switch (label) {
    case 0x01:      /* Plain Text Extension */
#if 0
      if (GetDataBlock(in, (unsigned char *) buf) == 0)
        ;

      lpos = LM_to_uint (buf[0], buf[1]);
      tpos = LM_to_uint (buf[2], buf[3]);
      width = LM_to_uint (buf[4], buf[5]);
      height = LM_to_uint (buf[6], buf[7]);
      cellw = buf[8];
      cellh = buf[9];
      foreground = buf[10];
      background = buf[11];

      while (GetDataBlock (in, (unsigned char *) buf) != 0) {
        PPM_ASSIGN (image[ypos][xpos],
                    cmap[CM_RED][v],
                    cmap[CM_GREEN][v],
                    cmap[CM_BLUE][v]);
        ++index;
      }
      return false;
#else
      break;
#endif
    case 0xff:      /* Application Extension */
      break;
    case 0xfe:      /* Comment Extension */
      while (GetDataBlock(in, (unsigned char *) buf) != 0)  {
        // printf ("gif comment: %s\n", buf);
      }
      return false;
    case 0xf9:      /* Graphic Control Extension */
      (void) GetDataBlock (in, (unsigned char *) buf);
      Gif89.disposal = (buf[0] >> 2) & 0x7;
      Gif89.inputFlag = (buf[0] >> 1) & 0x1;
      Gif89.delayTime = LM_to_uint (buf[1], buf[2]);
      if ((buf[0] & 0x1) != 0)
        Gif89.transparent = buf[3];

      while (GetDataBlock (in, (unsigned char *) buf) != 0)
        ;
      return false;
    default:
//      cout << "TFilterGIF: unknown thunk type " << label << endl;
      break;
    }

  while (GetDataBlock(in, (unsigned char *) buf) != 0)
    ;

  return false;
}

bool TFilterGIF::save(ostream&)
{
  return false;
}

} // namespace toad
