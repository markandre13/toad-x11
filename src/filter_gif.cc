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
  TRGB map[256];
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


// Save GIF Picture
//---------------------------------------------------------------------------

#ifdef GIF_PATENT_EXPIRED

// the code is a modified version of 'ppmtogif.c'

#define MAXCOLORS 256

//pointer to function returning an int
typedef int (*ifunptr) (int, int);

// a code_int must be able to hold 2**MAX_LWZ_BITS values of type int, and also -1
typedef int code_int;

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else /*SIGNED_COMPARE_SLOW */
typedef long int count_int;
#endif /*SIGNED_COMPARE_SLOW */

static int colorstobpp (int);
static int GetPixel(int, int);
static void BumpPixel (void);
static int GIFNextPixel();
static void GIFEncode (TFile &, int, int, int, int, int, int, int *, int *, int *, ifunptr);

static long rowstride;
//static unsigned char *pixels;
static int cur_progress;
static int max_progress;

static void compress(int, TFile&, ifunptr);
static void output (code_int);
static void cl_block (void);    // clear hash table and write clear code to file
static void cl_hash();          // clear hash table
static void writeerr (void);
static void char_init (void);
static void char_out(int);
static void flush_char (void);

static int rows, cols;

static TFilterGIF *toad_filter;

// Quick & Dirty implementation of a GIF options menu
//----------------------------------------------------
class TConfig: 
  public TDialog
{
  public:
    TConfig(TWindow *p, const string &t);
    
    void create();
    void paint();
    void indexChanged(TScrollBar*);
    int index;
    TFilterGIF *filter;
    int palette_size;
    
    TCheckBox *interlace;
    TCheckBox *transparent;
};

TConfig::TConfig(TWindow *p, const string &t)
  :TDialog(p,t)
{
  setSize(200,200);
  
  // the controls must created in the constructor because when created
  // in `create()', the will be removed during `Destroy()'.
  interlace = new TCheckBox(this, "Interlaced");
    interlace->setShape(10,45,180,20);

  transparent = new TCheckBox(this, "Transparent");
    transparent->setShape(10,65,180,20);
}

void TConfig::create()
{
  TScrollBar *sb = new TScrollBar(this,"color_selector");
    OLD_CONNECT(this, indexChanged, sb, sb->sigValueChanged);
    sb->setShape(10,130,180,TSIZE_PREVIOUS);
    sb->setValue(0);
    sb->setVisible(1);
    sb->setRange(0,palette_size-1);

  TPushButton *pb = new TPushButton(this, "Ok");
    OLD_CONNECT(this, closeRequest, pb, pb->sigClicked);
    pb->setShape(10,200-30,100,20);
}

void TConfig::paint()
{
  TPen pen(this);
  pen.drawTextWidth(10,85,"Select a transparent color:",180);
  TRGB c;
  filter->getIndexColor(index, &c);
  pen.setColor(c);
  pen.setColorMode(TColor::DITHER);
  pen.fillRectanglePC(10,105,180,20);

  pen.setFont("arial,helvetica,sans:italic");
  pen.setColor(0,0,0);

  pen.drawTextWidth(10,10,
    "You can select some additional options now before saving:"
    ,180);
}

void TConfig::indexChanged(TScrollBar *sb)
{
  index = sb->Value();
  TPen pen(this);
  pen.setColorMode(TColor::DITHER);
  TRGB c;
  filter->getIndexColor(index, &c);
  pen.setColor(c);
  pen.fillRectanglePC(10,105,180,20);
}
#endif

bool TFilterGIF::save(ostream&)
{
#ifndef GIF_PATENT_EXPIRED
  setError("Can't write GIF due to patent problems.\n"
           "\n"
           "See\n"
           "http://www.gnu.org/philosophy/gif.html\n"
           "and\n"
           "http://lpf.ai.mit.edu/Patents/Gif/Gif.html\n"
           "for further informations.");

  return false;
#else
  ENTRYEXIT("TFilterGIF::Save");
  toad_filter = this;
  int Red[MAXCOLORS];
  int Green[MAXCOLORS];
  int Blue[MAXCOLORS];
  long BitsPerPixel;
  long interlace;
  long transparent;

  int palette_size;
  if (!convertToIndexed(&palette_size))
    return false;

  for(int i=0;i<256;i++) {
    TRGB c;
    getIndexColor(i,&c);
    Red[i]=c.r;
    Green[i]=c.g;
    Blue[i]=c.b;
  }

  TConfig dlg(NULL,"Save Bitmap: GIF Options");
  dlg.index = 0;
  dlg.filter = this;
  dlg.palette_size = palette_size;
  dlg.doModal();
  
  interlace = dlg.interlace->getValue();
  
  if (dlg.transparent->getValue()) {
    transparent = dlg.index;
  } else {
    transparent = -1;
  }

  cols = w;
  rows = h;
  rowstride = w;

  BitsPerPixel = colorstobpp(palette_size);

  GIFEncode (file, cols, rows, interlace, 0 /* background */, transparent, 
       BitsPerPixel, Red, Green, Blue, ::GetPixel);

  return true;
#endif
}

#ifdef GIF_PATENT_EXPIRED

// convert 'number of colors' to 'bit per pixel'
//-----------------------------------------------
static int colorstobpp (int colors)
{
  int bpp;

  if (colors <= 2)
    bpp = 1;
  else if (colors <= 4)
    bpp = 2;
  else if (colors <= 8)
    bpp = 3;
  else if (colors <= 16)
    bpp = 4;
  else if (colors <= 32)
    bpp = 5;
  else if (colors <= 64)
    bpp = 6;
  else if (colors <= 128)
    bpp = 7;
  else if (colors <= 256)
    bpp = 8;

  return bpp;
}

static int GetPixel(int x, int y)
{
  int p = toad_filter->GetIndexPixel(x,y);
  return p;
}


/*****************************************************************************
 *
 * GIFENCODE.C    - GIF Image compression interface
 *
 * GIFEncode( FName, GHeight, GWidth, GInterlace, Background, Transparent,
 *            BitsPerPixel, Red, Green, Blue, GetPixel )
 *
 *****************************************************************************/

static int Width, Height;
static int curx, cury;
static long CountDown;
static int Pass = 0;
static int Interlace;

// bump the 'curx' and 'cury' to point to the next pixel
//-------------------------------------------------------
static void BumpPixel ()
{
  //
  // bump the current X position
  ++curx;

  // If we are at the end of a scan line, set curx back to the beginning
  // If we are interlaced, bump the cury to the appropriate spot,
  // otherwise, just increment it.
  //---------------------------------------------------------------------
  if (curx == Width)
  {
    //      if ((++cur_progress % 5) == 0)
    //  gimp_do_progress (cur_progress, max_progress);
      
    curx = 0;

    if (!Interlace)
      ++cury;
    else
    {
      switch (Pass)
      {
        case 0:
          cury += 8;
          if (cury >= Height)
          {
            ++Pass;
            cury = 4;
          }
          break;

          case 1:
            cury += 8;
            if (cury >= Height)
            {
              ++Pass;
              cury = 2;
            }
            break;

          case 2:
            cury += 4;
            if (cury >= Height)
            {
              ++Pass;
              cury = 1;
            }
            break;
          case 3:
            cury += 2;
            break;
        }
    }
  }
}

// Return the next pixel from the image
//--------------------------------------
static int GIFNextPixel()
{
  if (CountDown == 0)             // no more pixels ?
    return EOF;                   // => RETURN
  --CountDown;                    // decrement total number of pixels

//  int pix = *(pixels + (rowstride * (long) cury) + (long) curx);
  int pix = GetPixel(curx,cury);
  BumpPixel();                    // move to the next pixel
  return pix;
}

static void GIFEncode (
  TFile &file, 
  int GWidth, int GHeight, int GInterlace, 
  int Background, 
  int Transparent,
  int BitsPerPixel, 
  int Red[], int Green[], int Blue[], 
  ifunptr GetPixel)
{
  int B;
  int RWidth, RHeight;
  int LeftOfs, TopOfs;
  int Resolution;
  int ColorMapSize;
  int InitCodeSize;
  int i;

  Interlace = GInterlace;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = TopOfs = 0;

  Resolution = BitsPerPixel;

  // calculate number of bits we are expecting
  //-------------------------------------------
  CountDown = (long) Width *(long) Height;

  // indicate which pass we are on (if interlace)
  //----------------------------------------------
  Pass = 0;

  // initial code size for compression
  //-----------------------------------
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;

  // set up the current x and y position
  //-------------------------------------
  curx = cury = 0;

  // Write the Magic header
  file.WriteString(Transparent < 0 ? "GIF87a" : "GIF89a");

  // Write out the screen width and height
  file.WriteWord(RWidth);
  file.WriteWord(RHeight);

  // Indicate that there is a global colour map
  B = 0x80;     // Yes, there is a color map

  // OR in the resolution
  B |= (Resolution - 1) << 5;

  // OR in the Bits per Pixel
  B |= (BitsPerPixel - 1);

  // Write it out
  file.WriteByte(B);

  // Write out the Background colour
  file.WriteByte(Background);

  // Byte of 0's (future expansion)
  file.WriteByte(0);

  // Write out the Global Colour Map
  for (i = 0; i < ColorMapSize; ++i)
  {
    file.WriteByte(Red[i]);
    file.WriteByte(Green[i]);
    file.WriteByte(Blue[i]);
  }

  // Write out extension for transparent colour index, if necessary.
  if (Transparent >= 0) {
    file.WriteByte('!');
    file.WriteByte(0xF9);
    file.WriteByte(4);
    file.WriteByte(1);
    file.WriteByte(0);
    file.WriteByte(0);
    file.WriteByte(Transparent);
    file.WriteByte(0);
  }
  
  // Write an Image separator
  file.WriteByte(',');

  // Write the Image header
  file.WriteWord(LeftOfs);
  file.WriteWord(TopOfs);
  file.WriteWord(Width);
  file.WriteWord(Height);

  // Write out whether or not the image is interlaced
  file.WriteByte( Interlace ? 0x40 : 0x00);

  // Write out the initial code size
  file.WriteByte(InitCodeSize);

  // Go and actually compress the data
  compress (InitCodeSize + 1, file, GetPixel);

  // Write out a Zero-length packet (to end the series)
  file.WriteByte(0);

  // Write the GIF file terminator
  file.WriteByte(';');
}

/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 * General DEFINEs
 */

// well the following is the same value as in MAX_LWZ_BITS
// #define BITS    12

/* hashtable size 80% occupancy */
#define HSIZE  5003
// toads definition of HSIZE
#define HASH_SIZE 5003

#ifdef NO_UCHAR
typedef char char_type;
#else /*NO_UCHAR */
typedef unsigned char char_type;
#endif /*NO_UCHAR */

/*
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>

static int n_bits;                    // number of bits/code
static int maxbits = MAX_LWZ_BITS;    // user settable max # bits/code
static code_int maxcode;              // maximum code, given n_bits
static code_int maxmaxcode = (code_int) 1 << MAX_LWZ_BITS;  /* should NEVER generate this code */
#ifdef COMPATIBLE   /* But wrong! */
#define MAXCODE(n_bits)        ((code_int) 1 << (n_bits) - 1)
#else /*COMPATIBLE */
#define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)
#endif /*COMPATIBLE */

static count_int htab[HSIZE];         // hashtable
static unsigned short codetab[HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static code_int hsize = HSIZE;  /* for dynamic table sizing */

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**MAX_LWZ_BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type*)(htab))[i]
#define de_stack               ((char_type*)&tab_suffixof((code_int)1<<MAX_LWZ_BITS))

static code_int free_ent = 0; /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

static int offset;
static long int in_count = 1; /* length of input */
static long int out_count = 0;  /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int g_init_bits;
static TFile *g_outfile=NULL;

static int ClearCode;
static int EOFCode;

static void compress (int init_bits, TFile &outfile, ifunptr)
{
  register code_int i /* = 0 */ ;
  register int c;
  register code_int ent;
  register code_int disp;
  register code_int hsize_reg;

  // Set up the globals
  //--------------------
  g_init_bits = init_bits;  // initial number of bits
  g_outfile = &outfile;

  /*
   * Set up the necessary values
   */
  offset = 0;
  out_count = 0;
  clear_flg = 0;
  in_count = 1;
  maxcode = MAXCODE (n_bits = g_init_bits);

  // special values
  //----------------
  ClearCode = (1 << (init_bits - 1));
  EOFCode = ClearCode + 1;
  free_ent = ClearCode + 2;

  char_init();          // is "a_count=0" meaning, set char out count to zero

  ent = GIFNextPixel(); // get the first pixel

  register long fcode;
  register int hshift = 0;
  for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
    ++hshift;
  hshift = 8 - hshift;                // set hash code range bound

  hsize_reg = hsize;
  cl_hash();                          // clear hash table

  output ((code_int) ClearCode);

#ifdef SIGNED_COMPARE_SLOW
  while ((c = GIFNextPixel()) != (unsigned) EOF)
    {
#else /*SIGNED_COMPARE_SLOW */
  while ((c = GIFNextPixel()) != EOF)
    {       /* } */
#endif /*SIGNED_COMPARE_SLOW */

      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((code_int) c << hshift) ^ ent); /* xor hashing */

      if (HashTabOf (i) == fcode)
  {
    ent = CodeTabOf (i);
    continue;
  }
      else if ((long) HashTabOf (i) < 0)  /* empty slot */
  goto nomatch;
      disp = hsize_reg - i; /* secondary hash (after G. Knott) */
      if (i == 0)
  disp = 1;
    probe:
      if ((i -= disp) < 0)
  i += hsize_reg;

      if (HashTabOf (i) == fcode)
  {
    ent = CodeTabOf (i);
    continue;
  }
      if ((long) HashTabOf (i) > 0)
  goto probe;
    nomatch:
      output ((code_int) ent);
      ++out_count;
      ent = c;
#ifdef SIGNED_COMPARE_SLOW
      if ((unsigned) free_ent < (unsigned) maxmaxcode)
  {
#else /*SIGNED_COMPARE_SLOW */
      if (free_ent < maxmaxcode)
  {     /* } */
#endif /*SIGNED_COMPARE_SLOW */
    CodeTabOf (i) = free_ent++; /* code -> hashtable */
    HashTabOf (i) = fcode;
  }
      else
  cl_block ();
    }
  /*
   * Put out the final code.
   */
  output ((code_int) ent);
  ++out_count;
  output ((code_int) EOFCode);
}

// write the given number 'code' of 'n_bits'-bits to 'g_outfile'
//--------------------------------------------------------------

/*
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a MAX_LWZ_BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static unsigned long cur_accum = 0;
static int cur_bits = 0;

static void output(code_int code)
{
  static const unsigned long masks[] =
  {0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
   0x001F, 0x003F, 0x007F, 0x00FF,
   0x01FF, 0x03FF, 0x07FF, 0x0FFF,
   0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};

  cur_accum &= masks[cur_bits];       // clear unused bits in the accumulator

  if (cur_bits > 0)                   // add 'code' to the accumulator
    cur_accum |= ((long) code << cur_bits);
  else
    cur_accum = code;

  cur_bits += n_bits;                 // add number of bits we've just added

  while (cur_bits >= 8)               // move all complete byte to the output buffer
  {
    char_out ((unsigned int) (cur_accum & 0xff));
    cur_accum >>= 8;
    cur_bits -= 8;
  }

  /*
   * If the next entry is going to be too big for the code size,
   * then increase it, if possible.
   */
  if (free_ent > maxcode || clear_flg)
  {
    if (clear_flg)
    {
      maxcode = MAXCODE (n_bits = g_init_bits);
      clear_flg = 0;
    }
    else
    {
      ++n_bits;
      if (n_bits == maxbits)
        maxcode = maxmaxcode;
      else
        maxcode = MAXCODE (n_bits);
    }
  }

  // at EOF, write the rest of the buffer
  //--------------------------------------
  if (code == EOFCode)
  {
    
    while (cur_bits > 0)
    {
      char_out ((unsigned int) (cur_accum & 0xff));
      cur_accum >>= 8;
      cur_bits -= 8;
    }
    flush_char();
    g_outfile->Flush();

    if (g_outfile->Error())
      writeerr();
  }
}

// clear hash table and write clear code to file
//-----------------------------------------------
static void cl_block()      /* table clear for block compress */
{
  cl_hash();
  free_ent = ClearCode + 2;
  clear_flg = 1;

  output ((code_int) ClearCode);
}

// clear the hash table
//----------------------
/* don't get confused, the following code means
 *
 *     for(i=0; i<HASH_SIZE; i++) htab[i]=-1L;
 *
 * but is improved for speed.
 */
static void cl_hash()
{
  register count_int *htab_p = htab + HASH_SIZE;

  register long i;
  register long m1 = -1;

  i = HASH_SIZE - 16;
  do
    {
      *(htab_p - 16) = m1;
      *(htab_p - 15) = m1;
      *(htab_p - 14) = m1;
      *(htab_p - 13) = m1;
      *(htab_p - 12) = m1;
      *(htab_p - 11) = m1;
      *(htab_p - 10) = m1;
      *(htab_p - 9) = m1;
      *(htab_p - 8) = m1;
      *(htab_p - 7) = m1;
      *(htab_p - 6) = m1;
      *(htab_p - 5) = m1;
      *(htab_p - 4) = m1;
      *(htab_p - 3) = m1;
      *(htab_p - 2) = m1;
      *(htab_p - 1) = m1;
      htab_p -= 16;
    }
  while ((i -= 16) >= 0);

  for (i += 16; i > 0; --i)
    *--htab_p = m1;
}

static void writeerr ()
{
  throw runtime_error("TFilterGIF: error writing output file");
}

/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/

/* void output(code_int)
 * Number of characters so far in this 'packet'
 */
static int a_count;

/* void output(code_int)
 * Set up the 'byte output' routine
 */
static void char_init ()
{
  a_count = 0;
  cur_accum = 0;  // Well, these two missing commands were the BUG in ..
  cur_bits = 0;   // .. the GIF compression.
}

/*
 * Define the storage for the packet accumulator
 */
static char accum[256];

/* part of: void output(code_int)
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void char_out (int c)
{
  accum[a_count++] = c;
  if (a_count >= 254)
    flush_char ();
}

/* part of: void output(code_int)
 * Flush the packet to disk, and reset the accumulator
 */
static void flush_char ()
{
  if (a_count > 0)
    {
      g_outfile->WriteByte(a_count);
      g_outfile->WriteString(accum, a_count);
      a_count = 0;
    }
}
#endif

/* The End */

} // namespace toad
